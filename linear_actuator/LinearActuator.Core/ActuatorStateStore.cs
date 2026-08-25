namespace LinearActuator.Core;

public sealed class ActuatorStateStore
{
    private readonly object syncRoot = new();
    private ActuatorStateBundle bundle = ActuatorStateBundle.CreateDefault();

    public event EventHandler<ActuatorStateBundle>? StateChanged;

    public ActuatorState Snapshot()
    {
        return SnapshotModule(ActuatorConstants.DefaultModuleId);
    }

    public ActuatorState SnapshotModule(string moduleId)
    {
        lock (syncRoot)
        {
            return bundle.Modules.TryGetValue(moduleId, out ActuatorState? state)
                ? state.Clone()
                : ActuatorState.CreateDefault();
        }
    }

    public ActuatorStateBundle SnapshotBundle()
    {
        lock (syncRoot)
        {
            return bundle.Clone();
        }
    }

    public void ReplaceState(ActuatorState nextState)
    {
        ReplaceModule(ActuatorConstants.DefaultModuleId, nextState);
    }

    public void ReplaceModule(string moduleId, ActuatorState nextState)
    {
        ActuatorStateBundle snapshot;
        lock (syncRoot)
        {
            bundle.Modules[moduleId] = nextState.Clone();
            snapshot = bundle.Clone();
        }

        StateChanged?.Invoke(this, snapshot);
    }

    public void ReplaceBundle(ActuatorStateBundle nextBundle)
    {
        ActuatorStateBundle snapshot;
        lock (syncRoot)
        {
            bundle = nextBundle.Clone();
            for (int i = 1; i <= ActuatorConstants.ModuleCount; i++)
            {
                string moduleId = ActuatorConstants.FormatModuleId(i);
                bundle.Modules.TryAdd(moduleId, ActuatorState.CreateDefault());
            }

            snapshot = bundle.Clone();
        }

        StateChanged?.Invoke(this, snapshot);
    }

    public void UpdateCurrents(ActuatorState telemetry)
    {
        UpdateCurrents(ActuatorConstants.DefaultModuleId, telemetry);
    }

    public void UpdateCurrents(string moduleId, ActuatorState telemetry)
    {
        ActuatorStateBundle snapshot;
        lock (syncRoot)
        {
            if (!bundle.Modules.TryGetValue(moduleId, out ActuatorState? state))
            {
                state = ActuatorState.CreateDefault();
                bundle.Modules[moduleId] = state;
            }

            state.A1Current = telemetry.A1Current;
            state.A2Current = telemetry.A2Current;
            state.A3Current = telemetry.A3Current;
            state.A4Current = telemetry.A4Current;
            snapshot = bundle.Clone();
        }

        StateChanged?.Invoke(this, snapshot);
    }
}
