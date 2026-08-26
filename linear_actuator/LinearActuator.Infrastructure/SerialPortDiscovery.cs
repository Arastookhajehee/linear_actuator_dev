using System.IO.Ports;

namespace LinearActuator.Infrastructure;

public static class SerialPortDiscovery
{
    public static List<string> GetPortNames()
    {
        return SerialPort.GetPortNames()
            .OrderBy(GetPortNumber)
            .ThenBy(port => port, StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private static int GetPortNumber(string portName)
    {
        return portName.StartsWith("COM", StringComparison.OrdinalIgnoreCase)
            && int.TryParse(portName[3..], out int portNumber)
            ? portNumber
            : int.MaxValue;
    }
}
