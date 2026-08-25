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
                ModuleRows.Add(new ModuleRow
                {
                    MappingId = mapping.Id,
                    ModuleId = mapping.ModuleId,
                    SerialEnabled = mapping.SerialEnabled,
                    ComPort = mapping.ComPort,
                    BaudRate = mapping.BaudRate,
                    Status = mapping.SerialEnabled ? "Enabled" : "Off"
                });
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
                serialModuleManager.Stop();
                await apiHost.StopAsync();
                StartStopButton.Content = "Start";
                foreach (ModuleRow row in ModuleRows)
                {
                    row.Status = row.SerialEnabled ? "Enabled" : "Off";
                }

                SetStatus("Stopped.");
                return;
            }

            List<PortMapping> mappings = ModuleRows.Select(row => new PortMapping
            {
                Id = row.MappingId,
                ModuleId = row.ModuleId,
                ComPort = row.ComPort.Trim(),
                BaudRate = row.BaudRate,
                SerialEnabled = row.SerialEnabled
            }).ToList();

            PortMapping? invalidBaud = mappings.FirstOrDefault(mapping => mapping.BaudRate <= 0);
            if (invalidBaud is not null)
            {
                SetStatus($"{invalidBaud.ModuleId} baud rate must be a positive number.");
                return;
            }

            await portMappingRepository.SaveAsync(mappings);
            foreach (ModuleRow row in ModuleRows)
            {
                row.Status = row.SerialEnabled ? "Starting" : "Off";
            }

            serialModuleManager.Start(mappings);
            IReadOnlyDictionary<string, bool> statuses = serialModuleManager.ConnectionStatuses;
            foreach (ModuleRow row in ModuleRows)
            {
                row.Status = row.SerialEnabled
                    ? statuses.TryGetValue(row.ModuleId, out bool connected) && connected ? "Connected" : "Unavailable"
                    : "Off";
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

    private void SetStatus(string message)
    {
        StatusTextBlock.Text = message;
    }

    private static string FormatCurrent(double? value) => value?.ToString("0.##") ?? "-";

    private static string FormatTarget(double? value) => value?.ToString() ?? "-";

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
