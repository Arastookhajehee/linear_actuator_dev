using LinearActuator.Core;

namespace LinearActuator.Infrastructure;

public sealed class SerialTelemetryEventArgs : EventArgs
{
    public SerialTelemetryEventArgs(string moduleId, ActuatorState telemetry)
    {
        ModuleId = moduleId;
        Telemetry = telemetry;
    }

    public string ModuleId { get; }
    public ActuatorState Telemetry { get; }
}

public sealed class SerialMessageEventArgs : EventArgs
{
    public SerialMessageEventArgs(string moduleId, string message)
    {
        ModuleId = moduleId;
        Message = message;
    }

    public string ModuleId { get; }
    public string Message { get; }
}
