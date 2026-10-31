<#
.SYNOPSIS
    Compile actual Unreal Engine 5.8 Editor/UHT target without packaging.
.DESCRIPTION
    Requires installed licensed UE 5.8, PowerShell 7 and C++/Windows SDK.
    Pins source and engine metadata before and after the real Editor build.
    No game package, release, cook or installed gameplay qualification.
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EngineRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $IsWindows -or $PSVersionTable.PSVersion.Major -lt 7) {
    throw 'Real Unreal Editor compilation requires Windows PowerShell 7.'
}
$repo = Split-Path -Parent $PSScriptRoot
& (Join-Path $PSScriptRoot 'Test-UnrealWorkstationPrerequisites.ps1') -EngineRoot $EngineRoot -Platform Win64
if (-not $?) { throw 'Unreal workstation prerequisites failed.' }
& (Join-Path $PSScriptRoot 'Test-SourceProvenance.ps1') -RepoRoot $repo
if (-not $?) { throw 'Untraceable checkout: refusing Editor compilation.' }

$revisions = @(& git -C $repo rev-parse --verify HEAD)
if ($LASTEXITCODE -ne 0 -or $revisions.Count -ne 1 -or
    [string]$revisions[0] -cnotmatch '^[a-fA-F0-9]{40}$') {
    throw 'Could not pin source Git SHA before Editor compilation.'
}
$sourceCommit = [string]$revisions[0]
$versionPath = Join-Path $EngineRoot 'Engine/Build/Build.version'
$engineHash = (Get-FileHash -LiteralPath $versionPath -Algorithm SHA256).Hash.ToLowerInvariant()
& (Join-Path $PSScriptRoot 'Test-EngineBuildSnapshot.ps1') -EngineRoot $EngineRoot -ExpectedSha256 $engineHash
if (-not $?) { throw 'Invalid engine metadata snapshot before compile.' }

Write-Host "Starting real UE Editor/UHT compile from pinned source $sourceCommit"
& (Join-Path $PSScriptRoot 'Build-Editor.ps1') -EngineRoot $EngineRoot
if (-not $?) { throw 'Real Unreal Editor/UHT build failed.' }

& (Join-Path $PSScriptRoot 'Test-SourceProvenance.ps1') -RepoRoot $repo -ExpectedCommit $sourceCommit
if (-not $?) { throw 'Source changed while compiling Unreal Editor.' }
& (Join-Path $PSScriptRoot 'Test-EngineBuildSnapshot.ps1') -EngineRoot $EngineRoot -ExpectedSha256 $engineHash
if (-not $?) { throw 'Engine metadata changed while compiling Unreal Editor.' }
Write-Host 'Actual Unreal Editor/UHT build command completed with original metadata.'
Write-Host 'NOT VERIFIED: game package, cooked map, Win64/Android installs or gameplay.'
