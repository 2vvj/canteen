Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class WinAPI {
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string c, string n);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder t, int n);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, int lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, int lParam);
}
"@

$titles = @()
$callback = {
    $h = $args[0]
    $sb = New-Object System.Text.StringBuilder 256
    [WinAPI]::GetWindowText($h, $sb, 256) | Out-Null
    $t = $sb.ToString().Trim()
    if ($t -ne "") { Write-Host "Window: '$t'" }
    return $true
}
$gch = [Runtime.InteropServices.GCHandle]::Alloc($callback)
try {
    [WinAPI]::EnumWindows($callback, 0) | Out-Null
} finally {
    $gch.Free()
}
