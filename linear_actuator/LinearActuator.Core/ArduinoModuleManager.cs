namespace LinearActuator.Core;

public static class ArduinoModuleManager
{
    public const int BinaryIdSampleCount = 10;
    public const double BinaryIdIntegerTolerance = 0.2;

    public static int? UpdateBinaryIdAverage(Queue<int> samples, int? value, int? previousStableValue)
    {
        if (value is null)
        {
            return previousStableValue;
        }

        samples.Enqueue(value.Value);
        while (samples.Count > BinaryIdSampleCount)
        {
            samples.Dequeue();
        }

        if (samples.Count < BinaryIdSampleCount)
        {
            return previousStableValue;
        }

        double average = samples.Average();
        int rounded = (int)Math.Round(average, MidpointRounding.AwayFromZero);

        return Math.Abs(average - rounded) <= BinaryIdIntegerTolerance
            ? rounded
            : previousStableValue;
    }
}
