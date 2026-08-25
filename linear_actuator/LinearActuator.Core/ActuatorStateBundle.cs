using System.Text.Json;
using System.Text.Json.Serialization;

namespace LinearActuator.Core;

public sealed class ActuatorStateBundle
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNameCaseInsensitive = true,
        WriteIndented = true
    };

    [JsonPropertyName("modules")]
    public Dictionary<string, ActuatorState> Modules { get; set; } = new();

    public static ActuatorStateBundle CreateDefault()
    {
        ActuatorStateBundle bundle = new();

        for (int i = 1; i <= ActuatorConstants.ModuleCount; i++)
        {
            bundle.Modules[ActuatorConstants.FormatModuleId(i)] = ActuatorState.CreateDefault();
        }

        return bundle;
    }

    public ActuatorStateBundle Clone()
    {
        ActuatorStateBundle clone = new();

        foreach (KeyValuePair<string, ActuatorState> module in Modules)
        {
            clone.Modules[module.Key] = module.Value.Clone();
        }

        return clone;
    }

    public string ToJson() => JsonSerializer.Serialize(this, JsonOptions);

    public static ActuatorStateBundle? FromJson(string json) => JsonSerializer.Deserialize<ActuatorStateBundle>(json, JsonOptions);
}
