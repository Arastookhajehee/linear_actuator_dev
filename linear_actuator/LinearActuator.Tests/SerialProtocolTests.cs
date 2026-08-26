using LinearActuator.Core;
using Newtonsoft.Json;

namespace LinearActuator.Tests;

public sealed class SerialProtocolTests
{
    [Fact]
    public void FormatTargetCommand_UsesArchiveCsvCommand()
    {
        ActuatorState state = new()
        {
            A1Target = 10,
            A2Target = 20,
            A3Target = 30,
            A4Target = 40
        };

        string command = SerialProtocol.FormatTargetCommand(state);

        Assert.Equal("T,10,20,30,40\n", command);
    }

    [Theory]
    [InlineData(-1)]
    [InlineData(10.5)]
    [InlineData(801)]
    public void FormatTargetCommand_RejectsTargetsOutsideArduinoRange(double target)
    {
        ActuatorState state = new()
        {
            A1Target = target,
            A2Target = 20,
            A3Target = 30,
            A4Target = 40
        };

        Assert.Throws<ArgumentOutOfRangeException>(() => SerialProtocol.FormatTargetCommand(state));
    }

    [Fact]
    public void NewtonsoftSerialization_UsesApiJsonNames()
    {
        ActuatorStateBundle bundle = new()
        {
            Modules =
            {
                ["M01"] = new ActuatorState
                {
                    A1Target = 123,
                    A2Target = 234,
                    A3Target = 345,
                    A4Target = 456
                }
            }
        };

        string json = JsonConvert.SerializeObject(bundle);

        Assert.Contains("\"modules\"", json);
        Assert.Contains("\"a1_target\":123", json);
        Assert.DoesNotContain("Modules", json);
        Assert.DoesNotContain("A1Target", json);
    }

    [Fact]
    public void NewtonsoftDeserialization_ReadsApiJsonNames()
    {
        ActuatorStateBundle? bundle = JsonConvert.DeserializeObject<ActuatorStateBundle>("{\"modules\":{\"M01\":{\"a1_target\":123,\"a2_target\":234,\"a3_target\":345,\"a4_target\":456}}}");

        Assert.NotNull(bundle);
        Assert.Equal(123, bundle.Modules["M01"].A1Target);
        Assert.Equal(234, bundle.Modules["M01"].A2Target);
        Assert.Equal(345, bundle.Modules["M01"].A3Target);
        Assert.Equal(456, bundle.Modules["M01"].A4Target);
    }

    [Fact]
    public void ParseTelemetry_ReadsArchiveJsonShape()
    {
        ActuatorState? state = SerialProtocol.ParseTelemetry("{\"a1_current\":1.25,\"a1_target\":10,\"a2_current\":2,\"a2_target\":20,\"a3_current\":3,\"a3_target\":30,\"a4_current\":4,\"a4_target\":40,\"binary_id_pin_23\":0,\"binary_id_pin_25\":1,\"binary_id_pin_27\":0,\"binary_id_pin_29\":1,\"binary_id_value\":5}");

        Assert.NotNull(state);
        Assert.Equal(1.25, state.A1Current);
        Assert.Equal(10, state.A1Target);
        Assert.Equal(2, state.A2Current);
        Assert.Equal(20, state.A2Target);
        Assert.Equal(3, state.A3Current);
        Assert.Equal(30, state.A3Target);
        Assert.Equal(4, state.A4Current);
        Assert.Equal(40, state.A4Target);
        Assert.Equal(0, state.BinaryIdPin23);
        Assert.Equal(1, state.BinaryIdPin25);
        Assert.Equal(0, state.BinaryIdPin27);
        Assert.Equal(1, state.BinaryIdPin29);
        Assert.Equal(5, state.BinaryIdValue);
    }
}
