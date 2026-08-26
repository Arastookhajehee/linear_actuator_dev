using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace LinearActuator.App;

public sealed class PortRow : INotifyPropertyChanged
{
    private bool serialEnabled;
    private string status = "Off";
    private string mappedModuleId = "-";
    private bool hasError;

    public event PropertyChangedEventHandler? PropertyChanged;

    public string ComPort { get; set; } = string.Empty;

    public bool SerialEnabled
    {
        get => serialEnabled;
        set
        {
            serialEnabled = value;
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

    public string MappedModuleId
    {
        get => mappedModuleId;
        set
        {
            mappedModuleId = value;
            OnPropertyChanged();
        }
    }

    public bool HasError
    {
        get => hasError;
        set
        {
            hasError = value;
            OnPropertyChanged();
        }
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
