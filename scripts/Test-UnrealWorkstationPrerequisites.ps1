<#
.SYNOPSIS
    Read-only prerequisite inspection for a REAL Unreal Engine 5.8 workstation.
.DESCRIPTION
    Checks basic engine/project paths and Git revision before invoking costly
    Editor/UHT/UAT commands. This is NOT authentication of engine binaries,
    a compiler/SDK check, a game build or installed gameplay QA.
    The synthetic CI fixtures contain deliberately fake files and cannot
    establish real Unreal Engine readiness.
.EXAMPLE
    pwsh ./scripts/Test-UnrealWorkstationPrerequisites.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -Platform Win64
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EngineRoot,
    [ValidateSet('Win64','Android')][string]$Platform = 'Win64'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $IsWindows -or $PSVersionTable.PSVersion.Major -lt 7) {
    throw 'Real Unreal 5.8 candidate builds require Windows and PowerShell 7.'
}
if ([string]::IsNullOrWhiteSpace($EngineRoot) -or
    -not (Test-Path -LiteralPath $EngineRoot -PathType Container)) {
    throw 'EngineRoot must point to an existing authorized Unreal Engine 5.8 directory.'
}

$repo = Split-Path -Parent $PSScriptRoot
$descriptorPath = Join-Path $repo 'Latebrake.uproject'
if (-not (Test-Path -LiteralPath $descriptorPath -PathType Leaf)) {
    throw 'The complete Latebrake.uproject source checkout is required.'
}
$descriptor = Get-Content -LiteralPath $descriptorPath -Raw -ErrorAction Stop |
    ConvertFrom-Json -ErrorAction Stop
if ($descriptor.EngineAssociation -cne '5.8') {
    throw 'Latebrake.uproject does not request the supported Unreal Engine 5.8.'
}

$requiredPaths = @(
    'Engine/Build/Build.version',
    'Engine/Build/BatchFiles/Build.bat',
    'Engine/Build/BatchFiles/RunUAT.bat',
    'Engine/Binaries/Win64/UnrealEditor.exe'
)
foreach ($relative in $requiredPaths) {
    if (-not (Test-Path -LiteralPath (Join-Path $EngineRoot $relative) -PathType Leaf)) {
        throw "Missing required Unreal Engine file: $relative"
    }
}
$versionPath = Join-Path $EngineRoot 'Engine/Build/Build.version'
$version = Get-Content -LiteralPath $versionPath -Raw -ErrorAction Stop |
    ConvertFrom-Json -ErrorAction Stop
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 8) {
    throw "Incompatible engine metadata: expected UE 5.8, got $($version.MajorVersion).$($version.MinorVersion)."
}
$engineHash = (Get-FileHash -LiteralPath $versionPath -Algorithm SHA256).Hash.ToLowerInvariant()

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw 'Git is required to identify the exact source revision in the candidate manifest.'
}
$sourceCommit = @(& git -C $repo rev-parse --verify HEAD 2>$null)
if ($LASTEXITCODE -ne 0 -or $sourceCommit.Count -ne 1 -or
    [string]$sourceCommit[0] -cnotmatch '^[a-fA-F0-9]{40}$') {
    throw 'Source checkout must contain a valid 40-character Git commit.'
}

Write-Host "STRUCTURAL UE 5.8 PREREQUISITES PRESENT for $Platform."
Write-Host "Source commit: $($sourceCommit[0])"
Write-Host "Build.version SHA256: $engineHash"
if ($Platform -eq 'Android') {
    if ([string]::IsNullOrWhiteSpace($env:ANDROID_HOME) -and
        [string]::IsNullOrWhiteSpace($env:ANDROID_SDK_ROOT)) {
        Write-Warning 'Android SDK environment hint missing; UE may configure SDK paths separately.'
    }
    if ([string]::IsNullOrWhiteSpace($env:JAVA_HOME)) {
        Write-Warning 'JAVA_HOME not set; Unreal Android JDK configuration needs real verification.'
    }
}
Write-Host 'NOT VERIFIED: engine binaries, MSVC/SDK compatibility, Unreal compilation, cooker, packages or installed gameplay.'
