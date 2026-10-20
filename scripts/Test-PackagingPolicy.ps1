<#
.SYNOPSIS
    Offline negative regression tests for packaging scripts.
.DESCRIPTION
    Runs without Unreal or real executables. Generated fixtures are intentionally
    invalid and deleted at the end; NEVER publish these as game builds.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Expect-Rejection {
    param([string]$Label, [scriptblock]$Operation)
    $failed = $false
    try {
        & $Operation *> $null
    } catch {
        $failed = $true
    }
    if (-not $failed) { throw "FAIL: invalid packaging case accepted: $Label" }
    Write-Host "Rejected invalid packaging case: $Label"
}

$temp = Join-Path ([System.IO.Path]::GetTempPath()) ("LatebrakePolicy-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null
try {
    $missingEngine = Join-Path $temp 'engine-not-installed'
    $output = Join-Path $temp 'output'
    Expect-Rejection 'No engine: editor build' {
        & (Join-Path $PSScriptRoot 'Build-Editor.ps1') -EngineRoot $missingEngine
    }
    Expect-Rejection 'No engine: map authoring' {
        & (Join-Path $PSScriptRoot 'New-StarterMap.ps1') -EngineRoot $missingEngine
    }
    Expect-Rejection 'No engine: game packaging' {
        & (Join-Path $PSScriptRoot 'Build-PreRelease.ps1') -EngineRoot $missingEngine -Platform Win64 -OutputDirectory $output
    }

    # The fixtures are deliberately too small and are not legitimate packages.
    $windows = Join-Path $temp 'windows'
    New-Item -ItemType Directory -Path $windows -Force | Out-Null
    [System.IO.File]::WriteAllBytes((Join-Path $windows 'Latebrake.exe'), ([byte[]](0x4D,0x5A,0x00)))
    [System.IO.File]::WriteAllBytes((Join-Path $windows 'Game.pak'), ([byte[]](0x00)))
    $evidenceFile = Join-Path $temp 'fixture-evidence.json'
    @{
        platform = 'Win64'
        installed_and_playtested = $true
        unreal_version = '5.8.0'
        build_commit = 'TEST-ONLY-SYNTHETIC-FIXTURE'
        test_hardware = 'NOT A REAL DEVICE'
        tester = 'script'
        test_date = '2000-01-01'
        test_notes = 'Test fixture must fail due to invalid EXE size'
    } | ConvertTo-Json | Set-Content -LiteralPath $evidenceFile
    Expect-Rejection 'Tiny synthetic EXE is rejected' {
        & (Join-Path $PSScriptRoot 'Verify-PlatformPackage.ps1') -Platform Win64 -PackagePath $windows -EvidenceJson $evidenceFile
    }

    $apk = Join-Path $temp 'Latebrake.apk'
    [System.IO.File]::WriteAllBytes($apk, ([byte[]](0x50,0x4B,0x00)))
    Expect-Rejection 'Wrong-platform evidence is rejected' {
        & (Join-Path $PSScriptRoot 'Verify-PlatformPackage.ps1') -Platform Android -PackagePath $apk -EvidenceJson $evidenceFile
    }
    Expect-Rejection 'Missing QA evidence is rejected' {
        & (Join-Path $PSScriptRoot 'Verify-PlatformPackage.ps1') -Platform Win64 -PackagePath $windows -EvidenceJson (Join-Path $temp 'missing.json')
    }
    Write-Host 'All negative packaging checks passed. No usable binaries were created.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
