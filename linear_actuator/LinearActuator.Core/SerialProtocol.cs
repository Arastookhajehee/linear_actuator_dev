using Newtonsoft.Json;

namespace LinearActuator.Core;

public static class SerialProtocol
{
    public static string FormatTargetCommand(ActuatorState state)
    {
        int a1 = RequireValidTarget(state.A1Target, nameof(state.A1Target));
        int a2 = RequireValidTarget(state.A2Target, nameof(state.A2Target));
        int a3 = RequireValidTarget(state.A3Target, nameof(state.A3Target));
        int a4 = RequireValidTarget(state.A4Target, nameof(state.A4Target));

        return $"T,{a1},{a2},{a3},{a4}\n";
    }

    public static ActuatorState? ParseTelemetry(string line)
    {
        try
        {
            return JsonConvert.DeserializeObject<ActuatorState>(line);
        }
        catch (JsonException)
        {
            return null;
        }
    }

    public static bool IsValidTarget(double? value)
    {
        return value is >= ActuatorConstants.MinTarget and <= ActuatorConstants.MaxTarget
            && value == Math.Truncate(value.Value);
    }

    private static int RequireValidTarget(double? value, string name)
    {
        if (!IsValidTarget(value))
        {
            throw new ArgumentOutOfRangeException(name, $"Target must be in {ActuatorConstants.MinTarget}..{ActuatorConstants.MaxTarget}.");
        }

        return (int)value.GetValueOrDefault();
    }
}
