<#
.SYNOPSIS
    Offline self-tests for checksum provenance without Unreal or game binaries.
.DESCRIPTION
    Creates a short-lived text payload and synthetic *unverified* manifest
    only to test the verifier. All test data is deleted. Passing these tests
    NEVER means that a real Windows or Android game was built or validated.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Expect-Rejection {
    param([string]$Label, [scriptblock]$Action)
    $rejected = $false
    try { & $Action *> $null } catch { $rejected = $true }
    if (-not $rejected) { throw "Manifest policy accepted invalid fixture: $Label" }
    Write-Host "Rejected invalid manifest fixture: $Label"
}

function Expect-RejectionMatching {
    param([string]$Label, [string]$Pattern, [scriptblock]$Action)
    $message = $null
    try { & $Action *> $null } catch { $message = $_.Exception.Message }
    if ([string]::IsNullOrWhiteSpace($message) -or $message -notmatch $Pattern) {
        throw "Expected QA rejection '$Pattern' for $Label; received '$message'"
    }
    Write-Host "Rejected incomplete QA: $Label"
}

$verifier = Join-Path $PSScriptRoot 'Test-CandidateManifest.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ("LatebrakeManifestFixture-" + [Guid]::NewGuid().ToString('N'))
$qaTemp = Join-Path ([IO.Path]::GetTempPath()) ("LatebrakeQAOutsideArchive-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null
New-Item -ItemType Directory -Path $qaTemp -Force | Out-Null
try {
    $payloadPath = Join-Path $temp 'TEST_ONLY_NOT_A_GAME.bin'
    $manifestPath = Join-Path $temp 'Latebrake-candidate-manifest.json'
    $originalBytes = [Text.Encoding]::UTF8.GetBytes('This is synthetic text, not an Unreal executable.')
    [IO.File]::WriteAllBytes($payloadPath, $originalBytes)
    $actualHash = (Get-FileHash -LiteralPath $payloadPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $state = [ordered]@{
        schema_version = 1
        project = 'LateBrake'
        platform = 'Win64'
        status = 'UNVERIFIED_CANDIDATE_NOT_PLAYTESTED'
        source_commit = ('a' * 40)
        source_tree_tracked_changes = $false
        unreal_engine_version = '5.8'
        starter_map_sha256 = ('b' * 64)
        created_utc = '2000-01-01T00:00:00Z'
        files = @(
            [ordered]@{
                path = 'TEST_ONLY_NOT_A_GAME.bin'
                bytes = [int64]$originalBytes.Length
                sha256 = $actualHash
            }
        )
    }
    function Write-Fixture {
        $state | ConvertTo-Json -Depth 10 |
            Set-Content -LiteralPath $manifestPath -Encoding utf8NoBOM
    }

    Write-Fixture
    & $verifier -ManifestJson $manifestPath -Platform Win64
    if (-not $?) { throw 'Clean synthetic fixture must exercise the integrity checker.' }
    Write-Host 'Clean synthetic manifest checksum logic passed (NOT a game package).'

    [IO.File]::WriteAllBytes($payloadPath,
        [Text.Encoding]::UTF8.GetBytes(('X' * $originalBytes.Length)))
    Expect-Rejection 'same-size payload tampering' {
        & $verifier -ManifestJson $manifestPath -Platform Win64
    }
    [IO.File]::WriteAllBytes($payloadPath, $originalBytes)

    $extra = Join-Path $temp 'UNLISTED_TEST_ONLY.txt'
    [IO.File]::WriteAllText($extra, 'unlisted test file')
    Expect-Rejection 'file not included in manifest' {
        & $verifier -ManifestJson $manifestPath -Platform Win64
    }
    Remove-Item -LiteralPath $extra

    Remove-Item -LiteralPath $payloadPath
    Expect-Rejection 'missing payload despite manifest entry' {
        & $verifier -ManifestJson $manifestPath -Platform Win64
    }
    [IO.File]::WriteAllBytes($payloadPath, $originalBytes)

    $state.files[0].bytes = -1
    Write-Fixture
    Expect-Rejection 'invalid file size' {
        & $verifier -ManifestJson $manifestPath -Platform Win64
    }
    $state.files[0].bytes = [int64]$originalBytes.Length

    $state.files[0].path = '../TEST_ONLY_NOT_A_GAME.bin'
    Write-Fixture
    Expect-Rejection 'path traversal escaping manifest root' {
        & $verifier -ManifestJson $manifestPath -Platform Win64
    }
    $state.files[0].path = 'TEST_ONLY_NOT_A_GAME.bin'

    $state.files = @($state.files[0], $state.files[0])
    Write-Fixture
    Expect-Rejection 'duplicate file path' {
        & $verifier -ManifestJson $manifestPath -Platform Win64
    }
    $state.files = @($state.files[0])

    $state.source_tree_tracked_changes = $true
    Write-Fixture
    Expect-Rejection 'dirty tracked source tree' {
        & $verifier -ManifestJson $manifestPath -Platform Win64
    }
    $state.source_tree_tracked_changes = $false

    $state.status = 'PLAYTESTED_THIS_IS_NOT_SUPPORTED'
    Write-Fixture
    Expect-Rejection 'unsupported claim of verified candidate' {
        & $verifier -ManifestJson $manifestPath -Platform Win64
    }
    $state.status = 'UNVERIFIED_CANDIDATE_NOT_PLAYTESTED'
    Write-Fixture
    Expect-Rejection 'platform mismatch' {
        & $verifier -ManifestJson $manifestPath -Platform Android
    }
    & $verifier -ManifestJson $manifestPath -Platform Win64
    if (-not $?) { throw 'Fixture failed to recover a valid manifest state.' }

    # Windows junctions can be created without the elevated symlink
    # privilege. A legitimate file behind one must NEVER count as
    # a package payload or be hashed from outside the candidate root.
    $outside = Join-Path $qaTemp 'EXTERNAL_TEST_ONLY'
    $junction = Join-Path $temp 'REDIRECTED_TEST_ONLY'
    New-Item -ItemType Directory -Path $outside -Force | Out-Null
    [IO.File]::WriteAllBytes(
        (Join-Path $outside 'TEST_ONLY_NOT_A_GAME.bin'), $originalBytes)
    if ($IsWindows) {
        New-Item -ItemType Junction -Path $junction -Target $outside -ErrorAction Stop | Out-Null
    } else {
        New-Item -ItemType SymbolicLink -Path $junction -Target $outside -ErrorAction Stop | Out-Null
    }
    try {
        $state.files[0].path = 'REDIRECTED_TEST_ONLY/TEST_ONLY_NOT_A_GAME.bin'
        Write-Fixture
        Expect-RejectionMatching 'file through junction outside archive' 'Unsafe symlink, junction or non-directory' {
            & $verifier -ManifestJson $manifestPath -Platform Win64
        }
        $state.files[0].path = 'TEST_ONLY_NOT_A_GAME.bin'
        Write-Fixture
        Expect-RejectionMatching 'unlisted linked directory with otherwise valid manifest' 'symlink/junction directory' {
            & $verifier -ManifestJson $manifestPath -Platform Win64
        }
    } finally {
        $state.files[0].path = 'TEST_ONLY_NOT_A_GAME.bin'
        Write-Fixture
        Remove-Item -LiteralPath $junction -Force -ErrorAction Stop
    }
    & $verifier -ManifestJson $manifestPath -Platform Win64
    if (-not $?) {
        throw 'Manifest must recover after removal of the synthetic junction.'
    }

    # Prove a truthful platform QA checklist is REQUIRED *before* any
    # structural EXE validation can pass. The test archive remains text-only.
    $qaPath = Join-Path $qaTemp 'test-only-qa.json'
    $qa = [ordered]@{
        platform = 'Win64'
        installed_and_playtested = $true
        unreal_version = '5.8.0'
        build_commit = ('a' * 40)
        candidate_manifest_sha256 = ''
        test_hardware = 'TEST ONLY - NOT REAL HARDWARE'
        tester = 'offline regression'
        test_date = '2000-01-01'
        test_notes = 'Synthetic QA fixture: never publish'
        checks = [ordered]@{
            launch = $true
            driving = $false
            camera = $true
            lap_finish = $true
            career = $true
            save_load = $true
            menu_input = $true
            performance = $true
        }
    }
    function Write-QA {
        $qa | ConvertTo-Json -Depth 10 |
            Set-Content -LiteralPath $qaPath -Encoding utf8NoBOM
    }
    Write-QA
    $platformVerifier = Join-Path $PSScriptRoot 'Verify-PlatformPackage.ps1'
    Expect-RejectionMatching 'failed Windows driving QA' 'Required Win64 installed-game QA check not passed: driving' {
        & $platformVerifier -Platform Win64 -PackagePath $temp -ManifestJson $manifestPath -EvidenceJson $qaPath
    }
    $qa.checks.driving = $true
    $qa.build_commit = ('c' * 40)
    Write-QA
    Expect-RejectionMatching 'different tested source revision' 'build_commit does not match' {
        & $platformVerifier -Platform Win64 -PackagePath $temp -ManifestJson $manifestPath -EvidenceJson $qaPath
    }
    $qa.build_commit = ('a' * 40)
    $qa.checks.performance = $false
    Write-QA
    Expect-RejectionMatching 'failed Windows performance QA' 'Required Win64 installed-game QA check not passed: performance' {
        & $platformVerifier -Platform Win64 -PackagePath $temp -ManifestJson $manifestPath -EvidenceJson $qaPath
    }

    Write-Host 'All candidate manifest and QA policy fixtures passed. No usable binaries created.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $qaTemp -Recurse -Force -ErrorAction SilentlyContinue
}
