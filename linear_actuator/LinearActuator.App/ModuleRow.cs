using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace LinearActuator.App;

public sealed class ModuleRow : INotifyPropertyChanged
{
    private bool serialEnabled;
    private string comPort = string.Empty;
    private int baudRate;
    private string status = "Off";
    private string c1T1 = "- / 50";
    private string c2T2 = "- / 50";
    private string c3T3 = "- / 50";
    private string c4T4 = "- / 50";
    private string binaryId = "23 25 27 29: -";
    private string mappedComPort = "-";
    private string cardBackground = "White";
    private string cardBorderBrush = "#C8CED6";

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

    public string C1T1
    {
        get => c1T1;
        set
        {
            c1T1 = value;
            OnPropertyChanged();
        }
    }

    public string C2T2
    {
        get => c2T2;
        set
        {
            c2T2 = value;
            OnPropertyChanged();
        }
    }

    public string C3T3
    {
        get => c3T3;
        set
        {
            c3T3 = value;
            OnPropertyChanged();
        }
    }

    public string C4T4
    {
        get => c4T4;
        set
        {
            c4T4 = value;
            OnPropertyChanged();
        }
    }

    public string BinaryId
    {
        get => binaryId;
        set
        {
            binaryId = value;
            OnPropertyChanged();
        }
    }

    public string MappedComPort
    {
        get => mappedComPort;
        set
        {
            mappedComPort = value;
            OnPropertyChanged();
        }
    }

    public string CardBackground
    {
        get => cardBackground;
        set
        {
            cardBackground = value;
            OnPropertyChanged();
        }
    }

    public string CardBorderBrush
    {
        get => cardBorderBrush;
        set
        {
            cardBorderBrush = value;
            OnPropertyChanged();
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
