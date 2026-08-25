namespace LinearActuator.Core;

public static class ActuatorConstants
{
    public const int Count = 4;
    public const int ModuleCount = 10;
    public const int DefaultTarget = 50;
    public const int MinTarget = 0;
    public const int MaxTarget = 800;
    public const int DefaultBaudRate = 9600;
    public const string DefaultApiHost = "127.0.0.1";
    public const int DefaultApiPort = 7500;

    public static string DefaultModuleId => FormatModuleId(1);

    public static string FormatModuleId(int moduleNumber) => $"M{moduleNumber:00}";
}
