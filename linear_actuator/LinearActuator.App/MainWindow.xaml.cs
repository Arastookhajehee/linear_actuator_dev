using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Windows;
using LinearActuator.Core;
using LinearActuator.Infrastructure;
using Microsoft.EntityFrameworkCore;

namespace LinearActuator.App;

public partial class MainWindow : Window
{
    public ObservableCollection<ModuleRow> ModuleRows { get; } = new();

    private readonly ActuatorStateStore stateStore = new();
    private readonly SerialModuleManager serialModuleManager = new();
    private readonly ActuatorApiHost apiHost;
    private readonly PortMappingRepository portMappingRepository;
    private bool suppressSerialToggleEvents;

    public MainWindow()
    {
        InitializeComponent();

        LinearActuatorDbContext dbContext = new(new DbContextOptionsBuilder<LinearActuatorDbContext>()
            .UseSqlite($"Data Source={Path.Combine(AppContext.BaseDirectory, "linear-actuator-modules.db")}")
            .Options);

        portMappingRepository = new PortMappingRepository(dbContext);
        apiHost = new ActuatorApiHost(stateStore, serialModuleManager);

        DataContext = this;

        stateStore.StateChanged += (_, bundle) => Dispatcher.Invoke(() => DisplayState(bundle));
        serialModuleManager.TelemetryReceived += (_, args) => stateStore.UpdateCurrents(args.ModuleId, args.Telemetry);
        serialModuleManager.MessageReceived += (_, args) => Dispatcher.Invoke(() =>
        {
            ModuleRow? row = ModuleRows.FirstOrDefault(module => module.ModuleId == args.ModuleId);
            if (row is not null)
            {
                row.Status = args.Message;
            }

            SetStatus($"{args.ModuleId}: {args.Message}");
        });

        Loaded += MainWindow_Loaded;
        Closed += MainWindow_Closed;
        DisplayState(stateStore.SnapshotBundle());
    }

    private async void MainWindow_Loaded(object sender, RoutedEventArgs e)
    {
        try
        {
            List<PortMapping> mappings = await portMappingRepository.LoadOrCreateDefaultsAsync();
            ModuleRows.Clear();

            foreach (PortMapping mapping in mappings.OrderBy(GetModuleLayoutOrder))
            {
                suppressSerialToggleEvents = true;
                ModuleRows.Add(new ModuleRow
                {
                    MappingId = mapping.Id,
                    ModuleId = mapping.ModuleId,
                    SerialEnabled = false,
                    ComPort = mapping.ComPort,
                    BaudRate = mapping.BaudRate,
                    Status = "Off"
                });
                suppressSerialToggleEvents = false;
            }

            DisplayState(stateStore.SnapshotBundle());
            SetStatus("Ready.");
        }
        catch (Exception ex)
        {
            SetStatus($"Failed to load SQLite settings: {ex.Message}");
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

            List<PortMapping> mappings = CurrentMappings();

            PortMapping? invalidBaud = mappings.FirstOrDefault(mapping => mapping.BaudRate <= 0);
            if (invalidBaud is not null)
            {
                SetStatus($"{invalidBaud.ModuleId} baud rate must be a positive number.");
                return;
            }

            await portMappingRepository.SaveAsync(mappings);
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

    private async void SerialToggle_Changed(object sender, RoutedEventArgs e)
    {
        if (suppressSerialToggleEvents || sender is not FrameworkElement { DataContext: ModuleRow row })
        {
            return;
        }

        PortMapping mapping = ToPortMapping(row);

        if (!row.SerialEnabled)
        {
            serialModuleManager.StopModule(row.ModuleId);
            row.Status = "Off";
            await portMappingRepository.SaveAsync(CurrentMappings());
            SetStatus($"{row.ModuleId}: serial disconnected.");
            return;
        }

        if (string.IsNullOrWhiteSpace(mapping.ComPort))
        {
            row.Status = "COM port required";
            SetStatus($"{row.ModuleId} COM port is required.");
            ResetSerialToggle(row);
            return;
        }

        if (mapping.BaudRate <= 0)
        {
            row.Status = "Invalid baud";
            SetStatus($"{row.ModuleId} baud rate must be a positive number.");
            ResetSerialToggle(row);
            return;
        }

        row.Status = "Starting";
        await portMappingRepository.SaveAsync(CurrentMappings());
        bool connected = serialModuleManager.StartModule(mapping);
        row.Status = connected ? "Connected" : "Unavailable";
        SetStatus(connected
            ? $"{row.ModuleId}: serial connected on {mapping.ComPort}."
            : $"{row.ModuleId}: serial unavailable on {mapping.ComPort}.");
    }

    private void ResetSerialToggle(ModuleRow row)
    {
        suppressSerialToggleEvents = true;
        row.SerialEnabled = false;
        suppressSerialToggleEvents = false;
    }

    private List<PortMapping> CurrentMappings()
    {
        return ModuleRows.Select(ToPortMapping).ToList();
    }

    private static PortMapping ToPortMapping(ModuleRow row)
    {
        return new PortMapping
        {
            Id = row.MappingId,
            ModuleId = row.ModuleId,
            ComPort = row.ComPort.Trim(),
            BaudRate = row.BaudRate,
            SerialEnabled = row.SerialEnabled
        };
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
            row.BinaryId = FormatBinaryId(state);
        }
    }

    private void SetStatus(string message)
    {
        StatusTextBlock.Text = message;
    }

    private static string FormatCurrent(double? value) => value?.ToString("0.##") ?? "-";

    private static string FormatTarget(double? value) => value?.ToString() ?? "-";

    private static string FormatBinaryId(ActuatorState state)
    {
        if (state.BinaryIdValue is null)
        {
            return "23 25 27 29: -";
        }

        return $"23 25 27 29: {state.BinaryIdPin23 ?? 0} {state.BinaryIdPin25 ?? 0} {state.BinaryIdPin27 ?? 0} {state.BinaryIdPin29 ?? 0} ({state.BinaryIdValue})";
    }

    private static int GetModuleLayoutOrder(PortMapping mapping)
    {
        return mapping.ModuleId switch
        {
            "M06" => 1,
            "M07" => 2,
            "M08" => 3,
            "M09" => 4,
            "M10" => 5,
            "M01" => 6,
            "M02" => 7,
            "M03" => 8,
            "M04" => 9,
            "M05" => 10,
            _ => int.MaxValue
        };
    }
}
