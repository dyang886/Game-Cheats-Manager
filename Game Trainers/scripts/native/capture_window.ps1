# Screenshot of a game's window without bringing it to the front - the agent's way of reading the
# HUD during native trainer discovery (see docs/native_trainer_discovery.md). PrintWindow with
# PW_RENDERFULLCONTENT captures DirectX flip-model windows that a screen grab of the desktop misses.
#
#   .\scripts\native\capture_window.ps1 -ProcessName <exe name without .exe> -Out shot.png [-Scale 0.5]

param(
    [Parameter(Mandatory)] [string]$ProcessName,
    [Parameter(Mandatory)] [string]$Out,
    [double]$Scale = 0.5
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public static class CaptureWin32 {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
"@

$process = Get-Process -Name $ProcessName | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $process) { throw "No window found for process $ProcessName" }

# Without DPI awareness a scaled display reports a shrunken client rect and the capture is cropped.
[void][CaptureWin32]::SetProcessDPIAware()
$rect = New-Object CaptureWin32+RECT
[void][CaptureWin32]::GetClientRect($process.MainWindowHandle, [ref]$rect)
$width = $rect.R - $rect.L
$height = $rect.B - $rect.T

$bitmap = New-Object System.Drawing.Bitmap $width, $height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$hdc = $graphics.GetHdc()
$ok = [CaptureWin32]::PrintWindow($process.MainWindowHandle, $hdc, 3) # PW_CLIENTONLY | PW_RENDERFULLCONTENT
$graphics.ReleaseHdc($hdc)
if (-not $ok) { throw "PrintWindow failed for $ProcessName" }

$scaled = New-Object System.Drawing.Bitmap $bitmap, ([int]($width * $Scale)), ([int]($height * $Scale))
$scaled.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
"captured ${width}x${height} -> $Out"
