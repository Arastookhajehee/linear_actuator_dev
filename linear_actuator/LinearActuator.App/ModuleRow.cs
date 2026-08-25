using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace LinearActuator.App;

public sealed class ModuleRow : INotifyPropertyChanged
{
    private bool serialEnabled;
    private string comPort = string.Empty;
    private int baudRate;
    private string status = "Off";
    private string current = "- / - / - / -";
    private string target = "50 / 50 / 50 / 50";

    public event PropertyChangedEventHandler? PropertyChanged;

    public int MappingId { get; set; }
    public string ModuleId { get; set; } = string.Empty;

    public bool SerialEnabled
    {
        get => serialEnabled;
        set
        {
            serialEnabled = value;
            OnPropertyChanged();
        }
    }

    public string ComPort
    {
        get => comPort;
        set
        {
            comPort = value;
            OnPropertyChanged();
        }
    }

    public int BaudRate
    {
        get => baudRate;
        set
        {
            baudRate = value;
            OnPropertyChanged();
        }
    }

    public string Status
    {
        get => status;
        set
        {
            status = value;
            OnPropertyChanged();
        }
    }

    public string Current
    {
        get => current;
        set
        {
            current = value;
            OnPropertyChanged();
        }
    }

    public string Target
    {
        get => target;
        set
        {
            target = value;
            OnPropertyChanged();
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
