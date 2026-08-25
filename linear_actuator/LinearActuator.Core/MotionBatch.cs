namespace LinearActuator.Core;

public enum StepStatus
{
    Pending = 0,
    Active = 1,
    Finished = 2,
}

public sealed class MotionBatch
{
    public Dictionary<int, MotionStep> Steps { get; }
    public ActuatorStateBundle FromState { get; }
    public ActuatorStateBundle ToState { get; }
    public int ActiveStep { get; private set; }

    public MotionBatch(
        ActuatorStateBundle fromState,
        ActuatorStateBundle toState,
        int steps = 10)
    {
        FromState = fromState.Clone();
        ToState = toState.Clone();
        Steps = InterpolateBundles(fromState, toState, steps);
        ActiveStep = Steps.Keys.Min();
        Steps[ActiveStep].Status = StepStatus.Active;
    }

    public static Dictionary<int, MotionStep> InterpolateBundles(
        ActuatorStateBundle fromState,
        ActuatorStateBundle toState,
        int steps = 10
    )
    {
        if (steps <= 0)
        {
            throw new ArgumentOutOfRangeException(nameof(steps), "Steps must be greater than zero.");
        }

        Dictionary<int, MotionStep> interpolatedSteps = [];
        List<string> keys = fromState.Modules.Keys.Order().ToList();

        foreach (string key in keys)
        {
            if (!toState.Modules.ContainsKey(key))
            {
                throw new ArgumentException($"Target bundle is missing module {key}.", nameof(toState));
            }
        }

        for (int i = 1; i <= steps; i++)
        {
            ActuatorStateBundle midBundle = new() { Modules = [] };
            foreach (string key in keys)
            {
                ActuatorState fromStep = fromState.Modules[key];
                ActuatorState toStep = toState.Modules[key];

                double ratio = (double)i / steps;

                ActuatorState midStep = new()
                {
                    A1Current = fromStep.A1Current,
                    A1Target = InterpolateTarget(fromStep.A1Target, toStep.A1Target, ratio, key, "A1"),
                    A2Current = fromStep.A2Current,
                    A2Target = InterpolateTarget(fromStep.A2Target, toStep.A2Target, ratio, key, "A2"),
                    A3Current = fromStep.A3Current,
                    A3Target = InterpolateTarget(fromStep.A3Target, toStep.A3Target, ratio, key, "A3"),
                    A4Current = fromStep.A4Current,
                    A4Target = InterpolateTarget(fromStep.A4Target, toStep.A4Target, ratio, key, "A4"),
                };

                midBundle.Modules.Add(key, midStep);
            }

            MotionStep motionStep = new()
            {
                Status = StepStatus.Pending,
                StepBundle = midBundle,
                Step = i,
            };

            interpolatedSteps.Add(motionStep.Step, motionStep);
        }

        return interpolatedSteps;
    }

    public MotionStep? GetActiveStep()
    {
        return Steps.TryGetValue(ActiveStep, out MotionStep? step) && step.Status == StepStatus.Active
            ? step
            : null;
    }

    public bool AdvanceIfActiveStepReached(ActuatorStateBundle actualState, double tolerance = 2)
    {
        MotionStep? step = GetActiveStep();
        if (step is null)
        {
            return false;
        }

        if (!IsStepReached(step.StepBundle, actualState, tolerance))
        {
            return false;
        }

        step.Status = StepStatus.Finished;
        int nextStep = ActiveStep + 1;
        if (Steps.TryGetValue(nextStep, out MotionStep? next))
        {
            next.Status = StepStatus.Active;
            ActiveStep = nextStep;
            return true;
        }

        ActiveStep = 0;
        return true;
    }

    public bool IsFinished => Steps.Values.All(step => step.Status == StepStatus.Finished);

    private static double InterpolateTarget(double? from, double? to, double ratio, string moduleId, string actuatorId)
    {
        if (from is null || to is null)
        {
            throw new ArgumentException($"{moduleId} {actuatorId} targets must not be null.");
        }

        return Math.Round(((to.Value - from.Value) * ratio) + from.Value);
    }

    private static bool IsStepReached(ActuatorStateBundle targetStep, ActuatorStateBundle actualState, double tolerance)
    {
        foreach (KeyValuePair<string, ActuatorState> targetModule in targetStep.Modules)
        {
            if (!actualState.Modules.TryGetValue(targetModule.Key, out ActuatorState? actualModule))
            {
                return false;
            }

            ActuatorState target = targetModule.Value;
            if (!IsActuatorReached(actualModule.A1Current, target.A1Target, tolerance)
                || !IsActuatorReached(actualModule.A2Current, target.A2Target, tolerance)
                || !IsActuatorReached(actualModule.A3Current, target.A3Target, tolerance)
                || !IsActuatorReached(actualModule.A4Current, target.A4Target, tolerance))
            {
                return false;
            }
        }

        return true;
    }

    private static bool IsActuatorReached(double? current, double? target, double tolerance)
    {
        return current is not null && target is not null && Math.Abs(current.Value - target.Value) <= tolerance;
    }
}

public sealed class MotionStep
{
    public int Step { get; set; }
    public StepStatus Status { get; set; }
    public required ActuatorStateBundle StepBundle { get; set; }
}
