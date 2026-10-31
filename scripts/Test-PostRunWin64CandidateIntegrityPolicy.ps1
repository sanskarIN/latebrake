<#
.SYNOPSIS
    Disposable negative tests for the post-run Win64 archive seal.
.DESCRIPTION
    This creates inert synthetic bytes named Latebrake.exe, NOT an
    executable Unreal game. It NEVER launches a process or makes QA claims.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Expect-Rejection {
    param([string]$Name, [scriptblock]$Action)
    $rejected = $false
    try { & $Action *> $null } catch { $rejected = $true }
    if (-not $rejected) {
        throw "Post-run archive seal accepted malformed synthetic fixture: $Name"
    }
    Write-Host "Post-run seal rejected: $Name"
}

$temp = Join-Path ([IO.Path]::GetTempPath()) (
    'LateBrakePostRunSeal-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $temp -Force)
try {
    $verify = Join-Path $PSScriptRoot 'Test-PostRunWin64CandidateIntegrity.ps1'
    $game = Join-Path $temp 'Latebrake.exe'
    $manifest = Join-Path $temp 'Latebrake-candidate-manifest.json'
    $original = [byte[]](0x4D, 0x5A, 0x46, 0x41, 0x4B, 0x45)
    [IO.File]::WriteAllBytes($game, $original)
    $originalExeHash = (Get-FileHash -LiteralPath $game -Algorithm SHA256).Hash.ToLowerInvariant()
    $state = [ordered]@{
        schema_version = 2
        project = 'LateBrake'
        platform = 'Win64'
        status = 'UNVERIFIED_CANDIDATE_NOT_PLAYTESTED'
        source_commit = ('a' * 40)
        source_tree_tracked_changes = $false
        unreal_engine_version = '5.8'
        unreal_engine_build_sha256 = ('b' * 64)
        starter_map_sha256 = ('c' * 64)
        created_utc = '2000-01-01T00:00:00Z'
        files = @([ordered]@{
            path = 'Latebrake.exe'
            bytes = [int64]$original.Length
            sha256 = $originalExeHash
        })
    }
    function Write-Fixture {
        $state | ConvertTo-Json -Depth 10 |
            Set-Content -LiteralPath $manifest -Encoding utf8NoBOM
    }
    Write-Fixture
    $originalManifestHash = (Get-FileHash -LiteralPath $manifest -Algorithm SHA256).Hash.ToLowerInvariant()
    function Invoke-Seal {
        & $verify -ManifestJson $manifest -GameExecutable $game `
            -ExpectedManifestSha256 $originalManifestHash `
            -ExpectedExecutableSha256 $originalExeHash
    }

    Invoke-Seal
    if (-not $?) { throw 'Clean synthetic candidate must pass post-run seal policy.' }

    [IO.File]::WriteAllBytes($game, [byte[]](0x4D, 0x5A, 0x58, 0x58, 0x58, 0x58))
    Expect-Rejection 'same-length executable mutation' { Invoke-Seal }
    [IO.File]::WriteAllBytes($game, $original)
    Invoke-Seal

    $unexpected = Join-Path $temp 'Saved-RuntimeCache.txt'
    [IO.File]::WriteAllText($unexpected, 'unexpected game-owned runtime output')
    Expect-Rejection 'new runtime file inside sealed package' { Invoke-Seal }
    Remove-Item -LiteralPath $unexpected

    $state.created_utc = '2000-01-02T00:00:00Z'
    Write-Fixture
    # A rewritten but internally valid manifest is NOT the original one.
    & (Join-Path $PSScriptRoot 'Test-CandidateManifest.ps1') -ManifestJson $manifest -Platform Win64
    Expect-Rejection 'rewritten self-consistent manifest' { Invoke-Seal }
    $state.created_utc = '2000-01-01T00:00:00Z'
    Write-Fixture
    Invoke-Seal

    Remove-Item -LiteralPath $game
    Expect-Rejection 'missing executable after test run' { Invoke-Seal }
    [IO.File]::WriteAllBytes($game, $original)

    Expect-Rejection 'invalid pre-launch SHA256 input' {
        & $verify -ManifestJson $manifest -GameExecutable $game `
            -ExpectedManifestSha256 'not-a-sha256' `
            -ExpectedExecutableSha256 $originalExeHash
    }

    $outside = Join-Path ([IO.Path]::GetTempPath()) (
        'LateBrakePostRunOther-' + [Guid]::NewGuid().ToString('N'))
    [void](New-Item -ItemType Directory -Path $outside -Force)
    try {
        $outsideExe = Join-Path $outside 'Latebrake.exe'
        [IO.File]::WriteAllBytes($outsideExe, $original)
        Expect-Rejection 'EXE outside selected candidate folder' {
            & $verify -ManifestJson $manifest -GameExecutable $outsideExe `
                -ExpectedManifestSha256 $originalManifestHash `
                -ExpectedExecutableSha256 $originalExeHash
        }

        # Even an EMPTY junction into another location must be rejected.
        $link = Join-Path $temp 'RuntimeRedirect'
        New-Item -ItemType Junction -Path $link -Target $outside -ErrorAction Stop | Out-Null
        try {
            Expect-Rejection 'post-run linked candidate directory' { Invoke-Seal }
        } finally {
            Remove-Item -LiteralPath $link -Force -ErrorAction Stop
        }
    } finally {
        Remove-Item -LiteralPath $outside -Recurse -Force -ErrorAction SilentlyContinue
    }

    Invoke-Seal
    Write-Host 'Post-run seal passed all synthetic mutation checks; NO game binary was run.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
