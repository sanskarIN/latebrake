<#
.SYNOPSIS
    Write an UNQUALIFIED build manifest for an actual Unreal packaging output.
.DESCRIPTION
    This fingerprints packaged files and the serialized map; it does not
    certify that the game launches, drives or works on a phone. The build
    pipeline invokes this only after RunUAT exits successfully.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Win64','Android')][string]$Platform,
    [Parameter(Mandatory)][string]$PackageDirectory,
    [Parameter(Mandatory)][string]$EngineRoot,
    [string]$ExpectedCommit,
    [string]$ExpectedMapSha256
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
# Checking only tracked files allowed an untracked source .cpp/.ini to
# affect the cooked game without appearing in manifest source_commit.
$sourceCheck = @{ RepoRoot = $repo }
if ($PSBoundParameters.ContainsKey('ExpectedCommit')) {
    $sourceCheck.ExpectedCommit = $ExpectedCommit
}
& (Join-Path $PSScriptRoot 'Test-SourceProvenance.ps1') @sourceCheck
if (-not $?) { throw 'Untraceable source checkout blocks candidate manifest creation.' }
$versionPath = Join-Path $EngineRoot 'Engine/Build/Build.version'
$projectMap = Join-Path $repo 'Content/Maps/LatebrakeCircuit.umap'
if (-not (Test-Path -LiteralPath $PackageDirectory -PathType Container)) {
    throw 'No actual package directory exists for the candidate manifest.'
}
if (-not (Test-Path -LiteralPath $projectMap -PathType Leaf)) {
    throw 'Cannot fingerprint a missing real Unreal project map.'
}
# The one-command candidate pipeline supplies the map digest it captured
# before UAT. Reject a different map rather than documenting an unrelated
# post-cook version as if its bytes were packaged.
if ($PSBoundParameters.ContainsKey('ExpectedMapSha256')) {
    & (Join-Path $PSScriptRoot 'Test-CandidateMapSnapshot.ps1') -MapPath $projectMap -ExpectedSha256 $ExpectedMapSha256
    if (-not $?) { throw 'Candidate manifest map differs from the cooked map snapshot.' }
}
if (-not (Test-Path -LiteralPath $versionPath -PathType Leaf)) {
    throw 'Cannot identify the Unreal Engine version used to build this candidate.'
}

$engine = Get-Content -LiteralPath $versionPath -Raw | ConvertFrom-Json -ErrorAction Stop
if ($engine.MajorVersion -ne 5 -or $engine.MinorVersion -ne 8) {
    throw 'Only licensed Unreal Engine 5.8 candidates are supported.'
}

$sourceSha = (& git -C $repo rev-parse HEAD 2>$null)
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($sourceSha)) {
    throw 'Git source revision unavailable; do not publish untraceable builds.'
}
$modifiedTrackedFiles = @(& git -C $repo status --porcelain --untracked-files=no 2>$null)
if ($LASTEXITCODE -ne 0) {
    throw 'Could not determine source tree modification status.'
}
if ($modifiedTrackedFiles.Count -gt 0) {
    throw 'Tracked source changes present. Commit source before fingerprinting candidate binaries.'
}

$root = (Resolve-Path -LiteralPath $PackageDirectory).Path
$manifestPath = Join-Path $root 'Latebrake-candidate-manifest.json'
if (Test-Path -LiteralPath $manifestPath) {
    throw 'Candidate manifest already exists; expected a fresh package directory.'
}

$rootItem = Get-Item -LiteralPath $root -Force -ErrorAction Stop
if (($rootItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'Candidate package root cannot be a symlink or reparse point.'
}
# Fingerprinting must not silently omit an empty linked directory or hash a
# regular-looking file reached through a junction outside the UAT archive.
$packageNodes = @(Get-ChildItem -LiteralPath $root -Recurse -Force)
foreach ($node in $packageNodes) {
    if (($node.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Candidate package contains a symlink/junction: $($node.FullName)"
    }
}
$packageFiles = @($packageNodes | Where-Object { -not $_.PSIsContainer } | Sort-Object FullName)
if ($packageFiles.Count -eq 0) {
    throw 'No packaged files found to fingerprint.'
}
if (@($packageFiles | Where-Object { $_.Length -le 0 }).Count -gt 0) {
    throw 'Candidate package contains empty files; the manifest cannot accept them.'
}
$fingerprints = @(
    foreach ($file in $packageFiles) {
        [ordered]@{
            path = [System.IO.Path]::GetRelativePath($root, $file.FullName).Replace('\', '/')
            bytes = [int64]$file.Length
            sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    }
)
$manifest = [ordered]@{
    schema_version = 1
    project = 'LateBrake'
    platform = $Platform
    status = 'UNVERIFIED_CANDIDATE_NOT_PLAYTESTED'
    source_commit = "$sourceSha".Trim()
    source_tree_tracked_changes = ($modifiedTrackedFiles.Count -gt 0)
    unreal_engine_version = "$($engine.MajorVersion).$($engine.MinorVersion)"
    starter_map_sha256 = (Get-FileHash -LiteralPath $projectMap -Algorithm SHA256).Hash.ToLowerInvariant()
    created_utc = [DateTime]::UtcNow.ToString('o')
    files = $fingerprints
}
$manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $manifestPath -Encoding utf8NoBOM
Write-Host "Unverified candidate hashes written to $manifestPath"
Write-Host 'Do not equate this manifest with real engine/driver/device gameplay qualification.'
