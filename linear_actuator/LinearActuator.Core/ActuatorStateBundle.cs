using System.Text.Json.Nodes;
using System.Text.Json.Serialization;

namespace LinearActuator.Core;

public sealed class ActuatorStateBundle
{
    // will have up to 10 Actuator modules
    public Dictionary<string, ActuatorState> Bundle;

    public static ActuatorStateBundle CreateDefault() => new();

    public string ToJSON()
    {
        // serialize to JSON
        return "";
    }

    public static ActuatorStateBundle FromJson(string json)
    {
        return null;
    }
}
