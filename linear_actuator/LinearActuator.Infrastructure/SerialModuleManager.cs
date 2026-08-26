using LinearActuator.Core;

namespace LinearActuator.Infrastructure;

public sealed class SerialModuleManager : IDisposable
{
    private readonly Dictionary<string, SerialActuatorConnection> connections = new();
    private readonly HashSet<string> enabledModules = new(StringComparer.OrdinalIgnoreCase);

    public event EventHandler<SerialTelemetryEventArgs>? TelemetryReceived;
    public event EventHandler<SerialMessageEventArgs>? MessageReceived;

    public IReadOnlyDictionary<string, bool> ConnectionStatuses => connections
        .ToDictionary(pair => pair.Key, pair => pair.Value.IsConnected, StringComparer.OrdinalIgnoreCase);

    public bool IsConnected(string moduleId)
    {
        return connections.TryGetValue(moduleId, out SerialActuatorConnection? connection) && connection.IsConnected;
    }

    public void Start(IEnumerable<PortMapping> mappings)
    {
        Stop();

        foreach (PortMapping mapping in mappings.OrderBy(mapping => mapping.ModuleId))
        {
            StartModule(mapping);
        }
    }

    public bool StartModule(PortMapping mapping)
    {
        StopModule(mapping.ModuleId);

        if (!mapping.SerialEnabled || string.IsNullOrWhiteSpace(mapping.ComPort))
        {
            return false;
        }

        SerialActuatorConnection connection = new(mapping.ModuleId);
        connection.TelemetryReceived += (_, args) => TelemetryReceived?.Invoke(this, args);
        connection.MessageReceived += (_, args) => MessageReceived?.Invoke(this, args);

        try
        {
            connection.Start(mapping.ComPort, mapping.BaudRate);
            connections[mapping.ModuleId] = connection;
            enabledModules.Add(mapping.ModuleId);
            return true;
        }
        catch (Exception ex)
        {
            connection.Dispose();
            MessageReceived?.Invoke(this, new SerialMessageEventArgs(mapping.ModuleId, $"Serial unavailable on {mapping.ComPort}: {ex.Message}"));
            return false;
        }
    }

    public void StopModule(string moduleId)
    {
        if (connections.Remove(moduleId, out SerialActuatorConnection? connection))
        {
            connection.Dispose();
        }

        enabledModules.Remove(moduleId);
    }

    public void Stop()
    {
        foreach (SerialActuatorConnection connection in connections.Values)
        {
            connection.Dispose();
        }

        connections.Clear();
        enabledModules.Clear();
    }

    public async Task SendTargetsAsync(string moduleId, ActuatorState state, CancellationToken cancellationToken = default)
    {
        if (!enabledModules.Contains(moduleId) || !connections.TryGetValue(moduleId, out SerialActuatorConnection? connection))
        {
            return;
        }

        try
        {
            await connection.SendTargetsAsync(state, cancellationToken);
        }
        catch (Exception ex)
        {
            MessageReceived?.Invoke(this, new SerialMessageEventArgs(moduleId, $"Serial write failed: {ex.Message}"));
        }
    }

    public void Dispose()
    {
        Stop();
    }
}
