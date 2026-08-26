using LinearActuator.Core;

namespace LinearActuator.Infrastructure;

public sealed class PortMapping
{
    public int Id { get; set; }
    public string ModuleId { get; set; } = ActuatorConstants.DefaultModuleId;
    public string ComPort { get; set; } = string.Empty;
    public int BaudRate { get; set; } = ActuatorConstants.DefaultBaudRate;
    public bool SerialEnabled { get; set; }
}
