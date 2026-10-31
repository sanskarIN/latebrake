<#
.SYNOPSIS
    Build and optionally launch a genuine private LateBrake Win64 test game.
.DESCRIPTION
    Requires authorized Unreal Engine 5.8, its Windows C++ toolchain and
    PowerShell 7. Never fabricates an EXE or downloads someone else's build.
    Delegates Editor/UHT, real level creation, UAT cooking, structural
    validation and SHA256 manifest generation to Build-PlayableCandidate.ps1.
    Produces a fresh, UNVERIFIED candidate for the owner to test locally;
    does not publish a GitHub Release or claim gameplay qualification.
.EXAMPLE
    pwsh ./scripts/Build-WindowsPlaytest.ps1
.EXAMPLE
    pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:/UE_5.8' -Launch
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$OutputRoot,
    [switch]$Launch
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $IsWindows) {
    throw 'Real Win64 Unreal packaging must run on Windows with PowerShell 7.'
}
if ($PSVersionTable.PSVersion.Major -lt 7) {
    throw 'Install PowerShell 7 (pwsh), then rerun this script.'
}

$repo = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repo 'Latebrake.uproject'
if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
    throw 'Missing Latebrake.uproject: run this script from a complete Git checkout.'
}
if ([string]::IsNullOrWhiteSpace($EngineRoot)) {
    throw 'Supply -EngineRoot pointing at the authorized Unreal Engine 5.8 installation.'
}
# Fail early: do not create test outputs or try fake packaging without the
# installed engine binaries. The underlying pipeline revalidates everything.
foreach ($required in @(
    'Engine/Build/BatchFiles/Build.bat',
    'Engine/Build/BatchFiles/RunUAT.bat',
    'Engine/Binaries/Win64/UnrealEditor.exe',
    'Engine/Build/Build.version'
)) {
    if (-not (Test-Path -LiteralPath (Join-Path $EngineRoot $required) -PathType Leaf)) {
        throw "Unreal Engine 5.8 prerequisite missing: $required in $EngineRoot"
    }
}
$engineMeta = Get-Content -LiteralPath (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw |
    ConvertFrom-Json -ErrorAction Stop
if ($engineMeta.MajorVersion -ne 5 -or $engineMeta.MinorVersion -ne 8) {
    throw 'This personal game candidate requires the actual Unreal Engine 5.8 installation.'
}
& (Join-Path $PSScriptRoot 'Test-UnrealWorkstationPrerequisites.ps1') -EngineRoot $EngineRoot -Platform Win64
if (-not $?) {
    throw 'Private Win64 playtest prerequisite inspection failed.'
}
& (Join-Path $PSScriptRoot 'Test-SourceProvenance.ps1') -RepoRoot $repo
if (-not $?) {
    throw 'A clean, committed Git checkout is mandatory for an attributable personal EXE.'
}

# The default lives under Git-ignored out/ so one run cannot dirty the source.
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $OutputRoot = Join-Path $repo 'out/Win64Playtests'
}
if (Test-Path -LiteralPath $OutputRoot) {
    if (-not (Test-Path -LiteralPath $OutputRoot -PathType Container)) {
        throw 'OutputRoot must be a directory.'
    }
    $rootItem = Get-Item -LiteralPath $OutputRoot -Force
    if (($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'OutputRoot must not be a symlink or directory junction.'
    }
} else {
    [void](New-Item -ItemType Directory -Path $OutputRoot -Force)
}
$root = (Resolve-Path -LiteralPath $OutputRoot).Path
$runName = 'LateBrake-Win64-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') +
    '-' + [Guid]::NewGuid().ToString('N').Substring(0, 8)
$candidate = Join-Path $root $runName
if (Test-Path -LiteralPath $candidate) {
    throw 'Candidate destination already exists. Never reuse stale game binaries.'
}

Write-Host "Building genuine Unreal 5.8 Win64 Development game at $candidate"
Write-Host 'This is a PERSONAL UNVERIFIED build, not a public or stable release.'
& (Join-Path $PSScriptRoot 'Build-PlayableCandidate.ps1') -EngineRoot $EngineRoot -Platform Win64 -OutputDirectory $candidate
if (-not $?) { throw 'Genuine Unreal Win64 packaging did not complete.' }

$manifest = Join-Path $candidate 'Latebrake-candidate-manifest.json'
& (Join-Path $PSScriptRoot 'Test-CandidateManifest.ps1') -ManifestJson $manifest -Platform Win64
if (-not $?) { throw 'Final Win64 candidate checksum verification failed.' }

# Prefer the actual game launcher, never the portable test executable,
# UnrealPrereqSetup, the editor or an arbitrary *Latebrake*.exe.
$gameExecutables = @(Get-ChildItem -LiteralPath $candidate -Recurse -File |
    Where-Object { $_.Name -ieq 'Latebrake.exe' })
if ($gameExecutables.Count -ne 1) {
    throw "Expected one packaged Latebrake.exe; found $($gameExecutables.Count). Inspect genuine UAT output at $candidate."
}
$game = $gameExecutables[0]
$gameHash = (Get-FileHash -LiteralPath $game.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
$manifestHash = (Get-FileHash -LiteralPath $manifest -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Host ''
Write-Host 'WIN64 PERSONAL CANDIDATE READY FOR HANDS-ON TESTING (NOT VERIFIED PLAYABLE)'
Write-Host "Game executable: $($game.FullName)"
Write-Host "EXE SHA256:      $gameHash"
Write-Host "Manifest SHA256: $manifestHash"
Write-Host "Candidate folder: $candidate"
Write-Host 'Keep the complete cooked directory together; do not copy the EXE alone.'
Write-Host 'Test startup, keyboard/gamepad, all gates, pause, restart, saves and physics.'
Write-Host 'Only installed, observed gameplay can qualify this as a playable build.'
if ($Launch) {
    Write-Host 'Starting guarded private test session with dedicated Unreal logs.'
    & (Join-Path $PSScriptRoot 'Run-WindowsPlaytest.ps1') -CandidateDirectory $candidate
    if (-not $?) { throw 'Personal Windows playtest session reported an error.' }
}
