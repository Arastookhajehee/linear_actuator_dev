using Newtonsoft.Json;

namespace LinearActuator.Core;

public sealed class ActuatorState
{
    [JsonProperty("a1_current")]
    public double? A1Current { get; set; }

    [JsonProperty("a1_target")]
    public double? A1Target { get; set; } = ActuatorConstants.DefaultTarget;

    [JsonProperty("a2_current")]
    public double? A2Current { get; set; }

    [JsonProperty("a2_target")]
    public double? A2Target { get; set; } = ActuatorConstants.DefaultTarget;

    [JsonProperty("a3_current")]
    public double? A3Current { get; set; }

    [JsonProperty("a3_target")]
    public double? A3Target { get; set; } = ActuatorConstants.DefaultTarget;

    [JsonProperty("a4_current")]
    public double? A4Current { get; set; }

    [JsonProperty("a4_target")]
    public double? A4Target { get; set; } = ActuatorConstants.DefaultTarget;

    public static ActuatorState CreateDefault() => new();

    public ActuatorState Clone() => new()
    {
        A1Current = A1Current,
        A1Target = A1Target,
        A2Current = A2Current,
        A2Target = A2Target,
        A3Current = A3Current,
        A3Target = A3Target,
        A4Current = A4Current,
        A4Target = A4Target
    };
}
