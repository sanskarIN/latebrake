<#
.SYNOPSIS
    Synthetic metadata tests for the Win64/Android candidate pairing gate.
.DESCRIPTION
    All manifests are disposable text, not authentic UAT archives, gameplay
    evidence, or real installed packages. This verifies policy ONLY.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Expect-Rejection {
    param([string]$Case, [scriptblock]$Action)
    $failed = $false
    try { & $Action *> $null } catch { $failed = $true }
    if (-not $failed) { throw "Candidate pair policy accepted bad metadata: $Case" }
    Write-Host "Rejected candidate pair metadata: $Case"
}

$verifier = Join-Path $PSScriptRoot 'Test-CandidatePairIdentity.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ("LatebrakeCandidatePair-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null
try {
    $winPath = Join-Path $temp 'win64-candidate.json'
    $androidPath = Join-Path $temp 'android-candidate.json'
    $win = [ordered]@{
        schema_version = 2
        project = 'LateBrake'
        platform = 'Win64'
        status = 'UNVERIFIED_CANDIDATE_NOT_PLAYTESTED'
        source_commit = ('a' * 40)
        source_tree_tracked_changes = $false
        unreal_engine_version = '5.8'
        unreal_engine_build_sha256 = ('c' * 64)
        starter_map_sha256 = ('b' * 64)
    }
    $android = [ordered]@{
        schema_version = 2
        project = 'LateBrake'
        platform = 'Android'
        status = 'UNVERIFIED_CANDIDATE_NOT_PLAYTESTED'
        source_commit = ('a' * 40)
        source_tree_tracked_changes = $false
        unreal_engine_version = '5.8'
        unreal_engine_build_sha256 = ('c' * 64)
        starter_map_sha256 = ('b' * 64)
    }
    function Write-Fixtures {
        $win | ConvertTo-Json -Depth 4 |
            Set-Content -LiteralPath $winPath -Encoding utf8NoBOM
        $android | ConvertTo-Json -Depth 4 |
            Set-Content -LiteralPath $androidPath -Encoding utf8NoBOM
    }
    function Verify-Pair {
        & $verifier -Win64ManifestJson $winPath -AndroidManifestJson $androidPath
    }
    Write-Fixtures
    Verify-Pair

    $android.starter_map_sha256 = ('c' * 64)
    Write-Fixtures
    Expect-Rejection 'same commit/engine with different circuit maps' { Verify-Pair }
    $android.starter_map_sha256 = ('b' * 64)

    $android.source_commit = ('d' * 40)
    Write-Fixtures
    Expect-Rejection 'different source revisions' { Verify-Pair }
    $android.source_commit = ('a' * 40)

    $android.unreal_engine_version = '5.8.1'
    Write-Fixtures
    Expect-Rejection 'different engine patch versions' { Verify-Pair }
    $android.unreal_engine_version = '5.8'

    # Matching '5.8' is insufficient when different installed UE builds
    # carry different changelist/build metadata. Even valid digests differ.
    $android.unreal_engine_build_sha256 = ('d' * 64)
    Write-Fixtures
    Expect-Rejection 'same 5.8 and circuit but different exact Unreal build' { Verify-Pair }
    $android.unreal_engine_build_sha256 = ('c' * 64)

    $android.unreal_engine_build_sha256 = ('c' * 63)
    Write-Fixtures
    Expect-Rejection 'truncated Android UE build identity' { Verify-Pair }
    $android.unreal_engine_build_sha256 = ('c' * 64)

    $win.unreal_engine_build_sha256 = ''
    Write-Fixtures
    Expect-Rejection 'missing Win64 UE build identity' { Verify-Pair }
    $win.unreal_engine_build_sha256 = ('c' * 64)

    $win.unreal_engine_build_sha256 = ('C' * 64)
    Write-Fixtures
    Expect-Rejection 'uppercase Win64 UE build digest' { Verify-Pair }
    $win.unreal_engine_build_sha256 = ('c' * 64)

    $android.starter_map_sha256 = ('b' * 63)
    Write-Fixtures
    Expect-Rejection 'missing hex character in Android map hash' { Verify-Pair }
    $android.starter_map_sha256 = ('b' * 64)

    $win.starter_map_sha256 = ''
    Write-Fixtures
    Expect-Rejection 'empty Windows map digest' { Verify-Pair }
    $win.starter_map_sha256 = ('b' * 64)

    $android.platform = 'Win64'
    Write-Fixtures
    Expect-Rejection 'mislabeled Android platform' { Verify-Pair }
    $android.platform = 'Android'

    $android.status = 'PLAYTESTED_UNVERIFIED'
    Write-Fixtures
    Expect-Rejection 'forged claim of qualified candidate' { Verify-Pair }
    $android.status = 'UNVERIFIED_CANDIDATE_NOT_PLAYTESTED'

    $win.source_tree_tracked_changes = $true
    Write-Fixtures
    Expect-Rejection 'dirty Windows candidate source metadata' { Verify-Pair }
    $win.source_tree_tracked_changes = $false

    $win.schema_version = 999
    Write-Fixtures
    Expect-Rejection 'unsupported manifest schema' { Verify-Pair }
    $win.schema_version = 2

    $android.project = 'AnotherGame'
    Write-Fixtures
    Expect-Rejection 'wrong project' { Verify-Pair }
    $android.project = 'LateBrake'

    $android.unreal_engine_version = '5.7'
    Write-Fixtures
    Expect-Rejection 'unsupported Unreal version' { Verify-Pair }
    $android.unreal_engine_version = '5.8'

    Write-Fixtures
    Verify-Pair
    Remove-Item -LiteralPath $androidPath
    Expect-Rejection 'missing Android candidate metadata' { Verify-Pair }
    Write-Host 'Candidate pairing policy tests passed: NO real game builds verified.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
