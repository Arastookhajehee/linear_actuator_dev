using Newtonsoft.Json;

namespace LinearActuator.Core;

public sealed class ActuatorStateBundle
{
    private static readonly JsonSerializerSettings JsonSettings = new()
    {
        Formatting = Formatting.Indented
    };

    [JsonProperty("modules")]
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

    public string ToJson() => JsonConvert.SerializeObject(this, JsonSettings);

    public static ActuatorStateBundle? FromJson(string json) => JsonConvert.DeserializeObject<ActuatorStateBundle>(json, JsonSettings);
}
