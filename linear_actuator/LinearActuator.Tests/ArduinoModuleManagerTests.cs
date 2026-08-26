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
}
