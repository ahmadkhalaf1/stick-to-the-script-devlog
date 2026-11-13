# Makes the zip that gets uploaded: Tools\release.ps1 -Team <TeamName>. Run Tools\package.ps1 first.
param([Parameter(Mandatory = $true)][string]$Team)
$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot
$game = Join-Path $root 'Saved\Package\Windows'
if (-not (Test-Path (Join-Path $game 'StickToTheScript.exe'))) {
    Write-Host 'FAIL: there is no packaged game. Run Tools\package.ps1 first.'
    exit 1
}
if ($Team -notmatch '^[A-Za-z0-9-]+$') {
    Write-Host 'FAIL: the team name goes into a file name. Use letters, digits and hyphens only.'
    exit 1
}

# The jam asks for TeamName_GameName.zip.
$folder = Join-Path $root 'Saved\Release'
$zip = Join-Path $folder "${Team}_StickToTheScript.zip"
New-Item -ItemType Directory -Force $folder | Out-Null
if (Test-Path $zip) {
    Remove-Item $zip
}

# A packaged game that has been run keeps its progress, settings and logs in its own Saved folder.
# A judge must start with the scenes locked, so that folder stays out of the zip.
$played = (Join-Path $game 'StickToTheScript\Saved') + '\'
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::Open($zip, [IO.Compression.ZipArchiveMode]::Create)
try {
    Get-ChildItem $game -Recurse -File | Where-Object { -not $_.FullName.StartsWith($played) } | ForEach-Object {
        $name = 'StickToTheScript/' + $_.FullName.Substring($game.Length + 1).Replace('\', '/')
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $_.FullName, $name, [IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
}
finally {
    $archive.Dispose()
}

# Checked on the zip itself, not on what was meant to go into it.
$written = [IO.Compression.ZipFile]::OpenRead($zip)
$names = @($written.Entries.FullName)
$written.Dispose()
if ($names -notcontains 'StickToTheScript/StickToTheScript.exe') {
    Write-Host 'FAIL: the game is not in the zip.'
    exit 1
}
if ($names -like 'StickToTheScript/StickToTheScript/Saved/*') {
    Write-Host 'FAIL: the zip holds a Saved folder, so a judge would start with someone else''s progress.'
    exit 1
}
$bytes = (Get-Item $zip).Length
if ($bytes -ge 1GB) {
    Write-Host "FAIL: the zip is $([int]($bytes / 1MB)) MB. The jam takes at most 1 GB without butler."
    exit 1
}

Write-Host "PASS: $zip ($([int]($bytes / 1MB)) MB, $($names.Count) files, no saved progress inside)."
