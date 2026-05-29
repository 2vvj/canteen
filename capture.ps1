Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Drawing.Imaging;
public class ScreenCapture {
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string cls, string win);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern IntPtr GetDesktopWindow();
    [DllImport("user32.dll")] public static extern IntPtr GetWindowDC(IntPtr h);
    [DllImport("gdi32.dll")] public static extern bool BitBlt(IntPtr hdc, int x, int y, int w, int h, IntPtr src, int sx, int sy, int op);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc proc, int l);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, System.Text.StringBuilder t, int n);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    public delegate bool EnumWindowsProc(IntPtr h, int l);
    public struct RECT { public int left,top,right,bottom; }
}
"@

Write-Host "Enumerating windows..."
$found = $false
$callback = {
    $h = $args[0]
    $sb = New-Object System.Text.StringBuilder 256
    [ScreenCapture]::GetWindowText($h, $sb, 256) | Out-Null
    $t = $sb.ToString()
    if ($t -match "干饭|菜品|what_to_eat|WhatToEat|Canteen|MainWindow") {
        Write-Host "FOUND: '$t'"
        [ScreenCapture]::ShowWindow($h, 9)
        [ScreenCapture]::SetForegroundWindow($h)
        Start-Sleep -Milliseconds 500
        $r = New-Object "ScreenCapture+RECT"
        [ScreenCapture]::GetWindowRect($h, [ref]$r)
        Write-Host "RECT: $($r.left),$($r.top) - $($r.right),$($r.bottom)"
        $w = $r.right - $r.left
        $hgt = $r.bottom - $r.top
        $script:foundWindow = $true
        $script:winRect = $r
    }
    return $true
}
$gch = [Runtime.InteropServices.GCHandle]::Alloc($callback)
try { [ScreenCapture]::EnumWindows($callback, 0) | Out-Null } finally { $gch.Free() }

if (-not $script:foundWindow) {
    Write-Host "No matching window found, taking fullscreen screenshot"
    Add-Type -AssemblyName System.Windows.Forms
    $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen($bounds.X, $bounds.Y, 0, 0, $bounds.Size)
    $bitmap.Save("c:\Users\lenovo\canteen\release\screenshot.png")
    $graphics.Dispose()
    $bitmap.Dispose()
    Write-Host "Full screenshot saved"
} else {
    Write-Host "Capturing app window..."
    Add-Type -AssemblyName System.Windows.Forms
    $bitmap = New-Object System.Drawing.Bitmap ($script:winRect.right - $script:winRect.left), ($script:winRect.bottom - $script:winRect.top)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen($script:winRect.left, $script:winRect.top, 0, 0, $bitmap.Size)
    $bitmap.Save("c:\Users\lenovo\canteen\release\screenshot.png")
    $graphics.Dispose()
    $bitmap.Dispose()
    Write-Host "Window screenshot saved, size: $($bitmap.Width)x$($bitmap.Height)"
}
