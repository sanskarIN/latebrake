<#
.SYNOPSIS
    Try to produce real packaged Latebrake game candidates on Windows.
.DESCRIPTION
    Requires a locally licensed Unreal 5.8 engine and installed target SDK.
    Will first compile the Editor, author/check a REAL binary .umap through
    Unreal Editor, and only then run UAT BuildCookRun. No archive is uploaded
    or called playable until a separate installed playtest has succeeded.
.EXAMPLE
    pwsh scripts/Build-PlayableCandidate.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8' -Platform Win64 -OutputDirectory 'D:/LatebrakeCandidate'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EngineRoot,
    [Parameter(Mandatory)][ValidateSet('Win64', 'Android')][string]$Platform,
    [Parameter(Mandatory)][string]$OutputDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $IsWindows) {
    throw 'This pipeline invokes Unreal Build.bat, RunUAT.bat and UnrealEditor.exe; run on Windows.'
}

# Read-only, structural workstation preflight. This is not an Unreal build
# or proof of installed SDK compatibility; Build.bat/UAT remain authoritative.
& (Join-Path $PSScriptRoot 'Test-UnrealWorkstationPrerequisites.ps1') -EngineRoot $EngineRoot -Platform $Platform
if (-not $?) { throw 'Unreal workstation prerequisites were not met.' }

# Fail before a long engine compilation if HEAD does not identify the
# source being compiled. The one untracked map is fingerprinted separately.
& (Join-Path $PSScriptRoot 'Test-SourceProvenance.ps1')
if (-not $?) { throw 'Untraceable source checkout blocks candidate compilation.' }
$repo = Split-Path -Parent $PSScriptRoot
$sourceCommit = @(& git -C $repo rev-parse --verify HEAD)
if ($LASTEXITCODE -ne 0 -or $sourceCommit.Count -ne 1 -or
    $sourceCommit[0] -cnotmatch '^[a-fA-F0-9]{40}$') {
    throw 'Could not pin the starting Git commit before Unreal compilation.'
}
$sourceCommit = [string]$sourceCommit[0]
$engineVersionPath = Join-Path $EngineRoot 'Engine/Build/Build.version'
if (-not (Test-Path -LiteralPath $engineVersionPath -PathType Leaf)) {
    throw 'Cannot fingerprint a missing Unreal Build.version before compilation.'
}
$engineBuildSha256 = (Get-FileHash -LiteralPath $engineVersionPath -Algorithm SHA256).Hash.ToLowerInvariant()
& (Join-Path $PSScriptRoot 'Test-EngineBuildSnapshot.ps1') -EngineRoot $EngineRoot -ExpectedSha256 $engineBuildSha256
if (-not $?) { throw 'Unreal 5.8 Build.version snapshot was invalid before compilation.' }
Write-Host "Pinned Unreal engine Build.version SHA256: $engineBuildSha256"
Write-Host "Pinned candidate source commit: $sourceCommit"

Write-Host 'Stage 1/3: compile editor and catch UHT/C++ errors.'
& (Join-Path $PSScriptRoot 'Build-Editor.ps1') -EngineRoot $EngineRoot
if (-not $?) { throw 'Editor compilation stage failed.' }

Write-Host 'Stage 2/3: create or confirm true Unreal starter level.'
& (Join-Path $PSScriptRoot 'New-StarterMap.ps1') -EngineRoot $EngineRoot
if (-not $?) { throw 'Real Unreal level generation failed.' }
& (Join-Path $PSScriptRoot 'Test-EngineBuildSnapshot.ps1') -EngineRoot $EngineRoot -ExpectedSha256 $engineBuildSha256
if (-not $?) { throw 'Engine version metadata changed during Editor compile or map authoring.' }

# The Editor may have saved an existing tracked map or left a new asset.
# Recheck before cooking so its provenance is not a surprise at upload.
& (Join-Path $PSScriptRoot 'Test-SourceProvenance.ps1') -ExpectedCommit $sourceCommit
if (-not $?) { throw 'Source changed during Editor map authoring; candidate is not traceable.' }
$mapPath = Join-Path $repo 'Content/Maps/LatebrakeCircuit.umap'
$mapSnapshot = (Get-FileHash -LiteralPath $mapPath -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Host "Pinned circuit map SHA256 before cook: $mapSnapshot"

Write-Host "Stage 3/3: cook and package real $Platform candidate."
& (Join-Path $PSScriptRoot 'Build-PreRelease.ps1') -EngineRoot $EngineRoot -Platform $Platform -OutputDirectory $OutputDirectory -ExpectedCommit $sourceCommit -ExpectedMapSha256 $mapSnapshot -ExpectedEngineSha256 $engineBuildSha256
if (-not $?) { throw 'BuildCookRun/package stage failed.' }

Write-Host 'Recording traceable source/engine/map and package SHA256 data.'
& (Join-Path $PSScriptRoot 'Write-CandidateManifest.ps1') -Platform $Platform -PackageDirectory $OutputDirectory -EngineRoot $EngineRoot -ExpectedCommit $sourceCommit -ExpectedMapSha256 $mapSnapshot -ExpectedEngineSha256 $engineBuildSha256
if (-not $?) { throw 'Candidate fingerprint manifest stage failed.' }

# Verify the complete closed-world archive immediately after writing the
# manifest. Even missing and extra files block the candidate handoff.
$manifest = Join-Path $OutputDirectory 'Latebrake-candidate-manifest.json'
& (Join-Path $PSScriptRoot 'Test-CandidateManifest.ps1') -ManifestJson $manifest -Platform $Platform
if (-not $?) { throw 'Candidate archive checksum verification failed.' }

$candidateHash = (Get-FileHash -LiteralPath $manifest -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Host "Candidate outputs: $OutputDirectory"
Write-Host "Exact candidate_manifest_sha256 for eventual human QA: $candidateHash"
Write-Host 'NOT QUALIFIED: install and test this package on target hardware before release.'
Write-Host 'Use Verify-PlatformPackage.ps1 with accurate tester evidence for subsequent gating.'
