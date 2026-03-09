param(
    [string]$MapFile = ".\port_map.json"
)

if (-not (Test-Path $MapFile)) {
    Write-Error "Map file not found: $MapFile"
    exit 1
}

$map = Get-Content $MapFile -Raw | ConvertFrom-Json
$ports = @()

foreach ($entry in $map.PSObject.Properties) {
    $apiPort = $entry.Value.api_port
    if ($null -ne $apiPort) {
        $ports += [int]$apiPort
    }
}

$ports = $ports | Sort-Object -Unique

foreach ($port in $ports) {
    $listeners = Get-NetTCPConnection -LocalPort $port -State Listen -ErrorAction SilentlyContinue
    if (-not $listeners) {
        Write-Host "No listener on port $port"
        continue
    }

    $pids = $listeners | Select-Object -ExpandProperty OwningProcess -Unique
    foreach ($pid in $pids) {
        try {
            $proc = Get-Process -Id $pid -ErrorAction Stop
            Stop-Process -Id $pid -Force -ErrorAction Stop
            Write-Host "Killed PID $pid ($($proc.ProcessName)) on port $port"
        }
        catch {
            Write-Warning "Failed to kill PID $pid on port $port : $($_.Exception.Message)"
        }
    }
}
