# Dot-source this file. It sets $engine to the Unreal Engine 5.8 folder, or stops with a message.

$engine = $env:UE_5_8_ROOT
if (-not $engine) {
    $manifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path $manifest) {
        $engine = ((Get-Content $manifest -Raw | ConvertFrom-Json).InstallationList |
            Where-Object AppName -eq 'UE_5.8').InstallLocation
    }
}
if (-not $engine) {
    Write-Host 'FAIL: Unreal Engine 5.8 not found. Install it from the Epic launcher or set UE_5_8_ROOT to its folder.'
    exit 1
}
