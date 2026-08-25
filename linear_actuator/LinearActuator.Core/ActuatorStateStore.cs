namespace LinearActuator.Core;

public sealed class ActuatorStateStore
{
    private readonly object syncRoot = new();
    private ActuatorState state = ActuatorState.CreateDefault();

    public event EventHandler<ActuatorState>? StateChanged;

    public ActuatorState Snapshot()
    {
        lock (syncRoot)
        {
            return state.Clone();
        }
    }

    public void ReplaceState(ActuatorState nextState)
    {
        ActuatorState snapshot;
        lock (syncRoot)
        {
            state = nextState.Clone();
            snapshot = state.Clone();
        }

        StateChanged?.Invoke(this, snapshot);
    }

    public void UpdateCurrents(ActuatorState telemetry)
    {
        ActuatorState snapshot;
        lock (syncRoot)
        {
            state.A1Current = telemetry.A1Current;
            state.A2Current = telemetry.A2Current;
            state.A3Current = telemetry.A3Current;
            state.A4Current = telemetry.A4Current;
            snapshot = state.Clone();
        }

        StateChanged?.Invoke(this, snapshot);
    }
}
