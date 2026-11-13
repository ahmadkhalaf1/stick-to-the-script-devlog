# Fills in the Windows file chooser that Chrome has just opened, and presses Open.
#   Toolschoose_file.ps1 <file> [<file> ...]
# For an agent that drives the browser: itch.io's upload buttons open a file chooser, which a browser tool cannot see.
# Click the button in the page first, then run this.
# It only acts on a chooser that belongs to Chrome, and only on files that exist.
param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Paths)

foreach ($path in $Paths) { if (-not (Test-Path $path)) { "missing: $path"; exit 1 } }
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
$auto = [System.Windows.Automation.AutomationElement]
$scope = [System.Windows.Automation.TreeScope]
$isDialog = [System.Windows.Automation.PropertyCondition]::new($auto::ClassNameProperty, '#32770')
$chrome = @((Get-Process chrome -ErrorAction SilentlyContinue).Id)

$dialog = $null
$until = (Get-Date).AddSeconds(15)
while (-not $dialog -and (Get-Date) -lt $until) {
    foreach ($window in $auto::RootElement.FindAll($scope::Children, [System.Windows.Automation.Condition]::TrueCondition)) {
        if ($chrome -notcontains $window.Current.ProcessId) { continue }
        # The chooser is either Chrome's own top-level window or a dialog owned by one.
        $dialog = if ($window.Current.ClassName -eq '#32770') { $window } else { $window.FindFirst($scope::Children, $isDialog) }
        if ($dialog) { break }
    }
    if (-not $dialog) { Start-Sleep -Milliseconds 300 }
}
if (-not $dialog) { 'no file chooser from Chrome appeared'; exit 1 }

# The file name box and the Open button have had these two ids in every Windows file chooser for decades.
$byId = { param($id) $dialog.FindAll($scope::Descendants, [System.Windows.Automation.PropertyCondition]::new($auto::AutomationIdProperty, $id)) }
$nameBox = (& $byId '1148') | Where-Object { $_.Current.ClassName -eq 'Edit' } | Select-Object -First 1
$open = (& $byId '1') | Where-Object { $_.Current.ClassName -eq 'Button' } | Select-Object -First 1
if (-not $nameBox -or -not $open) { "chooser found ('$($dialog.Current.Name)') but not its file name box or Open button"; exit 1 }

# These plain controls answer window messages, not the automation patterns: set the text, then press the button.
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class ChooserWindow {
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr window, uint message, IntPtr w, string text);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr w, IntPtr l);
}
"@
$text = if ($Paths.Count -eq 1) { $Paths[0] } else { ($Paths | ForEach-Object { '"' + $_ + '"' }) -join ' ' }
[ChooserWindow]::SendMessage([IntPtr]$nameBox.Current.NativeWindowHandle, 0x000C, [IntPtr]::Zero, $text) | Out-Null
Start-Sleep -Milliseconds 300
[ChooserWindow]::PostMessage([IntPtr]$open.Current.NativeWindowHandle, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
"chose $($Paths.Count) file(s) in '$($dialog.Current.Name)'"
