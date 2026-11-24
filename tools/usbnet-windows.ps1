# Windows host side of the lavender USB network (patch 0213, CDC-NCM).
#
# Run in an elevated PowerShell (Run as administrator), phone booted with
# sweetusb=2 and connected:
#   powershell -ExecutionPolicy Bypass -File tools\usbnet-windows.ps1 [-Log out.txt] [-NoListen]
#
# 1. waits for the phone's NCM adapter (UsbNcm, "Sweet iOS (USB NCM)"),
# 2. gives it 192.168.7.1/24 (the phone is 192.168.7.2),
# 3. allows inbound UDP 5140 on that interface only,
# 4. pings the phone and prints the live kernel console it streams to
#    192.168.7.1:5140 every 100 ms (Ctrl+C to stop), optionally saving it.
param(
    [string]$Log = "",
    [switch]$NoListen,
    [int]$TimeoutSeconds = 120
)
$ErrorActionPreference = 'Stop'

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "Run this from an elevated PowerShell (Run as administrator): it sets an IP address and a firewall rule."
}

function Find-PhoneAdapter {
    Get-NetAdapter -IncludeHidden -ErrorAction SilentlyContinue | Where-Object {
        $_.InterfaceDescription -match 'NCM|Sweet' -or $_.DriverDescription -match 'NCM' -or
        $_.PnPDeviceID -like 'USB\VID_1209&PID_0001*'
    } | Select-Object -First 1
}

Write-Host "Waiting for the phone's USB NCM adapter (up to $TimeoutSeconds s)..."
$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$nic = $null
while (-not $nic -and (Get-Date) -lt $deadline) {
    $nic = Find-PhoneAdapter
    if (-not $nic) { Start-Sleep -Seconds 2 }
}
if (-not $nic) {
    Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -like 'USB\VID_1209*' } |
        Format-Table Status, Class, FriendlyName, InstanceId -AutoSize | Out-String | Write-Host
    Write-Error "No NCM adapter. If the device shows with problem code 28, the kernel is still CDC-ECM (sweetusbncm=0) or older than 0213."
}
Write-Host ("Adapter: {0} ({1}), status {2}, ifIndex {3}" -f $nic.Name, $nic.InterfaceDescription, $nic.Status, $nic.ifIndex)

$have = Get-NetIPAddress -InterfaceIndex $nic.ifIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue |
    Where-Object { $_.IPAddress -eq '192.168.7.1' }
if (-not $have) {
    Get-NetIPAddress -InterfaceIndex $nic.ifIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Remove-NetIPAddress -Confirm:$false -ErrorAction SilentlyContinue
    Set-NetIPInterface -InterfaceIndex $nic.ifIndex -Dhcp Disabled -ErrorAction SilentlyContinue
    New-NetIPAddress -InterfaceIndex $nic.ifIndex -IPAddress 192.168.7.1 -PrefixLength 24 | Out-Null
}
Write-Host "Host address 192.168.7.1/24 set on $($nic.Name)."

$rule = 'lavender-ios16 live log (UDP 5140)'
if (-not (Get-NetFirewallRule -DisplayName $rule -ErrorAction SilentlyContinue)) {
    New-NetFirewallRule -DisplayName $rule -Direction Inbound -Protocol UDP -LocalPort 5140 `
        -InterfaceAlias $nic.Name -Action Allow | Out-Null
}

Write-Host "Ping 192.168.7.2:"
Test-Connection -ComputerName 192.168.7.2 -Count 4 -ErrorAction SilentlyContinue |
    Format-Table Address, ResponseTime, Status -AutoSize | Out-String | Write-Host

if ($NoListen) { return }
Write-Host "Live log on UDP 5140 (Ctrl+C to stop)..."
$udp = New-Object System.Net.Sockets.UdpClient(5140)
$ep = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, 0)
$writer = $null
# A fixed live.txt path is the convention: truncate it every run so the file
# always holds exactly this boot instead of a pile of appended sessions.
if ($Log) { $writer = [System.IO.StreamWriter]::new($Log, $false, [System.Text.Encoding]::UTF8) }
try {
    while ($true) {
        $bytes = $udp.Receive([ref]$ep)
        $text = [System.Text.Encoding]::UTF8.GetString($bytes)
        [Console]::Write($text)
        if ($writer) { $writer.Write($text); $writer.Flush() }
    }
} finally {
    $udp.Close()
    if ($writer) { $writer.Close() }
}
