using System.IO;
using System.Windows;
using LinearActuator.Core;
using LinearActuator.Infrastructure;
using Microsoft.EntityFrameworkCore;

namespace LinearActuator.App;

public partial class MainWindow : Window
{
    private readonly ActuatorStateStore stateStore = new();
    private readonly SerialActuatorConnection serialConnection = new();
    private readonly ActuatorApiHost apiHost;
    private readonly PortMappingRepository portMappingRepository;
    private PortMapping currentMapping = new();

    public MainWindow()
    {
        InitializeComponent();

        LinearActuatorDbContext dbContext = new(new DbContextOptionsBuilder<LinearActuatorDbContext>()
            .UseSqlite($"Data Source={Path.Combine(AppContext.BaseDirectory, "linear-actuator.db")}")
            .Options);

        portMappingRepository = new PortMappingRepository(dbContext);
        apiHost = new ActuatorApiHost(stateStore, serialConnection);

        stateStore.StateChanged += (_, state) => Dispatcher.Invoke(() => DisplayState(state));
        serialConnection.TelemetryReceived += (_, telemetry) => stateStore.UpdateCurrents(telemetry);
        serialConnection.MessageReceived += (_, message) => Dispatcher.Invoke(() => SetStatus(message));

        Loaded += MainWindow_Loaded;
        Closed += MainWindow_Closed;
        DisplayState(stateStore.Snapshot());
    }

    private async void MainWindow_Loaded(object sender, RoutedEventArgs e)
    {
        try
        {
            currentMapping = await portMappingRepository.LoadOrCreateDefaultAsync();
            ComPortTextBox.Text = currentMapping.ComPort;
            ApiPortTextBox.Text = currentMapping.ApiPort.ToString();
            BaudRateTextBox.Text = currentMapping.BaudRate.ToString();
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
                serialConnection.Stop();
                await apiHost.StopAsync();
                StartStopButton.Content = "Start";
                SetStatus("Stopped.");
                return;
            }

            if (!int.TryParse(ApiPortTextBox.Text, out int apiPort) || apiPort <= 0)
            {
                SetStatus("API port must be a positive number.");
                return;
            }

            if (!int.TryParse(BaudRateTextBox.Text, out int baudRate) || baudRate <= 0)
            {
                SetStatus("Baud rate must be a positive number.");
                return;
            }

            currentMapping.ComPort = ComPortTextBox.Text.Trim();
            currentMapping.ApiHost = ActuatorConstants.DefaultApiHost;
            currentMapping.ApiPort = apiPort;
            currentMapping.BaudRate = baudRate;
            await portMappingRepository.SaveAsync(currentMapping);

            string serialStatus = "serial not connected";
            if (!string.IsNullOrWhiteSpace(currentMapping.ComPort))
            {
                try
                {
                    serialConnection.Start(currentMapping.ComPort, currentMapping.BaudRate);
                    serialStatus = $"serial connected on {currentMapping.ComPort}";
                }
                catch (Exception ex)
                {
                    serialStatus = $"serial unavailable: {ex.Message}";
                }
            }

            await apiHost.StartAsync(currentMapping.ApiHost, currentMapping.ApiPort);
            StartStopButton.Content = "Stop";
            SetStatus($"API running at http://{currentMapping.ApiHost}:{currentMapping.ApiPort}; {serialStatus}.");
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
        serialConnection.Dispose();
        await apiHost.DisposeAsync();
    }

    private void DisplayState(ActuatorState state)
    {
        A1CurrentTextBlock.Text = FormatCurrent(state.A1Current);
        A2CurrentTextBlock.Text = FormatCurrent(state.A2Current);
        A3CurrentTextBlock.Text = FormatCurrent(state.A3Current);
        A4CurrentTextBlock.Text = FormatCurrent(state.A4Current);

        A1TargetTextBlock.Text = FormatTarget(state.A1Target);
        A2TargetTextBlock.Text = FormatTarget(state.A2Target);
        A3TargetTextBlock.Text = FormatTarget(state.A3Target);
        A4TargetTextBlock.Text = FormatTarget(state.A4Target);
    }

    private void SetStatus(string message)
    {
        StatusTextBlock.Text = message;
    }

    private static string FormatCurrent(double? value) => value?.ToString("0.##") ?? "-";

    private static string FormatTarget(int? value) => value?.ToString() ?? "-";
}
