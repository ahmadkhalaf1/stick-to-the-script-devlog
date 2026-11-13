$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'engine.ps1')

$root = Split-Path $PSScriptRoot
$project = Join-Path $root 'StickToTheScript.uproject'
$output = Join-Path $root 'Saved\Package'

# Development, not Shipping: the jam asks for a development build, and it keeps the console and the log.
# The cook runs without the agent plugins: they pull in GameFeatures, which logs an error that fails the cook.
$cookWithoutAgentPlugins = '-AdditionalCookerOptions=-DisablePlugins=ModelContextProtocol,AllToolsets,LiveCodingToolset'
# The Visual C++ runtime files go next to the game, so it starts on a PC that never had them installed.
# The quotes are single on purpose: UAT fills in $(EngineDir), not PowerShell.
$runtimeNextToGame = '-applocaldirectory=$(EngineDir)/Binaries/ThirdParty/AppLocalDependencies'
& (Join-Path $engine 'Engine\Build\BatchFiles\RunUAT.bat') BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$output" $cookWithoutAgentPlugins $runtimeNextToGame -nodebuginfo -utf8output -unattended
if ($LASTEXITCODE -ne 0) {
    Write-Host 'FAIL: packaging failed. If the Unreal editor is open, close it and run this again. Otherwise the log above says where.'
    exit 1
}

$game = Join-Path $output 'Windows'
$megabytes = [int]((Get-ChildItem $game -Recurse -File | Measure-Object Length -Sum).Sum / 1MB)

if (-not (Get-ChildItem $game -Recurse -Filter 'vcruntime140.dll')) {
    Write-Host 'FAIL: the Visual C++ runtime files are not in the package. On a PC without them the game would not start.'
    exit 1
}

# The plugins that let agents drive the editor must never ship. Every enabled plugin's descriptor is listed in the pak.
$unrealPak = Join-Path $engine 'Engine\Binaries\Win64\UnrealPak.exe'
$packed = Get-ChildItem $game -Recurse -Filter *.pak | ForEach-Object { & $unrealPak $_.FullName -List }
$loose = (Get-ChildItem $game -Recurse).FullName
$leaked = @($packed) + @($loose) | Select-String -Pattern 'ModelContextProtocol|AllToolsets|LiveCodingToolset'
if ($leaked) {
    Write-Host "FAIL: editor-only plugins are in the package ($(@($leaked).Count) entries, first: $(@($leaked)[0]))."
    exit 1
}

# A scene file names its level as plain text, which the cooker cannot follow. A level missing from MapsToCook
# in Config/DefaultGame.ini is silently left out, and the game falls back to the first level.
$cooked = [IO.File]::ReadAllText((Join-Path $root 'Saved\Cooked\Windows\StickToTheScript\AssetRegistry.bin'), [Text.Encoding]::ASCII)
$missing = Get-ChildItem (Join-Path $root 'Content\Scripts') -Filter 'Scene*.json' |
    ForEach-Object { (Get-Content $_.FullName -Raw | ConvertFrom-Json).Stage } |
    Where-Object { $_ -and -not $cooked.Contains($_) }
if ($missing) {
    Write-Host "FAIL: these levels are named by a scene file but are not in the package: $($missing -join ', '). Add them to MapsToCook in Config/DefaultGame.ini."
    exit 1
}

Write-Host "PASS: packaged to $game ($megabytes MB), with no editor-only plugins and every scene's level inside."
