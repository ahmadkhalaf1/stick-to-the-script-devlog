$ErrorActionPreference = 'Stop'

$project = Join-Path (Split-Path $PSScriptRoot) 'StickToTheScript.uproject'

# GitHub does not protect main on this plan, so each machine guards its own pushes with Tools/hooks/pre-push.
git -C (Split-Path $PSScriptRoot) config core.hooksPath Tools/hooks

. (Join-Path $PSScriptRoot 'engine.ps1')

# Without -DisableAdaptiveUnity the build tool compiles files that have local changes one by one, and so misses
# two files defining a helper of the same name. A clean checkout and the packaged build merge them, and break.
& (Join-Path $engine 'Engine\Build\BatchFiles\Build.bat') StickToTheScriptEditor Win64 Development "-project=$project" -waitmutex -DisableAdaptiveUnity
if ($LASTEXITCODE -ne 0) {
    Write-Host 'FAIL: the project does not compile. If the Unreal editor is open, close it and run this again.'
    exit 1
}

$log = & (Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') $project '-ExecCmds=Automation RunTests StickToTheScript; Quit' '-TestExit=Automation Test Queue Empty' -unattended -nopause -nosplash -nullrhi -nosound -stdout -FullStdOutLogOutput
$testExit = $LASTEXITCODE
$results = @($log | Select-String 'Test Completed\. Result=\{(\w+)\}.*Path=\{([^}]+)\}')
$failed = @($results | Where-Object { $_.Matches[0].Groups[1].Value -ne 'Success' })
if ($testExit -ne 0 -or $results.Count -eq 0 -or $failed.Count -gt 0) {
    $failed | ForEach-Object { Write-Host "  failed: $($_.Matches[0].Groups[2].Value)" }
    Write-Host "FAIL: the automation tests did not all pass ($($failed.Count) of $($results.Count) failed)."
    exit 1
}
Write-Host "PASS: the project compiles and $($results.Count) automation tests pass."
