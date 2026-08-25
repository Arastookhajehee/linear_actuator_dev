using System.IO.Ports;
using System.Text;
using LinearActuator.Core;

namespace LinearActuator.Infrastructure;

public sealed class SerialActuatorConnection : IDisposable
{
    private readonly string moduleId;
    private SerialPort? serialPort;
    private CancellationTokenSource? readCancellation;
    private Task? readTask;

    public SerialActuatorConnection(string moduleId)
    {
        this.moduleId = moduleId;
    }

    public event EventHandler<SerialTelemetryEventArgs>? TelemetryReceived;
    public event EventHandler<SerialMessageEventArgs>? MessageReceived;

    public bool IsConnected => serialPort?.IsOpen == true;

    public void Start(string comPort, int baudRate)
    {
        Stop();

        serialPort = new SerialPort(comPort, baudRate)
        {
            Encoding = Encoding.UTF8,
            NewLine = "\n",
            ReadTimeout = 500
        };

        serialPort.Open();
        readCancellation = new CancellationTokenSource();
        readTask = Task.Run(() => ReadLoop(readCancellation.Token));
    }

    public void Stop()
    {
        readCancellation?.Cancel();

        if (serialPort is { IsOpen: true })
        {
            serialPort.Close();
        }

        serialPort?.Dispose();
        serialPort = null;
        readCancellation?.Dispose();
        readCancellation = null;
        readTask = null;
    }

    public Task<bool> SendTargetsAsync(ActuatorState state, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (serialPort is not { IsOpen: true } port)
        {
            return Task.FromResult(false);
        }

        string command = SerialProtocol.FormatTargetCommand(state);
        port.Write(command);
        return Task.FromResult(true);
    }

    public void Dispose()
    {
        Stop();
    }

    private void ReadLoop(CancellationToken cancellationToken)
    {
        while (!cancellationToken.IsCancellationRequested)
        {
            string? line = null;
            try
            {
                line = serialPort?.ReadLine()?.Trim();
            }
            catch (TimeoutException)
            {
                continue;
            }
            catch (InvalidOperationException)
            {
                break;
            }
            catch (IOException ex)
            {
                MessageReceived?.Invoke(this, new SerialMessageEventArgs(moduleId, ex.Message));
                break;
            }

            if (string.IsNullOrWhiteSpace(line))
            {
                continue;
            }

            ActuatorState? telemetry = SerialProtocol.ParseTelemetry(line);
            if (telemetry is null)
            {
                MessageReceived?.Invoke(this, new SerialMessageEventArgs(moduleId, line));
                continue;
            }

            TelemetryReceived?.Invoke(this, new SerialTelemetryEventArgs(moduleId, telemetry));
        }
    }
}
