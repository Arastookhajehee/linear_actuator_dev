using LinearActuator.Core;

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
    [InlineData(801)]
    public void FormatTargetCommand_RejectsTargetsOutsideArduinoRange(int target)
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
    public void ParseTelemetry_ReadsArchiveJsonShape()
    {
        ActuatorState? state = SerialProtocol.ParseTelemetry("{\"a1_current\":1.25,\"a1_target\":10,\"a2_current\":2,\"a2_target\":20,\"a3_current\":3,\"a3_target\":30,\"a4_current\":4,\"a4_target\":40}");

        Assert.NotNull(state);
        Assert.Equal(1.25, state.A1Current);
        Assert.Equal(10, state.A1Target);
        Assert.Equal(2, state.A2Current);
        Assert.Equal(20, state.A2Target);
        Assert.Equal(3, state.A3Current);
        Assert.Equal(30, state.A3Target);
        Assert.Equal(4, state.A4Current);
        Assert.Equal(40, state.A4Target);
    }
}
