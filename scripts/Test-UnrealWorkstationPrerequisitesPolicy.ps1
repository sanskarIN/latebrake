<#
.SYNOPSIS
    Negative, synthetic test fixtures for the read-only Unreal workstation preflight.
.DESCRIPTION
    NO Unreal binary or map is created, compiled, executed or authenticated.
    Only temporary placeholder files and Build.version JSON are used.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$probe = Join-Path $PSScriptRoot 'Test-UnrealWorkstationPrerequisites.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ('LateBrake-UE58-preflight-' + [Guid]::NewGuid().ToString('N'))
$engineRoot = Join-Path $temp 'FakeUnrealEngine'
[void](New-Item -ItemType Directory -Path $temp -Force)

function Assert-Rejected([string]$caseName) {
    $rejected = $false
    try {
        & $probe -EngineRoot $engineRoot -Platform Win64 *> $null
    } catch {
        $rejected = $true
    }
    if (-not $rejected) {
        throw "Invalid synthetic Unreal prerequisite case was accepted: $caseName"
    }
}

try {
    Assert-Rejected 'missing Unreal directory'
    foreach ($relative in @(
        'Engine/Build/Build.version',
        'Engine/Build/BatchFiles/Build.bat',
        'Engine/Build/BatchFiles/RunUAT.bat',
        'Engine/Binaries/Win64/UnrealEditor.exe'
    )) {
        $file = Join-Path $engineRoot $relative
        [void](New-Item -ItemType Directory -Path (Split-Path -Parent $file) -Force)
        Set-Content -LiteralPath $file -Value 'SYNTHETIC PLACEHOLDER - NOT UNREAL' -Encoding utf8
    }

    $versionPath = Join-Path $engineRoot 'Engine/Build/Build.version'
    Set-Content -LiteralPath $versionPath -Value '{"MajorVersion":5,"MinorVersion":7}' -Encoding utf8
    Assert-Rejected 'wrong UE minor version'
    Set-Content -LiteralPath $versionPath -Value '{"MajorVersion":4,"MinorVersion":8}' -Encoding utf8
    Assert-Rejected 'wrong UE major version'
    Set-Content -LiteralPath $versionPath -Value '{malformed-json' -Encoding utf8
    Assert-Rejected 'malformed engine metadata'
    Set-Content -LiteralPath $versionPath -Value '{"MajorVersion":5,"MinorVersion":8}' -Encoding utf8

    foreach ($relative in @(
        'Engine/Build/BatchFiles/Build.bat',
        'Engine/Build/BatchFiles/RunUAT.bat',
        'Engine/Binaries/Win64/UnrealEditor.exe'
    )) {
        $file = Join-Path $engineRoot $relative
        Remove-Item -LiteralPath $file -Force
        Assert-Rejected "missing $relative"
        Set-Content -LiteralPath $file -Value 'SYNTHETIC PLACEHOLDER - NOT UNREAL' -Encoding utf8
    }
    # Passing synthetic paths proves *only* structural checks, not a
    # working Unreal install; never execute any placeholder binary.
    & $probe -EngineRoot $engineRoot -Platform Win64 *> $null
    if (-not $?) { throw 'Valid synthetic structural fixture was rejected.' }
    $candidateFolder = Join-Path $temp 'Candidate'
    if (Test-Path -LiteralPath $candidateFolder) {
        throw 'Read-only prerequisite checks cannot create a candidate.'
    }
    Write-Host 'UE58 prerequisite fixtures: invalid metadata/files rejected; no Unreal runtime executed.'
} finally {
    if (Test-Path -LiteralPath $temp -PathType Container) {
        Remove-Item -LiteralPath $temp -Recurse -Force
    }
}
