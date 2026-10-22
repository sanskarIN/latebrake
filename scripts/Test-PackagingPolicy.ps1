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
    Expect-Rejection 'No cooked candidate: manifest' {
        & (Join-Path $PSScriptRoot 'Write-CandidateManifest.ps1') -EngineRoot $missingEngine -Platform Win64 -PackageDirectory $output
    }

    # Temporary invalid bytes only: do not create or publish a synthetic
    # .umap as a real level. Unreal Editor itself must author such assets.
    $fakeMap = Join-Path $temp 'NOT_A_REAL_LEVEL.umap'
    [System.IO.File]::WriteAllBytes($fakeMap, [byte[]]::new(2048))
    Expect-Rejection 'Renamed arbitrary bytes are not Unreal level data' {
        & (Join-Path $PSScriptRoot 'Verify-UnrealMapHeader.ps1') -MapPath $fakeMap
    }
    [System.IO.File]::WriteAllText($fakeMap, 'version https://git-lfs.github.com/spec/v1')
    Expect-Rejection 'Git LFS pointer is not an Unreal level' {
        & (Join-Path $PSScriptRoot 'Verify-UnrealMapHeader.ps1') -MapPath $fakeMap
    }

    # Neither a missing log nor a fabricated log without our marker is
    # evidence that a real Unreal game started.
    $fakeRuntimeLog = Join-Path $temp 'NOT_A_REAL_ENGINE_RUN.log'
    Expect-Rejection 'Missing real runtime smoke log' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }
    [System.IO.File]::WriteAllText($fakeRuntimeLog, ('NOT A REAL ENGINE LOG. No startup evidence.' * 6))
    Expect-Rejection 'Random text is not Unreal GameMode startup evidence' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }

    # These are parser *rejection* fixtures, never synthetic evidence of a
    # real UE run. The new scenery marker is compulsory even when the old
    # checkpoint marker looks valid.
    $fixtureHead = ('NOT AN UNREAL LOG: PARSER REJECTION FIXTURE ONLY. ' * 4)
    $gateReady = 'LogLatebrakeStartup: Display: LATEBRAKE_STARTUP_READY map=LatebrakeCircuit checkpoints=4/4'
    $sceneReady = 'LogLatebrakeStartup: Display: LATEBRAKE_SCENERY_READY groups=7 instances=93 collision=off'
    $geometryReady = 'LogLatebrakeStartup: Display: LATEBRAKE_GEOMETRY_READY collision=9/9'
    $geometryIncomplete = 'LogLatebrakeStartup: Error: LATEBRAKE_GEOMETRY_INCOMPLETE collision=8/9 attempts=9'
    $geometryWrong = 'LogLatebrakeStartup: Display: LATEBRAKE_GEOMETRY_READY collision=8/9'
    $sceneIncomplete = 'LogLatebrakeStartup: Error: LATEBRAKE_SCENERY_INCOMPLETE groups=6/7 instances=70/93'
    $sceneWrong = 'LogLatebrakeStartup: Display: LATEBRAKE_SCENERY_READY groups=7 instances=92 collision=off'

    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$gateReady`n")
    Expect-Rejection 'Four gates without scenery are not enough for startup smoke' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }
    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$sceneIncomplete`n$gateReady`n")
    Expect-Rejection 'Incomplete scenery cannot pass even with four checkpoints' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }
    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$sceneWrong`n$gateReady`n")
    Expect-Rejection 'Wrong cosmetic instance count cannot pass' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }
    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$sceneReady`n$sceneReady`n$gateReady`n")
    Expect-Rejection 'Repeated scenery markers cannot pass a single-run smoke test' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }
    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$gateReady`n$sceneReady`n")
    Expect-Rejection 'Scenery must be prepared before the final startup marker' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }

    # This separate category is mandatory: genuine tracks need all nine
    # collision-bearing ground/road/barrier Actor creations as well as gates.
    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$sceneReady`n$gateReady`n")
    Expect-Rejection 'Four gates plus scenery without collision ground/roads are insufficient' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }
    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$sceneReady`n$geometryIncomplete`n$gateReady`n")
    Expect-Rejection 'Explicit failed collision block must fail' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }
    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$sceneReady`n$geometryWrong`n$gateReady`n")
    Expect-Rejection 'Wrong collision block count must fail' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }
    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$sceneReady`n$geometryReady`n$geometryReady`n$gateReady`n")
    Expect-Rejection 'Duplicate geometry READY markers must fail single-run audit' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }
    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$sceneReady`n$gateReady`n$geometryReady`n")
    Expect-Rejection 'Geometry marker after ready cannot certify startup' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }
    [System.IO.File]::WriteAllText($fakeRuntimeLog, "$fixtureHead`n$geometryReady`n$sceneReady`n$gateReady`n")
    Expect-Rejection 'Scenery log must precede geometry audit as emitted by GameMode' {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    }

    # Positive parser contract fixture ONLY: passing this is not real
    # engine installation evidence, does not contain a real UE log and is
    # never attached as a qualified game artifact.
    [System.IO.File]::WriteAllText($fakeRuntimeLog,
        "$fixtureHead`n$sceneReady`n$geometryReady`n$gateReady`n")
    & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $fakeRuntimeLog
    if (-not $?) { throw 'Positive synthetic startup marker parser fixture failed.' }
    Write-Host 'Synthetic parser grammar passed only; NO actual Unreal game was launched.'

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
