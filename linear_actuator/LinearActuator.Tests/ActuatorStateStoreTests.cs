using LinearActuator.Core;

namespace LinearActuator.Tests;

public sealed class ActuatorStateStoreTests
{
    [Fact]
    public void DefaultState_UsesArchiveTargetDefaults()
    {
        ActuatorState state = ActuatorState.CreateDefault();

        Assert.Null(state.A1Current);
        Assert.Equal(50, state.A1Target);
        Assert.Null(state.A2Current);
        Assert.Equal(50, state.A2Target);
        Assert.Null(state.A3Current);
        Assert.Equal(50, state.A3Target);
        Assert.Null(state.A4Current);
        Assert.Equal(50, state.A4Target);
    }

    [Fact]
    public void UpdateCurrents_DoesNotOverwriteTargets()
    {
        ActuatorStateStore store = new();
        store.ReplaceState(new ActuatorState
        {
            A1Target = 100,
            A2Target = 200,
            A3Target = 300,
            A4Target = 400
        });

        store.UpdateCurrents(ActuatorConstants.DefaultModuleId, new ActuatorState
        {
            A1Current = 1.5,
            A1Target = 11,
            A2Current = 2.5,
            A2Target = 22,
            A3Current = 3.5,
            A3Target = 33,
            A4Current = 4.5,
            A4Target = 44
        });

        ActuatorState snapshot = store.Snapshot();
        Assert.Equal(1.5, snapshot.A1Current);
        Assert.Equal(100, snapshot.A1Target);
        Assert.Equal(2.5, snapshot.A2Current);
        Assert.Equal(200, snapshot.A2Target);
        Assert.Equal(3.5, snapshot.A3Current);
        Assert.Equal(300, snapshot.A3Target);
        Assert.Equal(4.5, snapshot.A4Current);
        Assert.Equal(400, snapshot.A4Target);
    }

    [Fact]
    public void DefaultBundle_CreatesTenModules()
    {
        ActuatorStateBundle bundle = ActuatorStateBundle.CreateDefault();

        Assert.Equal(10, bundle.Modules.Count);
        Assert.Contains("M01", bundle.Modules.Keys);
        Assert.Contains("M10", bundle.Modules.Keys);
        Assert.All(bundle.Modules.Values, state => Assert.Equal(50, state.A1Target));
    }

    [Fact]
    public void ReplaceBundle_AddsMissingDefaultModules()
    {
        ActuatorStateStore store = new();
        store.ReplaceBundle(new ActuatorStateBundle
        {
            Modules =
            {
                ["M03"] = new ActuatorState { A1Target = 123 }
            }
        });

        ActuatorStateBundle snapshot = store.SnapshotBundle();

        Assert.Equal(10, snapshot.Modules.Count);
        Assert.Equal(123, snapshot.Modules["M03"].A1Target);
        Assert.Equal(50, snapshot.Modules["M01"].A1Target);
    }

    [Fact]
    public void UpdateCurrents_OnlyUpdatesSelectedModule()
    {
        ActuatorStateStore store = new();

        store.UpdateCurrents("M02", new ActuatorState { A1Current = 22 });

        ActuatorStateBundle snapshot = store.SnapshotBundle();
        Assert.Null(snapshot.Modules["M01"].A1Current);
        Assert.Equal(22, snapshot.Modules["M02"].A1Current);
    }
}
