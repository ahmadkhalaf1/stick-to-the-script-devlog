# Records the packaged game playing itself: the picture of its window, the sound it makes, and its log.
# Tools\cut_video.mjs then joins the three into the gameplay video.
#
#   Tools\record_run.ps1 -Seconds 300
#
# Leave the PC alone while it runs. The picture is taken from the main screen where the game's window is, so anything
# that covers the window is in the video; and nothing else heavy should be running, or the game stutters.
# The sound comes from the game itself (-RecordSound), because not every PC can record what it plays.
# Needs ffmpeg and an NVIDIA card for the encoder. The player's saved progress is put back afterwards.
param([int]$Seconds = 300, [switch]$EvenIfBusy)

# A game recorded next to another program that is working hard comes out stuttering. Better to say so than to record it.
$load = ((Get-Counter '\Processor(_Total)\% Processor Time' -SampleInterval 2 -MaxSamples 2).CounterSamples | Measure-Object CookedValue -Average).Average
if ($load -gt 35 -and -not $EvenIfBusy) { "The processor is at $([int]$load)% before the game has started. Close what is using it, or pass -EvenIfBusy."; exit 1 }

$root = Split-Path -Parent $PSScriptRoot
$game = Join-Path $root 'Saved\Package\Windows'
$out = Join-Path $root 'Saved\Release\video'
$progress = Join-Path $game 'StickToTheScript\Saved\Config\Windows\GameUserSettings.ini'
$sound = Join-Path $game 'StickToTheScript\Saved\Recording\GameSound.wav'
if (Get-Process StickToTheScript* -ErrorAction SilentlyContinue) { 'The game is already running.'; exit 1 }
New-Item -ItemType Directory -Force $out | Out-Null
Remove-Item "$out\run.mp4", "$out\run.log", "$out\run.wav", $sound -ErrorAction SilentlyContinue
if (Test-Path $progress) { Copy-Item $progress "$out\progress.ini" -Force }

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class RecordedWindow {
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr window, ref Spot spot);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr window, IntPtr after, int x, int y, int w, int h, uint flags);
    [StructLayout(LayoutKind.Sequential)] public struct Spot { public int X, Y; }
}
"@

# The sound setting keeps the game audible if its window loses the focus for a moment.
Start-Process "$game\StickToTheScript.exe" -ArgumentList '-windowed', '-resx=1920', '-resy=1080', '-WinX=100', '-WinY=60',
    '-ExecCmds="sts.Autoplay 1"', "-RecordSound=$Seconds", '-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0',
    '-LogCmds="LogImpactSound Verbose, LogTakeEvents Verbose"', "-abslog=$out\run.log"

$window = $null
for ($tries = 0; $tries -lt 60 -and -not $window; $tries++) {
    Start-Sleep -Milliseconds 500
    $window = Get-Process | Where-Object { $_.ProcessName -like 'StickToTheScript*' -and $_.MainWindowHandle -ne 0 } | Select-Object -First 1
}
if (-not $window) { 'The game opened no window.'; exit 1 }
Start-Sleep -Seconds 2
$handle = $window.MainWindowHandle
# On top of everything and in front, so that nothing covers it and it keeps the keyboard.
[RecordedWindow]::SetWindowPos($handle, [IntPtr](-1), 0, 0, 0, 0, 0x0003) | Out-Null
[RecordedWindow]::SetForegroundWindow($handle) | Out-Null
$corner = New-Object RecordedWindow+Spot
[RecordedWindow]::ClientToScreen($handle, [ref]$corner) | Out-Null

$grab = "ddagrab=output_idx=0:framerate=60:draw_mouse=0:offset_x=$($corner.X):offset_y=$($corner.Y):video_size=1920x1080"
$film = Start-Process ffmpeg -ArgumentList '-v', 'error', '-y', '-f', 'lavfi', '-i', $grab, '-t', ($Seconds - 4), '-fps_mode', 'cfr', '-r', '60', '-c:v', 'h264_nvenc', '-preset', 'p5', '-cq', '18', '-g', '30', "`"$out\run.mp4`"" -PassThru -WindowStyle Hidden
"recording $Seconds seconds from $($corner.X),$($corner.Y)"

# The game stops its own sound recording after $Seconds and needs a moment to write the file.
Start-Sleep -Seconds ($Seconds + 8)
if (-not $film.HasExited) { $film.WaitForExit(15000) | Out-Null }
Get-Process StickToTheScript* -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2
if (Test-Path "$out\progress.ini") { Copy-Item "$out\progress.ini" $progress -Force }
if (Test-Path $sound) { Move-Item $sound "$out\run.wav" -Force }
Get-ChildItem "$out\run.*" | ForEach-Object { "{0}  {1:N1} MB" -f $_.Name, ($_.Length / 1MB) }
