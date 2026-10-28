# ==============================================================================
# H1OS Tools: term_usb_toolbox.ps1
# USB Recovery Terminal, Win32 SE0 Hardware Port Cycler & Serial Monitor
# ==============================================================================

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator);
if (-not $isAdmin) {
    Start-Process powershell.exe -Verb RunAs -ArgumentList ("-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"");
    exit;
}

Add-Type -AssemblyName System.Windows.Forms;
Add-Type -AssemblyName System.Drawing;

if (-not ([System.Management.Automation.PSTypeName]'UsbHubPortCycler').Type) {
    $csharpCode = @"
using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

public class UsbHubPortCycler {
    private const uint GENERIC_WRITE = 0x40000000;
    private const uint FILE_SHARE_READ = 0x00000001;
    private const uint FILE_SHARE_WRITE = 0x00000002;
    private const uint OPEN_EXISTING = 3;
    /* IOCTL_USB_HUB_CYCLE_PORT = CTL_CODE(FILE_DEVICE_USB, 273, METHOD_BUFFERED, FILE_ANY_ACCESS) = 0x220444 */
    private const uint IOCTL_USB_HUB_CYCLE_PORT = 0x220444;

    [StructLayout(LayoutKind.Sequential)]
    public struct USB_CYCLE_PORT_PARAMS {
        public uint ConnectionIndex;
        public uint StatusReturned;
    }

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    private static extern SafeFileHandle CreateFile(
        string lpFileName, uint dwDesiredAccess, uint dwShareMode,
        IntPtr lpSecurityAttributes, uint dwCreationDisposition,
        uint dwFlagsAndAttributes, IntPtr hTemplateFile
    );

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool DeviceIoControl(
        SafeFileHandle hDevice, uint dwIoControlCode,
        ref USB_CYCLE_PORT_PARAMS lpInBuffer, uint nInBufferSize,
        ref USB_CYCLE_PORT_PARAMS lpOutBuffer, uint nOutBufferSize,
        out uint lpBytesReturned, IntPtr lpOverlapped
    );

    public static int CyclePort(string hubPath, uint portIndex) {
        using (SafeFileHandle h = CreateFile(hubPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero)) {
            if (h.IsInvalid) return Marshal.GetLastWin32Error();
            USB_CYCLE_PORT_PARAMS p = new USB_CYCLE_PORT_PARAMS { ConnectionIndex = portIndex, StatusReturned = 0 };
            uint ret;
            bool ok = DeviceIoControl(h, IOCTL_USB_HUB_CYCLE_PORT, ref p, (uint)Marshal.SizeOf(p), ref p, (uint)Marshal.SizeOf(p), out ret, IntPtr.Zero);
            if (!ok) return Marshal.GetLastWin32Error();
            return 0;
        }
    }
}
"@;
    Add-Type -TypeDefinition $csharpCode;
}

$clBg       = [System.Drawing.Color]::FromArgb(10, 12, 10);
$clBox      = [System.Drawing.Color]::FromArgb(18, 22, 18);
$clBorder   = [System.Drawing.Color]::FromArgb(35, 75, 45);
$clGreen    = [System.Drawing.Color]::FromArgb(0, 255, 100);
$clDimGreen = [System.Drawing.Color]::FromArgb(0, 160, 65);
$clAlert    = [System.Drawing.Color]::FromArgb(255, 60, 60);
$clWarn     = [System.Drawing.Color]::FromArgb(255, 185, 0);
$clCyan     = [System.Drawing.Color]::FromArgb(0, 215, 230);

$termFont     = New-Object System.Drawing.Font("Consolas", 8.5);
$termFontBold = New-Object System.Drawing.Font("Consolas", 8.5, [System.Drawing.FontStyle]::Bold);
$titleFont    = New-Object System.Drawing.Font("Consolas", 10, [System.Drawing.FontStyle]::Bold);

$form = New-Object System.Windows.Forms.Form;
$form.Text = "H1 // USB_RECOVERY_TERMINAL";
$form.Size = New-Object System.Drawing.Size(730, 570);
$form.StartPosition = "CenterScreen";
$form.BackColor = $clBg;
$form.ForeColor = $clGreen;
$form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::FixedSingle;
$form.MaximizeBox = $false;
$form.TopMost = $true;

$lblHeader = New-Object System.Windows.Forms.Label;
$lblHeader.Location = New-Object System.Drawing.Point(12, 10);
$lblHeader.Size = New-Object System.Drawing.Size(690, 22);
$lblHeader.Font = $titleFont;
$lblHeader.ForeColor = $clGreen;
$lblHeader.Text = "SYS_ACTIVE // SCANNING USB BUS...";
$form.Controls.Add($lblHeader);

$txtTerminal = New-Object System.Windows.Forms.TextBox;
$txtTerminal.Location = New-Object System.Drawing.Point(12, 36);
$txtTerminal.Size = New-Object System.Drawing.Size(690, 310);
$txtTerminal.Multiline = $true;
$txtTerminal.ScrollBars = [System.Windows.Forms.ScrollBars]::Vertical;
$txtTerminal.ReadOnly = $true;
$txtTerminal.BackColor = $clBox;
$txtTerminal.ForeColor = $clGreen;
$txtTerminal.Font = $termFont;
$txtTerminal.BorderStyle = [System.Windows.Forms.BorderStyle]::FixedSingle;
$form.Controls.Add($txtTerminal);

$script:serialPort = New-Object System.IO.Ports.SerialPort;

function Write-Term([string]$msg, [string]$tag = "INFO") {
    $ts = (Get-Date).ToString("HH:mm:ss.fff");
    $txtTerminal.AppendText("[$ts] [$tag] $msg`r`n");
    $txtTerminal.ScrollToCaret();
}

function Get-TargetDevices {
    $serial = @(Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue | Where-Object {
        ($_.Class -in @('Ports', 'USB')) -and
        ($_.FriendlyName -match 'Espressif|ESP32|CP210|CH34|FTDI|Silicon Labs|USB Serial Device|JTAG|WCH')
    });
    $faulted = @(Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue | Where-Object {
        ($_.Class -in @('Unknown', 'USB', 'Ports') -or [string]::IsNullOrEmpty($_.Class)) -and
        (
            ($_.FriendlyName -match 'Unknown|Unrecognized|Descriptor Request Failed|Port Reset Failed|Failed') -or
            ($_.Status -in @('Error', 'Degraded', 'Unknown')) -or
            ($_.Problem -notin @('CM_PROB_NONE', $null, 0))
        ) -and ($_.InstanceId -match '^USB\\') -and
        (@($serial.InstanceId) -notcontains$_.InstanceId)
    });
    return @{ Serial = $serial; Faulted =$faulted };
}

function Cycle-TargetPort([string]$InstanceId) {
    try {
        $parent = (Get-PnpDeviceProperty -InstanceId $InstanceId -KeyName "DEVPKEY_Device_Parent" -ErrorAction SilentlyContinue).Data;
        
        # 1. Obtain 1-based port index (Address is canonical for USB leaf devices)
        $portIndex = (Get-PnpDeviceProperty -InstanceId $InstanceId -KeyName "DEVPKEY_Device_Address" -ErrorAction SilentlyContinue).Data;
        if (-not $portIndex -or $portIndex -eq 0) {
            $loc = (Get-PnpDeviceProperty -InstanceId $InstanceId -KeyName "DEVPKEY_Device_LocationInfo" -ErrorAction SilentlyContinue).Data;
            if ($loc -match 'Port_#0*([0-9]+)') {
                $portIndex = [uint32]$Matches[1];
            }
        }
        if (-not $portIndex -or $portIndex -eq 0) {
            $paths = (Get-PnpDeviceProperty -InstanceId $InstanceId -KeyName "DEVPKEY_Device_LocationPaths" -ErrorAction SilentlyContinue).Data;
            if ($paths) {
                $paths | ForEach-Object {
                    if ($_ -match 'USB\([0-9]+\)#.*\.USB\(([0-9]+)\)' -or $_ -match '#USB\(([0-9]+)\)') {
                        $portIndex = [uint32]$Matches[1];
                    }
                };
            }
        }

        if ($parent -and $portIndex -gt 0) {
            # 2. Formulate proper Win32 Hub Device Interface symbolic links
            $guid = "{f18a0e88-c30c-11d0-8815-00a0c906bed8}";
            $pHash = $parent.Replace('\', '#');
            $hubPaths = @(
                "\\?\$pHash#$guid",
                "\\.\$pHash#$guid",
                "\\.\$pHash",
                "\\.\$parent"
            );
            
            $err = -1;
            foreach ($hubPath in $hubPaths) {
                $res = [UsbHubPortCycler]::CyclePort($hubPath, [uint32]$portIndex);
                if ($res -eq 0) {
                    Write-Term "IOCTL_USB_HUB_CYCLE_PORT ACK on Port #$portIndex ($hubPath)" "HW";
                    return $true;
                }
                $err = $res;
            }
            
            # 3. Fallback: If hardware port power-cycling is not supported by passive hub, restart PnP node
            Write-Term "IOCTL port cycle rejected by hub hardware (Win32 Err $err); falling back to PnP DevNode reset..." "WARN";
            pnputil /restart-device "$InstanceId" 2>&1 | Out-Null;
            if ($LASTEXITCODE -eq 0) {
                Write-Term "PnP DevNode successfully restarted via pnputil." "PNP";
                return $true;
            } else {
                pnputil /remove-device "$InstanceId" 2>&1 | Out-Null;
                Write-Term "PnP DevNode removed; rescan will renegotiate descriptor." "PNP";
                return $true;
            }
        }
    } catch {
        Write-Term "Cycle-TargetPort exception: $($_.Exception.Message)" "ERR";
        return $false;
    }
    return $false;
}

function Safely-CloseSerial {
    if ($script:serialPort.IsOpen) {
        $script:serialPort.Close();$btnConnect.Text = "[ OPEN MONITOR ]";
        $btnConnect.BackColor =$clBox;
        $btnConnect.ForeColor =$clGreen;
        Write-Term "COM handle released prior to hardware reset." "SER";
    }
}

$lblCom = New-Object System.Windows.Forms.Label;
$lblCom.Location = New-Object System.Drawing.Point(12, 360);
$lblCom.Size = New-Object System.Drawing.Size(42, 23);$lblCom.Text = "PORT:";
$lblCom.Font =$termFontBold;
$lblCom.ForeColor =$clDimGreen;
$form.Controls.Add($lblCom);

$cmbPorts = New-Object System.Windows.Forms.ComboBox;
$cmbPorts.Location = New-Object System.Drawing.Point(58, 357);
$cmbPorts.Size = New-Object System.Drawing.Size(95, 23);$cmbPorts.DropDownStyle = [System.Windows.Forms.ComboBoxStyle]::DropDownList;
$cmbPorts.BackColor =$clBox;
$cmbPorts.ForeColor =$clGreen;
$cmbPorts.FlatStyle = [System.Windows.Forms.FlatStyle]::Flat;
$cmbPorts.Font =$termFont;
$form.Controls.Add($cmbPorts);

$lblBaud = New-Object System.Windows.Forms.Label;
$lblBaud.Location = New-Object System.Drawing.Point(165, 360);
$lblBaud.Size = New-Object System.Drawing.Size(42, 23);$lblBaud.Text = "BAUD:";
$lblBaud.Font =$termFontBold;
$lblBaud.ForeColor =$clDimGreen;
$form.Controls.Add($lblBaud);

$cmbBaud = New-Object System.Windows.Forms.ComboBox;
$cmbBaud.Location = New-Object System.Drawing.Point(212, 357);
$cmbBaud.Size = New-Object System.Drawing.Size(95, 23);$cmbBaud.DropDownStyle = [System.Windows.Forms.ComboBoxStyle]::DropDownList;
$cmbBaud.BackColor =$clBox;
$cmbBaud.ForeColor =$clGreen;
$cmbBaud.FlatStyle = [System.Windows.Forms.FlatStyle]::Flat;
$cmbBaud.Font =$termFont;
@("115200", "921600", "460800", "230400", "74880", "9600") | ForEach-Object { $cmbBaud.Items.Add($_) } | Out-Null;
$cmbBaud.SelectedIndex = 0;
$form.Controls.Add($cmbBaud);

$btnConnect = New-Object System.Windows.Forms.Button;
$btnConnect.Location = New-Object System.Drawing.Point(315, 355);
$btnConnect.Size = New-Object System.Drawing.Size(125, 26);$btnConnect.Text = "[ OPEN MONITOR ]";
$btnConnect.Font =$termFontBold;
$btnConnect.FlatStyle = [System.Windows.Forms.FlatStyle]::Flat;
$btnConnect.FlatAppearance.BorderColor =$clBorder;
$btnConnect.BackColor =$clBox;
$btnConnect.ForeColor =$clGreen;
$btnConnect.Cursor = [System.Windows.Forms.Cursors]::Hand;
$form.Controls.Add($btnConnect);

$btnExportLog = New-Object System.Windows.Forms.Button;
$btnExportLog.Location = New-Object System.Drawing.Point(448, 355);
$btnExportLog.Size = New-Object System.Drawing.Size(130, 26);$btnExportLog.Text = "[ EXPORT LOG ]";
$btnExportLog.Font =$termFontBold;
$btnExportLog.FlatStyle = [System.Windows.Forms.FlatStyle]::Flat;
$btnExportLog.FlatAppearance.BorderColor =$clBorder;
$btnExportLog.BackColor =$clBox;
$btnExportLog.ForeColor =$clCyan;
$btnExportLog.Cursor = [System.Windows.Forms.Cursors]::Hand;
$form.Controls.Add($btnExportLog);

$btnClearLog = New-Object System.Windows.Forms.Button;
$btnClearLog.Location = New-Object System.Drawing.Point(586, 355);
$btnClearLog.Size = New-Object System.Drawing.Size(116, 26);$btnClearLog.Text = "[ CLEAR LOG ]";
$btnClearLog.Font =$termFontBold;
$btnClearLog.FlatStyle = [System.Windows.Forms.FlatStyle]::Flat;
$btnClearLog.FlatAppearance.BorderColor =$clBorder;
$btnClearLog.BackColor =$clBox;
$btnClearLog.ForeColor =$clDimGreen;
$btnClearLog.Cursor = [System.Windows.Forms.Cursors]::Hand;
$btnClearLog.Add_Click({$txtTerminal.Clear() });
$form.Controls.Add($btnClearLog);

function Create-HwButton($text, $x,$y, $w,$h, $color) {$btn = New-Object System.Windows.Forms.Button;
    $btn.Location = New-Object System.Drawing.Point($x, $y);$btn.Size = New-Object System.Drawing.Size($w,$h);
    $btn.Text =$text;
    $btn.Font =$termFontBold;
    $btn.FlatStyle = [System.Windows.Forms.FlatStyle]::Flat;
    $btn.FlatAppearance.BorderColor =$color;
    $btn.BackColor =$clBox;
    $btn.ForeColor =$color;
    $btn.Cursor = [System.Windows.Forms.Cursors]::Hand;
    $form.Controls.Add($btn);
    return $btn;
}

$btnPortCycle = Create-HwButton ">> [1] POWER CYCLE PORT (REPLUG)"       12 400 335 38 $clWarn;
$btnPnpDev    = Create-HwButton ">> [2] RESTART DRIVER (DISABLE/ENABLE)"  367 400 335 38 $clGreen;
$btnPurge     = Create-HwButton ">> [3] UNINSTALL FAULTED DEVICE (CODE 43)" 12 445 335 38 $clAlert;
$btnBusScan   = Create-HwButton ">> [4] SCAN FOR HARDWARE CHANGES"        367 445 335 38 $clDimGreen;

$chkTop = New-Object System.Windows.Forms.CheckBox;
$chkTop.Location = New-Object System.Drawing.Point(15, 495);
$chkTop.Size = New-Object System.Drawing.Size(140, 22);$chkTop.Text = "ALWAYS ON TOP";
$chkTop.Font =$termFont;
$chkTop.ForeColor =$clDimGreen;
$chkTop.Checked =$true;
$chkTop.Add_CheckedChanged({ $form.TopMost =$chkTop.Checked });
$form.Controls.Add($chkTop);

$btnExportLog.Add_Click({$saveDlg = New-Object System.Windows.Forms.SaveFileDialog;
    $saveDlg.Filter = "Log Files (*.log)|*.log|Text Files (*.txt)|*.txt|All Files (*.*)|*.*";
    $saveDlg.FileName = "esp32_boot_log_$(Get-Date -Format 'yyyyMMdd_HHmmss').log";
    $saveDlg.Title = "Export Console Buffer to File";
    if ($saveDlg.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
        try {
            [System.IO.File]::WriteAllText($saveDlg.FileName, $txtTerminal.Text, [System.Text.Encoding]::UTF8);
            Write-Term "Buffer exported to: $($saveDlg.FileName)" "DUMP";
        } catch {
            Write-Term "Failed to export log: $($_.Exception.Message)" "ERR";
        }
    }
});

$btnConnect.Add_Click({
    if ($script:serialPort.IsOpen) {
        Safely-CloseSerial;
    } else {
        if (-not $cmbPorts.SelectedItem) {
            Write-Term "No valid COM port selected." "ERR";
            return;
        }
        try {
            $script:serialPort.PortName = $cmbPorts.SelectedItem.ToString();$script:serialPort.BaudRate = [int]$cmbBaud.SelectedItem.ToString();$script:serialPort.DataBits = 8;
            $script:serialPort.Parity = [System.IO.Ports.Parity]::None;
            $script:serialPort.StopBits = [System.IO.Ports.StopBits]::One;
            $script:serialPort.DtrEnable =$true;
            $script:serialPort.RtsEnable =$false;
            $script:serialPort.Open();$btnConnect.Text = "[ CLOSE MONITOR ]";
            $btnConnect.BackColor =$clBorder;
            $btnConnect.ForeColor = [System.Drawing.Color]::White;
            Write-Term "Listening on $($script:serialPort.PortName) @ $($script:serialPort.BaudRate) baud." "SER";
        } catch {
            Write-Term "Failed to open $($cmbPorts.SelectedItem): $($_.Exception.Message)" "ERR";
        }
    }
});

$btnPortCycle.Add_Click({
    Safely-CloseSerial;
    Write-Term "Executing hardware USB port cycle (IOCTL_USB_HUB_CYCLE_PORT)..." "HW";
    $targets = Get-TargetDevices;
    $all = @($targets.Faulted) + @($targets.Serial);
    if ($all.Count -eq 0) {
        Write-Term "No target USB hardware detected." "WARN";
        return;
    }
    $all | ForEach-Object {
        $res = Cycle-TargetPort -InstanceId$_.InstanceId;
        Write-Term "Cycle port -> $($_.FriendlyName): $(if ($res) {'ACK'} else {'BYPASS/FAIL'})" "HW";
    };
    Start-Sleep -Milliseconds 400;
    pnputil /scan-devices | Out-Null;
});

$btnPnpDev.Add_Click({
    Safely-CloseSerial;
    Write-Term "Cycling PnP DevNode driver states..." "PNP";
    $targets = Get-TargetDevices;
    $all = @($targets.Faulted) + @($targets.Serial);
    if ($all.Count -eq 0) {
        Write-Term "No target nodes found to toggle." "WARN";
        return;
    }
    $all | ForEach-Object {
        try {
            Disable-PnpDevice -InstanceId $_.InstanceId -Confirm:$false -ErrorAction Stop | Out-Null;
            Start-Sleep -Milliseconds 250;
            Enable-PnpDevice -InstanceId $_.InstanceId -Confirm:$false -ErrorAction Stop | Out-Null;
            Write-Term "DevNode restarted: $($_.FriendlyName)" "PNP";
        } catch {
            pnputil /restart-device "$($_.InstanceId)" 2>&1 | Out-Null;
            Write-Term "pnputil fallback used for: $($_.FriendlyName)" "PNP";
        }
    };
});

$btnPurge.Add_Click({
    Safely-CloseSerial;
    Write-Term "Purging un-enumerated/corrupt nodes from PnP manager..." "PURGE";
    $targets = Get-TargetDevices;
    if ($targets.Faulted.Count -eq 0) {
        Write-Term "No faulted nodes currently present." "INFO";
    } else {
        $targets.Faulted | ForEach-Object {
            pnputil /remove-device "$($_.InstanceId)" 2>&1 | Out-Null;
            Write-Term "Purged node: $($_.FriendlyName)" "PURGE";
        };
    }
    pnputil /scan-devices | Out-Null;
    Write-Term "PnP scan issued post-purge." "INFO";
});

$btnBusScan.Add_Click({
    Write-Term "Issuing pnputil /scan-devices..." "PNP";
    pnputil /scan-devices | Out-Null;
    Write-Term "Bus rescan complete." "INFO";
});

$pollTimer = New-Object System.Windows.Forms.Timer;
$pollTimer.Interval = 100;
$script:lastPortList = @();$script:tickCount = 0;

$pollTimer.Add_Tick({
    if ($script:serialPort.IsOpen) {
        try {
            if ($script:serialPort.BytesToRead -gt 0) {
                $incoming =$script:serialPort.ReadExisting();
                $cleaned =$incoming -replace '\x1B\[[0-9;]*[a-zA-Z]', '';
                $txtTerminal.AppendText($cleaned);$txtTerminal.ScrollToCaret();
            }
        } catch {}
    }

    $script:tickCount = ($script:tickCount + 1) % 20;
    if ($script:tickCount -eq 0) {$rawPorts = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object;
        if (($rawPorts -join ',') -ne ($script:lastPortList -join ',')) {
            $script:lastPortList =$rawPorts;
            $selected =$cmbPorts.SelectedItem;
            $cmbPorts.Items.Clear();$rawPorts | ForEach-Object { $cmbPorts.Items.Add($_) | Out-Null };
            if ($selected -and $cmbPorts.Items.Contains($selected)) {
                $cmbPorts.SelectedItem =$selected;
            } elseif ($cmbPorts.Items.Count -gt 0) {$cmbPorts.SelectedIndex = 0;
            }
        }

        $targets = Get-TargetDevices;
        $portStr = if ($rawPorts) {$rawPorts -join ", " } else { "NONE" };
        if ($targets.Faulted.Count -gt 0) {
            $lblHeader.ForeColor =$clAlert;
            $lblHeader.Text = "PORTS: [$portStr] // FAULTED NODES: $($targets.Faulted.Count) (RESET REQUIRED)";
        } else {
            $lblHeader.ForeColor =$clGreen;
            $lblHeader.Text = "SYS_ACTIVE // PORTS: [$portStr] // FAULTS: 0";
        }
    }
});
$pollTimer.Start();

$form.Add_FormClosing({
    $pollTimer.Stop();$pollTimer.Dispose();
    Safely-CloseSerial;
    $script:serialPort.Dispose();
});

Write-Term "H1 Diagnostic Console online. Log export ready." "SYS";
[System.Windows.Forms.Application]::Run($form);
