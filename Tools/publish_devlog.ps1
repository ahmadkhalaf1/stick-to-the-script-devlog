# Publishes the dev log and the sources to the project's public repository: a copy of what is committed here, without
# the game's binary assets and without this repository's history. Run it again whenever the dev log has grown.
#
#   Tools\publish_devlog.ps1                      copies, commits and pushes
#   Tools\publish_devlog.ps1 -Forbidden a,b       also refuses to publish if any of these words is in the copy
#
# The commit is made under the GitHub handle `gh` is signed in with, and GitHub's no-reply address for it, so no
# real name and no email address is published. Needs the `gh` command, signed in.
param(
    [string]$Repository = 'stick-to-the-script-devlog',
    [string[]]$Forbidden = @()
)

# Not 'Stop': git writes its progress and its warnings to the error stream, and this PowerShell would end the script on the first.
$ErrorActionPreference = 'Continue'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (git status --porcelain) { throw 'There are uncommitted changes. What is published is what is committed.' }

# Asked for one at a time: this PowerShell splits an argument that has double quotes inside it.
$handle = gh api user --jq .login
$address = "$(gh api user --jq .id)+$handle@users.noreply.github.com"

$copy = Join-Path $root 'Saved\Release\public'
if (Test-Path $copy) { Remove-Item $copy -Recurse -Force }
git clone --quiet "https://github.com/$handle/$Repository.git" $copy
if (-not (Test-Path (Join-Path $copy '.git'))) { throw "Could not clone $handle/$Repository. Create it first: gh repo create $Repository --public" }
Get-ChildItem $copy -Force | Where-Object { $_.Name -ne '.git' } | Remove-Item -Recurse -Force

# Only what is committed here, and none of the binary assets, the editor's own settings or this repository's attributes.
$files = git ls-files | Where-Object { $_ -notmatch '\.(uasset|umap)$' -and $_ -notmatch '^\.obsidian/' -and $_ -ne '.gitattributes' -and $_ -ne 'Docs/PublicReadme.md' }
foreach ($file in $files) {
    $target = Join-Path $copy $file
    New-Item -ItemType Directory -Force (Split-Path -Parent $target) | Out-Null
    Copy-Item (Join-Path $root $file) $target
}
Copy-Item (Join-Path $root 'Docs\PublicReadme.md') (Join-Path $copy 'README.md')

# The last look before it is public: an address, a path with a user's name in it, or a key must not be in the copy.
$suspect = @('[A-Za-z0-9._-]+@(?!anthropic\.com|users\.noreply\.github\.com)[A-Za-z0-9-]+\.[a-z]{2,}', 'C:\\Users\\', 'fal_sk_') + ($Forbidden | ForEach-Object { [regex]::Escape($_) })
$found = Get-ChildItem $copy -Recurse -File | Where-Object { $_.FullName -notmatch '\\\.git\\' -and $_.Extension -notin '.jpg', '.png' } |
    Select-String -Pattern $suspect -AllMatches | Where-Object { $_.Path -notmatch 'publish_devlog\.ps1$' }
if ($found) {
    $found | Select-Object -First 10 | ForEach-Object { "{0}:{1}: {2}" -f $_.Path.Substring($copy.Length + 1), $_.LineNumber, $_.Line.Trim().Substring(0, [Math]::Min(120, $_.Line.Trim().Length)) }
    throw 'Something that should not be public is in the copy. Nothing was published.'
}

Set-Location $copy
git -c core.autocrlf=false add -A
if (-not (git status --porcelain)) { 'Nothing has changed since the last publication.'; exit 0 }
$state = git -C $root rev-parse --short HEAD
git -c "user.name=$handle" -c "user.email=$address" commit --quiet -m "The project as it stood at $state"
git push --quiet origin HEAD:main
if ($LASTEXITCODE -ne 0) { throw 'The push failed. Nothing was published.' }
$count = (git ls-files).Count
Set-Location $root
"Published $count files to https://github.com/$handle/$Repository"
