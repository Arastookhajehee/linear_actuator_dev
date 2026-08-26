using LinearActuator.Core;

namespace LinearActuator.Infrastructure;

public sealed class SerialModuleManager : IDisposable
{
    private readonly Dictionary<string, SerialActuatorConnection> connectionsByPort = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, string> portByModuleId = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, string> moduleIdByPort = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, Queue<int>> binaryIdSamplesByPort = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, int?> stableBinaryIdByPort = new(StringComparer.OrdinalIgnoreCase);
    private readonly HashSet<string> duplicateModuleIds = new(StringComparer.OrdinalIgnoreCase);

    public event EventHandler<SerialTelemetryEventArgs>? TelemetryReceived;
    public event EventHandler<SerialMessageEventArgs>? MessageReceived;
    public event EventHandler<SerialMappingEventArgs>? MappingChanged;
    public event EventHandler<SerialDuplicateModuleEventArgs>? DuplicateModuleIdDetected;

    public bool IsPortConnected(string comPort)
    {
        return connectionsByPort.TryGetValue(comPort, out SerialActuatorConnection? connection) && connection.IsConnected;
    }

    public bool IsConnected(string moduleId)
    {
        return portByModuleId.TryGetValue(moduleId, out string? comPort) && IsPortConnected(comPort);
    }

    public bool StartPort(string comPort, int baudRate)
    {
        StopPort(comPort);

        if (string.IsNullOrWhiteSpace(comPort))
        {
            return false;
        }

        SerialActuatorConnection connection = new(comPort);
        connection.TelemetryReceived += (_, args) => ProcessTelemetry(args.ComPort, args.Telemetry);
        connection.MessageReceived += (_, args) => MessageReceived?.Invoke(this, args);

        try
        {
            connection.Start(comPort, baudRate);
            connectionsByPort[comPort] = connection;
            MessageReceived?.Invoke(this, new SerialMessageEventArgs(comPort, "Connected, waiting for ID"));
            return true;
        }
        catch (Exception ex)
        {
            connection.Dispose();
            MessageReceived?.Invoke(this, new SerialMessageEventArgs(comPort, $"Serial unavailable on {comPort}: {ex.Message}"));
            return false;
        }
    }

    public void StopPort(string comPort)
    {
        if (connectionsByPort.Remove(comPort, out SerialActuatorConnection? connection))
        {
            connection.Dispose();
        }

        ClearPortMapping(comPort);
        binaryIdSamplesByPort.Remove(comPort);
        stableBinaryIdByPort.Remove(comPort);
    }

    public void Start(IEnumerable<PortMapping> mappings)
    {
        Stop();

        foreach (PortMapping mapping in mappings.OrderBy(mapping => mapping.ModuleId))
        {
            if (mapping.SerialEnabled)
            {
                StartPort(mapping.ComPort, mapping.BaudRate);
            }
        }
    }

    public bool StartModule(PortMapping mapping)
    {
        return mapping.SerialEnabled && StartPort(mapping.ComPort, mapping.BaudRate);
    }

    public void StopModule(string moduleId)
    {
        if (portByModuleId.TryGetValue(moduleId, out string? comPort))
        {
            StopPort(comPort);
        }
    }

    public void Stop()
    {
        foreach (SerialActuatorConnection connection in connectionsByPort.Values)
        {
            connection.Dispose();
        }

        connectionsByPort.Clear();
        ClearMappings();
    }

    public void ClearMappings()
    {
        List<KeyValuePair<string, string>> mappedPorts = moduleIdByPort.ToList();
        portByModuleId.Clear();
        moduleIdByPort.Clear();
        binaryIdSamplesByPort.Clear();
        stableBinaryIdByPort.Clear();
        duplicateModuleIds.Clear();

        foreach (KeyValuePair<string, string> mapping in mappedPorts)
        {
            MappingChanged?.Invoke(this, new SerialMappingEventArgs(mapping.Key, mapping.Value, false));
        }
    }

    public async Task SendTargetsAsync(string moduleId, ActuatorState state, CancellationToken cancellationToken = default)
    {
        if (duplicateModuleIds.Contains(moduleId)
            || !portByModuleId.TryGetValue(moduleId, out string? comPort)
            || !connectionsByPort.TryGetValue(comPort, out SerialActuatorConnection? connection))
        {
            return;
        }

        try
        {
            await connection.SendTargetsAsync(state, cancellationToken);
        }
        catch (Exception ex)
        {
            MessageReceived?.Invoke(this, new SerialMessageEventArgs(comPort, $"Serial write failed: {ex.Message}", moduleId));
        }
    }

    public void Dispose()
    {
        Stop();
    }

    private void ProcessTelemetry(string comPort, ActuatorState telemetry)
    {
        if (!binaryIdSamplesByPort.TryGetValue(comPort, out Queue<int>? binaryIdSamples))
        {
            binaryIdSamples = new Queue<int>();
            binaryIdSamplesByPort[comPort] = binaryIdSamples;
        }

        stableBinaryIdByPort.TryGetValue(comPort, out int? previousStableValue);
        int? stableValue = ArduinoModuleManager.UpdateBinaryIdAverage(binaryIdSamples, telemetry.BinaryIdValue, previousStableValue);
        stableBinaryIdByPort[comPort] = stableValue;
        telemetry.BinaryIdAverageValue = stableValue;

        string? moduleId = ArduinoModuleManager.FormatModuleId(stableValue);
        if (moduleId is null)
        {
            if (stableValue is not null)
            {
                ClearPortMapping(comPort);
                MessageReceived?.Invoke(this, new SerialMessageEventArgs(comPort, $"Invalid ID {stableValue}"));
            }
            else
            {
                MessageReceived?.Invoke(this, new SerialMessageEventArgs(comPort, "Connected, waiting for ID"));
            }

            return;
        }

        if (duplicateModuleIds.Contains(moduleId))
        {
            ClearPortMapping(comPort);
            return;
        }

        if (portByModuleId.TryGetValue(moduleId, out string? existingComPort) && !existingComPort.Equals(comPort, StringComparison.OrdinalIgnoreCase))
        {
            duplicateModuleIds.Add(moduleId);
            ClearPortMapping(existingComPort);
            ClearPortMapping(comPort);
            DuplicateModuleIdDetected?.Invoke(this, new SerialDuplicateModuleEventArgs(moduleId, existingComPort, comPort));
            return;
        }

        duplicateModuleIds.Remove(moduleId);
        SetPortMapping(comPort, moduleId);
        TelemetryReceived?.Invoke(this, new SerialTelemetryEventArgs(comPort, moduleId, telemetry));
    }

    private void SetPortMapping(string comPort, string moduleId)
    {
        if (moduleIdByPort.TryGetValue(comPort, out string? currentModuleId)
            && currentModuleId.Equals(moduleId, StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        ClearPortMapping(comPort);
        moduleIdByPort[comPort] = moduleId;
        portByModuleId[moduleId] = comPort;
        MappingChanged?.Invoke(this, new SerialMappingEventArgs(comPort, moduleId, true));
        MessageReceived?.Invoke(this, new SerialMessageEventArgs(comPort, $"Mapped to {moduleId}", moduleId));
    }

    private void ClearPortMapping(string comPort)
    {
        if (!moduleIdByPort.Remove(comPort, out string? moduleId))
        {
            return;
        }

        portByModuleId.Remove(moduleId);
        MappingChanged?.Invoke(this, new SerialMappingEventArgs(comPort, moduleId, false));
    }
}
