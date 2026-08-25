using System.Windows;
using LinearActuator.Core;
using LinearActuator.Infrastructure;

namespace LinearActuator.App;

public partial class App : Application
{
    private SerialModuleManager? apiOnlySerialModuleManager;
    private ActuatorApiHost? apiOnlyHost;

    protected override async void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        if (e.Args.Any(arg => string.Equals(arg, "--api-only", StringComparison.OrdinalIgnoreCase)))
        {
            ShutdownMode = ShutdownMode.OnExplicitShutdown;
            apiOnlySerialModuleManager = new SerialModuleManager();
            apiOnlyHost = new ActuatorApiHost(new ActuatorStateStore(), apiOnlySerialModuleManager);
            await apiOnlyHost.StartAsync();
            return;
        }

        MainWindow = new MainWindow();
        MainWindow.Show();
    }

    protected override async void OnExit(ExitEventArgs e)
    {
        if (apiOnlyHost is not null)
        {
            await apiOnlyHost.DisposeAsync();
        }

        apiOnlySerialModuleManager?.Dispose();
        base.OnExit(e);
    }
}

