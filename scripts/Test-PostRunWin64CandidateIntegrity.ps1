<#
.SYNOPSIS
    Recheck the *same* Win64 candidate once a private game run exits.
.DESCRIPTION
    Compare the packaged EXE and manifest with their PRE-LAUNCH SHA256
    fingerprints, then independently verify every listed cooked file and
    reject missing/extra/linked archive entries. Does not execute an EXE,
    authenticate Unreal compilation, or certify gameplay.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ManifestJson,
    [Parameter(Mandatory)][string]$GameExecutable,
    [Parameter(Mandatory)][string]$ExpectedManifestSha256,
    [Parameter(Mandatory)][string]$ExpectedExecutableSha256
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

foreach ($namedDigest in @(
    @{ Name = 'ExpectedManifestSha256'; Value = $ExpectedManifestSha256 },
    @{ Name = 'ExpectedExecutableSha256'; Value = $ExpectedExecutableSha256 }
)) {
    if ($namedDigest.Value -notmatch '^[0-9a-fA-F]{64}$') {
        throw "Invalid pre-launch SHA256 digest: $($namedDigest.Name)"
    }
}
if (-not (Test-Path -LiteralPath $ManifestJson -PathType Leaf) -or
    -not (Test-Path -LiteralPath $GameExecutable -PathType Leaf)) {
    throw 'A packaged game EXE or candidate manifest disappeared after runtime.'
}
$manifestItem = Get-Item -LiteralPath $ManifestJson -Force -ErrorAction Stop
$exeItem = Get-Item -LiteralPath $GameExecutable -Force -ErrorAction Stop
if ($manifestItem.Name -cne 'Latebrake-candidate-manifest.json' -or
    $exeItem.Name -ine 'Latebrake.exe') {
    throw 'Unexpected post-run manifest or game executable name.'
}
if (($manifestItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
    ($exeItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'A post-run game or manifest cannot be a symlink or junction.'
}
$root = $manifestItem.Directory.FullName
$relativeExe = [IO.Path]::GetRelativePath($root, $exeItem.FullName)
if ($relativeExe -eq '..' -or $relativeExe.StartsWith('..' + [IO.Path]::DirectorySeparatorChar) -or
    [IO.Path]::IsPathRooted($relativeExe)) {
    throw 'Post-run executable does not belong to its candidate archive.'
}

function Assert-OriginalIdentity {
    $manifestDigest = (Get-FileHash -LiteralPath $manifestItem.FullName -Algorithm SHA256).Hash
    if ($manifestDigest -ine $ExpectedManifestSha256) {
        throw 'Candidate manifest changed after game launch; previous integrity cannot be assumed.'
    }
    $gameDigest = (Get-FileHash -LiteralPath $exeItem.FullName -Algorithm SHA256).Hash
    if ($gameDigest -ine $ExpectedExecutableSha256) {
        throw 'Packaged Latebrake.exe changed after game launch.'
    }
}

Assert-OriginalIdentity
# Rehash the entire original archive, not just the executable. This also
# rejects runtime-generated Saved/Logs, new .ini files and linked folders
# inside a previously sealed cooked directory.
& (Join-Path $PSScriptRoot 'Test-CandidateManifest.ps1') -ManifestJson $manifestItem.FullName -Platform Win64
if (-not $?) {
    throw 'Full candidate manifest integrity check failed after game exit.'
}
# Detect edits racing the first check, including changes to the manifest.
Assert-OriginalIdentity
Write-Host 'POST-RUN ARCHIVE SEALED: original manifest/EXE hashes and entire Win64 file tree match.'
Write-Host 'NOT GAMEPLAY QA: archive integrity does not prove rendering, input, lap progression or device play.'
