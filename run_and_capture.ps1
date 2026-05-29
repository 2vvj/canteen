# Kill existing first
Get-Process what_to_eat -ErrorAction SilentlyContinue | Stop-Process -Force

# Start the app
$appPath = "C:\Users\lenovo\canteen\release\what_to_eat.exe"
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.10.1\mingw_64\bin;" + $env:Path
$proc = Start-Process -FilePath $appPath -WorkingDirectory "C:\Users\lenovo\canteen\release" -PassThru
Write-Host "Started PID: $($proc.Id)"

# Wait for window
Start-Sleep -Seconds 4

# Enumerate all windows and find ours
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class WinAPI {
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string cls, string win);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern IntPtr GetDesktopWindow();
    [DllImport("user32.dll")] public static extern IntPtr GetWindowDC(IntPtr h);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    public struct RECT { public int left,top,right,bottom; }
}
"@

# Find window by checking various possible titles
$titles = @("what_to_eat", "干饭", "食堂", "Canteen", "MainWindow", "菜品评价", "Welcome")
foreach ($t in $titles) {
    $h = [WinAPI]::FindWindow([IntPtr]::Zero, $t)
    if ($h -ne [IntPtr]::Zero) {
        Write-Host "Found window with title containing: $t"
        [WinAPI]::ShowWindow($h, 9) # SW_RESTORE
        [WinAPI]::SetForegroundWindow($h)
        Start-Sleep -Seconds 1
        break
    }
}

# Take screenshot
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
# Try to capture just the app if we found it, otherwise full screen
$bitmap = New-Object System.Drawing.Bitmap 800, 600
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.CopyFromScreen(0, 0, 0, 0, @(800,600))
$bitmap.Save("c:\Users\lenovo\canteen\release\screenshot.png")
$graphics.Dispose()
$bitmap.Dispose()

Write-Host "Screenshot captured: $($bitmap.Size)"
