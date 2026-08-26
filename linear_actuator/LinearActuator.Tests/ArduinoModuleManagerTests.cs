using LinearActuator.Core;

namespace LinearActuator.Tests;

public sealed class ArduinoModuleManagerTests
{
    [Fact]
    public void UpdateBinaryIdAverage_WaitsForTenSamples()
    {
        Queue<int> samples = new();
        int? stableValue = null;

        for (int i = 0; i < 9; i++)
        {
            stableValue = ArduinoModuleManager.UpdateBinaryIdAverage(samples, 5, stableValue);
        }

        Assert.Null(stableValue);
    }

    [Fact]
    public void UpdateBinaryIdAverage_RoundsStableNearIntegerAverage()
    {
        Queue<int> samples = new();
        int? stableValue = null;

        for (int i = 0; i < 9; i++)
        {
            stableValue = ArduinoModuleManager.UpdateBinaryIdAverage(samples, 5, stableValue);
        }

        stableValue = ArduinoModuleManager.UpdateBinaryIdAverage(samples, 4, stableValue);

        Assert.Equal(5, stableValue);
    }

    [Fact]
    public void UpdateBinaryIdAverage_UsesLatestTenSamples()
    {
        Queue<int> samples = new();
        int? stableValue = null;

        for (int i = 0; i < 10; i++)
        {
            stableValue = ArduinoModuleManager.UpdateBinaryIdAverage(samples, 2, stableValue);
        }

        for (int i = 0; i < 10; i++)
        {
            stableValue = ArduinoModuleManager.UpdateBinaryIdAverage(samples, 7, stableValue);
        }

        Assert.Equal(7, stableValue);
    }

    [Fact]
    public void UpdateBinaryIdAverage_PreservesPreviousStableValueWhenNoisy()
    {
        Queue<int> samples = new();
        int? stableValue = null;

        for (int i = 0; i < 10; i++)
        {
            stableValue = ArduinoModuleManager.UpdateBinaryIdAverage(samples, 5, stableValue);
        }

        for (int i = 0; i < 5; i++)
        {
            stableValue = ArduinoModuleManager.UpdateBinaryIdAverage(samples, 4, stableValue);
            stableValue = ArduinoModuleManager.UpdateBinaryIdAverage(samples, 5, stableValue);
        }

        Assert.Equal(5, stableValue);
    }

    [Theory]
    [InlineData(1, "M01")]
    [InlineData(10, "M10")]
    [InlineData(0, null)]
    [InlineData(11, null)]
    [InlineData(15, null)]
    public void FormatModuleId_OnlyMapsValidModuleIds(int binaryId, string? moduleId)
    {
        Assert.Equal(moduleId, ArduinoModuleManager.FormatModuleId(binaryId));
    }
}
