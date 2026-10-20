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

Write-Host 'Stage 1/3: compile editor and catch UHT/C++ errors.'
& (Join-Path $PSScriptRoot 'Build-Editor.ps1') -EngineRoot $EngineRoot
if (-not $?) { throw 'Editor compilation stage failed.' }

Write-Host 'Stage 2/3: create or confirm true Unreal starter level.'
& (Join-Path $PSScriptRoot 'New-StarterMap.ps1') -EngineRoot $EngineRoot
if (-not $?) { throw 'Real Unreal level generation failed.' }

Write-Host "Stage 3/3: cook and package real $Platform candidate."
& (Join-Path $PSScriptRoot 'Build-PreRelease.ps1') -EngineRoot $EngineRoot -Platform $Platform -OutputDirectory $OutputDirectory
if (-not $?) { throw 'BuildCookRun/package stage failed.' }

Write-Host 'Recording traceable source/engine/map and package SHA256 data.'
& (Join-Path $PSScriptRoot 'Write-CandidateManifest.ps1') -Platform $Platform -PackageDirectory $OutputDirectory -EngineRoot $EngineRoot
if (-not $?) { throw 'Candidate fingerprint manifest stage failed.' }

# Verify the complete closed-world archive immediately after writing the
# manifest. Even missing and extra files block the candidate handoff.
$manifest = Join-Path $OutputDirectory 'Latebrake-candidate-manifest.json'
& (Join-Path $PSScriptRoot 'Test-CandidateManifest.ps1') -ManifestJson $manifest -Platform $Platform
if (-not $?) { throw 'Candidate archive checksum verification failed.' }

Write-Host "Candidate outputs: $OutputDirectory"
Write-Host 'NOT QUALIFIED: install and test this package on target hardware before release.'
Write-Host 'Use Verify-PlatformPackage.ps1 with accurate tester evidence for subsequent gating.'
