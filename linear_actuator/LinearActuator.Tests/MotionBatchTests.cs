using LinearActuator.Core;

namespace LinearActuator.Tests;

public sealed class MotionBatchTests
{
    [Fact]
    public void InterpolateBundles_UsesStepCountForRatiosAndFinalStep()
    {
        ActuatorStateBundle from = ActuatorStateBundle.CreateDefault();
        ActuatorStateBundle to = ActuatorStateBundle.CreateDefault();
        to.Modules["M01"].A1Target = 150;

        Dictionary<int, MotionStep> steps = MotionBatch.InterpolateBundles(from, to, 4);

        Assert.Equal(4, steps.Count);
        Assert.Equal(75, steps[1].StepBundle.Modules["M01"].A1Target);
        Assert.Equal(100, steps[2].StepBundle.Modules["M01"].A1Target);
        Assert.Equal(125, steps[3].StepBundle.Modules["M01"].A1Target);
        Assert.Equal(150, steps[4].StepBundle.Modules["M01"].A1Target);
    }

    [Fact]
    public void Constructor_MarksFirstStepActive()
    {
        MotionBatch batch = new(ActuatorStateBundle.CreateDefault(), ActuatorStateBundle.CreateDefault(), 3);

        Assert.Equal(1, batch.ActiveStep);
        Assert.Equal(StepStatus.Active, batch.Steps[1].Status);
        Assert.Equal(StepStatus.Pending, batch.Steps[2].Status);
    }

    [Fact]
    public void InterpolateBundles_RejectsMissingTargetModule()
    {
        ActuatorStateBundle from = ActuatorStateBundle.CreateDefault();
        ActuatorStateBundle to = ActuatorStateBundle.CreateDefault();
        to.Modules.Remove("M02");

        Assert.Throws<ArgumentException>(() => MotionBatch.InterpolateBundles(from, to));
    }

    [Fact]
    public void InterpolateBundles_RejectsNullTargets()
    {
        ActuatorStateBundle from = ActuatorStateBundle.CreateDefault();
        ActuatorStateBundle to = ActuatorStateBundle.CreateDefault();
        to.Modules["M01"].A1Target = null;

        Assert.Throws<ArgumentException>(() => MotionBatch.InterpolateBundles(from, to));
    }

    [Fact]
    public void AdvanceIfActiveStepReached_WaitsUntilAllCurrentsReachActiveTargets()
    {
        ActuatorStateBundle from = ActuatorStateBundle.CreateDefault();
        ActuatorStateBundle to = ActuatorStateBundle.CreateDefault();
        to.Modules["M01"].A1Target = 60;

        MotionBatch batch = new(from, to, 1);
        ActuatorStateBundle actual = batch.GetActiveStep()!.StepBundle.Clone();
        foreach (ActuatorState module in actual.Modules.Values)
        {
            module.A1Current = module.A1Target;
            module.A2Current = module.A2Target;
            module.A3Current = module.A3Target;
            module.A4Current = module.A4Target;
        }

        actual.Modules["M01"].A1Current = 59;

        bool advanced = batch.AdvanceIfActiveStepReached(actual, tolerance: 1);

        Assert.True(advanced);
        Assert.True(batch.IsFinished);
    }

    [Fact]
    public void AdvanceIfActiveStepReached_DoesNotAdvanceWhenAnyCurrentIsMissing()
    {
        MotionBatch batch = new(ActuatorStateBundle.CreateDefault(), ActuatorStateBundle.CreateDefault(), 1);

        bool advanced = batch.AdvanceIfActiveStepReached(ActuatorStateBundle.CreateDefault());

        Assert.False(advanced);
        Assert.False(batch.IsFinished);
    }
}
