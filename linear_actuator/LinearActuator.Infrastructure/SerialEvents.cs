using LinearActuator.Core;

namespace LinearActuator.Infrastructure;

public sealed class SerialTelemetryEventArgs : EventArgs
{
    public SerialTelemetryEventArgs(string comPort, string? moduleId, ActuatorState telemetry)
    {
        ComPort = comPort;
        ModuleId = moduleId;
        Telemetry = telemetry;
    }

    public string ComPort { get; }
    public string? ModuleId { get; }
    public ActuatorState Telemetry { get; }
}

public sealed class SerialMessageEventArgs : EventArgs
{
    public SerialMessageEventArgs(string comPort, string message, string? moduleId = null)
    {
        ComPort = comPort;
        Message = message;
        ModuleId = moduleId;
    }

    public string ComPort { get; }
    public string Message { get; }
    public string? ModuleId { get; }
}

public sealed class SerialMappingEventArgs : EventArgs
{
    public SerialMappingEventArgs(string comPort, string? moduleId, bool isMapped)
    {
        ComPort = comPort;
        ModuleId = moduleId;
        IsMapped = isMapped;
    }

    public string ComPort { get; }
    public string? ModuleId { get; }
    public bool IsMapped { get; }
}

public sealed class SerialDuplicateModuleEventArgs : EventArgs
{
    public SerialDuplicateModuleEventArgs(string moduleId, string firstComPort, string secondComPort)
    {
        ModuleId = moduleId;
        FirstComPort = firstComPort;
        SecondComPort = secondComPort;
    }

    public string ModuleId { get; }
    public string FirstComPort { get; }
    public string SecondComPort { get; }
}
