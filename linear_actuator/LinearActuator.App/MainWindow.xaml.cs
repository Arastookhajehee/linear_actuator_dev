using System.Collections.ObjectModel;
using System.Linq;
using System.Windows;
using System.Windows.Media;
using LinearActuator.Core;
using LinearActuator.Infrastructure;

namespace LinearActuator.App;

public partial class MainWindow : Window
{
    public ObservableCollection<ModuleRow> ModuleRows { get; } = new();
    public ObservableCollection<PortRow> PortRows { get; } = new();

    private readonly ActuatorStateStore stateStore = new();
    private readonly SerialModuleManager serialModuleManager = new();
    private readonly ActuatorApiHost apiHost;
    private bool suppressPortToggleEvents;
    private bool suppressPortLockEvents;
    private bool hasDuplicateError;

    public MainWindow()
    {
        InitializeComponent();

        apiHost = new ActuatorApiHost(stateStore, serialModuleManager);

        DataContext = this;

        stateStore.StateChanged += (_, bundle) => Dispatcher.Invoke(() => DisplayState(bundle));
        serialModuleManager.TelemetryReceived += (_, args) => Dispatcher.Invoke(() =>
        {
            UpdatePortBinaryId(args.ComPort, args.Telemetry);
            if (args.ModuleId is null)
            {
                return;
            }

            UpdateModuleMapping(args.ComPort, args.ModuleId, true);
            stateStore.UpdateCurrents(args.ModuleId, args.Telemetry);
        });
        serialModuleManager.MessageReceived += (_, args) => Dispatcher.Invoke(() =>
        {
            PortRow? row = PortRows.FirstOrDefault(port => port.ComPort.Equals(args.ComPort, StringComparison.OrdinalIgnoreCase));
            if (row is not null)
            {
                row.Status = args.Message;
            }

            SetStatus($"{args.ComPort}: {args.Message}");
        });
        serialModuleManager.MappingChanged += (_, args) => Dispatcher.Invoke(() =>
        {
            if (args.ModuleId is not null)
            {
                UpdateModuleMapping(args.ComPort, args.ModuleId, args.IsMapped);
            }
        });
        serialModuleManager.DuplicateModuleIdDetected += (_, args) => Dispatcher.Invoke(() =>
        {
            MarkDuplicatePort(args.FirstComPort);
            MarkDuplicatePort(args.SecondComPort);
            SetError($"Duplicate Arduino ID {args.ModuleId} detected on {args.FirstComPort} and {args.SecondComPort}. Fix wiring, then click Refresh Ports.");
        });

        Loaded += MainWindow_Loaded;
        Closed += MainWindow_Closed;
        DisplayState(stateStore.SnapshotBundle());
    }

    private void MainWindow_Loaded(object sender, RoutedEventArgs e)
    {
        try
        {
            ModuleRows.Clear();
            foreach (string moduleId in ModuleLayoutOrder())
            {
                ModuleRows.Add(new ModuleRow
                {
                    ModuleId = moduleId,
                    BaudRate = ActuatorConstants.DefaultBaudRate,
                    Status = "Unmapped"
                });
            }

            RefreshPortRows();
            DisplayState(stateStore.SnapshotBundle());
            SetStatus("Ready.");
        }
        catch (Exception ex)
        {
            SetStatus($"Failed to load UI: {ex.Message}");
        }
    }

    private async void StartStopButton_Click(object sender, RoutedEventArgs e)
    {
        StartStopButton.IsEnabled = false;

        try
        {
            if (apiHost.IsRunning)
            {
                await apiHost.StopAsync();
                StartStopButton.Content = "Start";
                SetStatus("Stopped.");
                return;
            }

            await apiHost.StartAsync();
            StartStopButton.Content = "Stop";
            SetStatus($"API running at http://{ActuatorConstants.DefaultApiHost}:{ActuatorConstants.DefaultApiPort}.");
        }
        catch (Exception ex)
        {
            SetStatus(ex.Message);
        }
        finally
        {
            StartStopButton.IsEnabled = true;
        }
    }

    private void GetCurrentButton_Click(object sender, RoutedEventArgs e)
    {
        DisplayState(stateStore.SnapshotBundle());
        SetStatus("Current display refreshed from latest telemetry.");
    }

    private async void SendTargetsButton_Click(object sender, RoutedEventArgs e)
    {
        ActuatorStateBundle snapshot = stateStore.SnapshotBundle();
        foreach (KeyValuePair<string, ActuatorState> module in snapshot.Modules)
        {
            await serialModuleManager.SendTargetsAsync(module.Key, module.Value);
        }

        SetStatus("Targets sent to mapped Arduino modules.");
    }

    private async void RefreshPortsButton_Click(object sender, RoutedEventArgs e)
    {
        serialModuleManager.Stop();
        hasDuplicateError = false;
        MarkAllModulesUnmapped();
        ClearPortRows();
        SetStatus("Cleared detected ports. Scanning for available COM ports...");
        await Task.Delay(500);
        RefreshPortRows();
        SetStatus($"Ports refreshed. Found {PortRows.Count} COM port(s). Toggle detected ports on to map Arduino modules.");
    }

    private void RefreshPortRows()
    {
        ClearPortRows();
        foreach (string portName in SerialPortDiscovery.GetPortNames())
        {
            PortRows.Add(new PortRow { ComPort = portName });
        }
    }

    private void ClearPortRows()
    {
        suppressPortToggleEvents = true;
        try
        {
            PortRows.Clear();
        }
        finally
        {
            suppressPortToggleEvents = false;
        }
    }

    private void MarkAllModulesUnmapped()
    {
        foreach (ModuleRow row in ModuleRows)
        {
            row.MappedComPort = "-";
            row.Status = "Unmapped";
            row.CardBackground = "White";
            row.CardBorderBrush = "#C8CED6";
        }
    }

    private void UpdateModuleMapping(string comPort, string moduleId, bool isMapped)
    {
        PortRow? portRow = PortRows.FirstOrDefault(port => port.ComPort.Equals(comPort, StringComparison.OrdinalIgnoreCase));
        if (portRow is not null)
        {
            portRow.MappedModuleId = isMapped ? moduleId : "-";
            portRow.Status = isMapped
                ? portRow.IsLocked ? $"Locked to {moduleId}" : $"Mapped to {moduleId}"
                : "Connected, waiting for ID";
            portRow.HasError = false;
        }

        ModuleRow? moduleRow = ModuleRows.FirstOrDefault(module => module.ModuleId == moduleId);
        if (moduleRow is null)
        {
            return;
        }

        moduleRow.MappedComPort = isMapped ? comPort : "-";
        moduleRow.Status = isMapped ? "Mapped" : "Unmapped";
        moduleRow.CardBackground = isMapped ? "#E8F7EA" : "White";
        moduleRow.CardBorderBrush = isMapped ? "#36A852" : "#C8CED6";
    }

    private void MarkDuplicatePort(string comPort)
    {
        PortRow? portRow = PortRows.FirstOrDefault(port => port.ComPort.Equals(comPort, StringComparison.OrdinalIgnoreCase));
        if (portRow is not null)
        {
            portRow.Status = "Duplicate ID";
            portRow.HasError = true;
        }
    }

    private void PortToggle_Changed(object sender, RoutedEventArgs e)
    {
        if (suppressPortToggleEvents || sender is not FrameworkElement { DataContext: PortRow row })
        {
            return;
        }

        if (!row.SerialEnabled)
        {
            serialModuleManager.StopPort(row.ComPort);
            row.Status = "Off";
            row.MappedModuleId = "-";
            ResetPortLock(row);
            row.HasError = false;
            SetStatus($"{row.ComPort}: serial disconnected.");
            return;
        }

        row.Status = "Connecting";
        row.HasError = false;
        bool connected = serialModuleManager.StartPort(row.ComPort, ActuatorConstants.DefaultBaudRate);
        if (!connected)
        {
            try
            {
                suppressPortToggleEvents = true;
                row.SerialEnabled = false;
            }
            finally
            {
                suppressPortToggleEvents = false;
            }

            row.Status = "Unavailable";
            row.HasError = true;
        }
    }

    private void PortLock_Changed(object sender, RoutedEventArgs e)
    {
        if (suppressPortLockEvents || sender is not FrameworkElement { DataContext: PortRow row })
        {
            return;
        }

        if (!row.IsLocked)
        {
            serialModuleManager.UnlockPort(row.ComPort);
            row.Status = row.MappedModuleId == "-" ? "Connected, waiting for ID" : $"Mapped to {row.MappedModuleId}";
            SetStatus($"{row.ComPort}: mapping unlocked.");
            return;
        }

        if (row.MappedModuleId == "-" || !serialModuleManager.LockPort(row.ComPort, row.MappedModuleId))
        {
            ResetPortLock(row);
            row.Status = "Map before locking";
            SetStatus($"{row.ComPort}: wait for a mapped module before locking.");
            return;
        }

        row.Status = $"Locked to {row.MappedModuleId}";
        SetStatus($"{row.ComPort}: locked to {row.MappedModuleId}.");
    }

    private void ResetPortLock(PortRow row)
    {
        try
        {
            suppressPortLockEvents = true;
            row.IsLocked = false;
        }
        finally
        {
            suppressPortLockEvents = false;
        }
    }

    private async void MainWindow_Closed(object? sender, EventArgs e)
    {
        serialModuleManager.Dispose();
        await apiHost.DisposeAsync();
    }

    private void DisplayState(ActuatorStateBundle bundle)
    {
        foreach (ModuleRow row in ModuleRows)
        {
            if (!bundle.Modules.TryGetValue(row.ModuleId, out ActuatorState? state))
            {
                continue;
            }

            row.C1T1 = $"{FormatCurrent(state.A1Current)} / {FormatTarget(state.A1Target)}";
            row.C2T2 = $"{FormatCurrent(state.A2Current)} / {FormatTarget(state.A2Target)}";
            row.C3T3 = $"{FormatCurrent(state.A3Current)} / {FormatTarget(state.A3Target)}";
            row.C4T4 = $"{FormatCurrent(state.A4Current)} / {FormatTarget(state.A4Target)}";
        }
    }

    private void UpdatePortBinaryId(string comPort, ActuatorState telemetry)
    {
        PortRow? portRow = PortRows.FirstOrDefault(port => port.ComPort.Equals(comPort, StringComparison.OrdinalIgnoreCase));
        if (portRow is not null)
        {
            portRow.BinaryId = FormatBinaryId(telemetry);
        }
    }

    private void SetStatus(string message)
    {
        if (hasDuplicateError)
        {
            return;
        }

        StatusTextBlock.Foreground = Brushes.Black;
        StatusTextBlock.Text = message;
    }

    private void SetError(string message)
    {
        hasDuplicateError = true;
        StatusTextBlock.Foreground = Brushes.Red;
        StatusTextBlock.Text = message;
    }

    private static string FormatCurrent(double? value) => value?.ToString("0.##") ?? "-";

    private static string FormatTarget(double? value) => value?.ToString() ?? "-";

    private static string FormatBinaryId(ActuatorState state)
    {
        if (state.BinaryIdValue is null)
        {
            return "29 27 25 23\n -  -  -  - (avg -)";
        }

        return $"29 27 25 23\n {state.BinaryIdPin29 ?? 0}  {state.BinaryIdPin27 ?? 0}  {state.BinaryIdPin25 ?? 0}  {state.BinaryIdPin23 ?? 0} (avg {FormatAverageBinaryId(state.BinaryIdAverageValue)})";
    }

    private static string FormatAverageBinaryId(int? value) => value?.ToString() ?? "-";

    private static IEnumerable<string> ModuleLayoutOrder()
    {
        yield return "M06";
        yield return "M07";
        yield return "M08";
        yield return "M09";
        yield return "M10";
        yield return "M01";
        yield return "M02";
        yield return "M03";
        yield return "M04";
        yield return "M05";
    }
}
