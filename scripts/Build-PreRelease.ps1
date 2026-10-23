<#
.SYNOPSIS
    Build a real Latebrake Win64 or Android candidate using licensed UE 5.8.
.DESCRIPTION
    Unreal BuildCookRun must be installed and a REAL, engine-authored starter
    map must already exist. This script cannot fake playable game binaries.
.EXAMPLE
    pwsh scripts/Build-PreRelease.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8' -Platform Win64 -OutputDirectory 'D:/LatebrakeBuilds/Win64'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EngineRoot,
    [Parameter(Mandatory)][ValidateSet('Win64', 'Android')][string]$Platform,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$ExpectedCommit
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
# Direct Build-PreRelease invocations must also reject source changes
# not represented by the commit used for release QA provenance.
& (Join-Path $PSScriptRoot 'Test-SourceProvenance.ps1') -RepoRoot $repo -ExpectedCommit $ExpectedCommit
if (-not $?) { throw 'Untraceable source checkout blocks UAT packaging.' }
$project = Join-Path $repo 'Latebrake.uproject'
$uat = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
$engineVersionPath = Join-Path $EngineRoot 'Engine/Build/Build.version'
if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
    throw "Project missing: $project"
}
if (-not (Test-Path -LiteralPath $uat -PathType Leaf)) {
    throw "Unreal automation tool not found: $uat. Install licensed UE 5.8."
}
if (-not (Test-Path -LiteralPath $engineVersionPath -PathType Leaf)) {
    throw "Engine Build.version is unavailable: $engineVersionPath"
}
$engineVersion = Get-Content -LiteralPath $engineVersionPath -Raw | ConvertFrom-Json -ErrorAction Stop
if ($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 8) {
    throw "Unreal 5.8 required; found $($engineVersion.MajorVersion).$($engineVersion.MinorVersion)"
}
$descriptor = Get-Content -LiteralPath $project -Raw | ConvertFrom-Json -ErrorAction Stop
if ($descriptor.EngineAssociation -ne '5.8') {
    throw "Project engine association is not 5.8: $($descriptor.EngineAssociation)"
}
$requiredMap = Join-Path $repo 'Content/Maps/LatebrakeCircuit.umap'
if (-not (Test-Path -LiteralPath $requiredMap -PathType Leaf)) {
    throw 'Genuine Content/Maps/LatebrakeCircuit.umap is absent. Run New-StarterMap.ps1 inside Unreal Editor first.'
}
& (Join-Path $PSScriptRoot 'Verify-UnrealMapHeader.ps1') -MapPath $requiredMap
if (-not $?) { throw 'Refusing to cook a file that is not a plausible engine-authored Unreal map.' }
$config = Get-Content -LiteralPath (Join-Path $repo 'Config/DefaultEngine.ini') -Raw
if ($config -notmatch '(?m)^GameDefaultMap=/Game/Maps/LatebrakeCircuit\s*$' -or
    $config -notmatch '(?m)^\+MapsToCook=\(FilePath="/Game/Maps/LatebrakeCircuit"\)\s*$') {
    throw 'GameDefaultMap and MapsToCook must both reference /Game/Maps/LatebrakeCircuit.'
}

$project = (Resolve-Path -LiteralPath $project).Path
# Never mistake an EXE or APK from an earlier packaging attempt for the
# result of this run. Refuse an existing nonempty destination, don't delete it.
if (Test-Path -LiteralPath $OutputDirectory) {
    if (-not (Test-Path -LiteralPath $OutputDirectory -PathType Container)) {
        throw 'OutputDirectory must be a directory, not a file.'
    }
    if (Get-ChildItem -LiteralPath $OutputDirectory -Force | Select-Object -First 1) {
        throw 'OutputDirectory is not empty. Choose a fresh directory for each candidate to prevent stale binaries passing package checks.'
    }
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$out = (Resolve-Path -LiteralPath $OutputDirectory).Path

$uatArguments = @(
    'BuildCookRun',
    "-project=$project",
    '-noP4',
    '-build',
    '-cook',
    '-stage',
    '-pak',
    '-package',
    '-archive',
    '-unattended',
    "-archivedirectory=$out",
    "-platform=$Platform",
    '-clientconfig=Development',
    '-utf8output'
)
if ($Platform -eq 'Win64') {
    $uatArguments += '-prereqs'
}
Write-Host "Running authentic Unreal BuildCookRun for $Platform."
& $uat @uatArguments
if ($LASTEXITCODE -ne 0) {
    throw "Unreal BuildCookRun failed with exit code $LASTEXITCODE"
}

# UAT is long-running; a changed HEAD cannot be relabeled with the
# revision recorded at the beginning of the authorized build.
& (Join-Path $PSScriptRoot 'Test-SourceProvenance.ps1') -RepoRoot $repo -ExpectedCommit $ExpectedCommit
if (-not $?) { throw 'Source HEAD or working tree changed during UAT packaging.' }

# Unreal returning 0 is necessary but not enough. Reject implausible EXEs,
# tiny cooked packages and malformed APKs BEFORE recording a manifest or
# making the output available as an UNVERIFIED CI artifact.
& (Join-Path $PSScriptRoot 'Test-UATPackageStructure.ps1') -Platform $Platform -PackageDirectory $out
if (-not $?) {
    throw 'Actual UAT candidate structural preflight rejected the package.'
}
Write-Host "Candidate $Platform package created in $out."
Write-Host 'This candidate is NOT qualified. Install/playtest before any public release.'
