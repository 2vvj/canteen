Add-Type -AssemblyName System.Windows.Forms
$allTitles = @()
$screen = [System.Windows.Forms.Screen]::PrimaryScreen

# Use WinAPI to enumerate windows
$code = @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public class WndEnum {
    public delegate bool EnumProc(IntPtr h, int l);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc p, int l);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder t, int n);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern IntPtr GetDC(IntPtr h);
    [DllImport("gdi32.dll")] public static extern bool BitBlt(IntPtr d, int x,int y,int w,int h, IntPtr s,int sx,int sy,int r);
    [DllImport("user32.dll")] public static extern int ReleaseDC(IntPtr h, IntPtr dc);
    public struct RECT { public int l,t,r,b; }
}
'@
Add-Type -TypeDefinition $code

$results = @{}
$callback = {
    $h = $args[0]
    $sb = New-Object System.Text.StringBuilder 256
    [WndEnum]::GetWindowText($h, $sb, 256)
    $t = $sb.ToString().Trim()
    if ($t.Length -gt 0) { $results[$h] = $t }
    return $true
}
[WndEnum]::EnumWindows($callback, 0)

# Find our app window - try multiple possible titles
$target = $null
foreach ($h in $results.Keys) {
    $t = $results[$h]
    if ($t -match "干饭|菜品|评价|档案|what_to_eat|WhatToEat|MainWindow|食堂|Canteen|最终确认|人$") {
        Write-Host "Candidate: $t"
        $target = $h
    }
}

if ($target -ne $null) {
    Write-Host "Found app window, bringing to front..."
    [WndEnum]::ShowWindow($target, 9)  # SW_RESTORE
    Start-Sleep -Milliseconds 300
    [WndEnum]::SetForegroundWindow($target)
    Start-Sleep -Milliseconds 500

    $r = New-Object "WndEnum+RECT"
    [WndEnum]::GetWindowRect($target, [ref]$r)
    $ww = $r.r - $r.l
    $wh = $r.b - $r.t
    Write-Host "Window: ${ww}x${wh} at ($($r.l),$($r.t))"

    # Take screenshot of just the window area
    $bmp = New-Object System.Drawing.Bitmap $ww, $wh
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.l, $r.t, 0, 0, (New-Object System.Drawing.Size($ww, $wh)))
    $bmp.Save("c:\Users\lenovo\canteen\release\screenshot_app.png")
    $g.Dispose()
    $bmp.Dispose()
    Write-Host "Screenshot saved"
} else {
    Write-Host "App window not found. Listing all windows:"
    foreach ($h in $results.Keys) { Write-Host "  '$($results[$h])'" }
}
