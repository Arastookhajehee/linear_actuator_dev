using LinearActuator.Core;

namespace LinearActuator.Infrastructure;

public sealed class PortMapping
{
    public int Id { get; set; }
    public string Name { get; set; } = "API01";
    public string ComPort { get; set; } = "COM4";
    public string ApiHost { get; set; } = ActuatorConstants.DefaultApiHost;
    public int ApiPort { get; set; } = ActuatorConstants.DefaultApiPort;
    public int BaudRate { get; set; } = ActuatorConstants.DefaultBaudRate;
    public bool Enabled { get; set; } = true;
}
