<#
.SYNOPSIS
    Exercise circuit map snapshot policy without Unreal or fake game builds.
.DESCRIPTION
    All map-like payloads are temporary synthetic bytes. Passing only proves
    checksum policy behavior; none is an engine-authored or playable .umap.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Expect-Rejection {
    param([string]$Case, [scriptblock]$Action)
    $rejected = $false
    try { & $Action *> $null } catch { $rejected = $true }
    if (-not $rejected) { throw "Circuit snapshot accepted unsafe fixture: $Case" }
    Write-Host "Rejected circuit snapshot fixture: $Case"
}

$verifier = Join-Path $PSScriptRoot 'Test-CandidateMapSnapshot.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ("LatebrakeMapSnapshot-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null
try {
    $map = Join-Path $temp 'TEST_ONLY_NOT_A_REAL_MAP.umap'
    $original = [byte[]]::new(2048)
    for ($i = 0; $i -lt $original.Length; ++$i) {
        $original[$i] = [byte]($i % 251)
    }
    [IO.File]::WriteAllBytes($map, $original)
    $baseline = (Get-FileHash -LiteralPath $map -Algorithm SHA256).Hash.ToLowerInvariant()

    & $verifier -MapPath $map -ExpectedSha256 $baseline
    if (-not $?) { throw 'Identical synthetic bytes should pass hash identity.' }
    & $verifier -MapPath $map -ExpectedSha256 $baseline.ToUpperInvariant()
    if (-not $?) { throw 'Case-insensitive hexadecimal hash should be accepted.' }

    $mutated = [byte[]]::new($original.Length)
    [Array]::Copy($original, $mutated, $original.Length)
    $mutated[1000] = [byte]($mutated[1000] -bxor 1)
    [IO.File]::WriteAllBytes($map, $mutated)
    Expect-Rejection 'same-size map byte modification after cook' {
        & $verifier -MapPath $map -ExpectedSha256 $baseline
    }
    [IO.File]::WriteAllBytes($map, $original)
    & $verifier -MapPath $map -ExpectedSha256 $baseline
    if (-not $?) { throw 'Restored byte-identical snapshot must pass.' }

    Expect-Rejection 'wrong but well-formed map digest' {
        & $verifier -MapPath $map -ExpectedSha256 ('f' * 64)
    }
    Expect-Rejection 'truncated expected digest' {
        & $verifier -MapPath $map -ExpectedSha256 ('a' * 63)
    }
    Expect-Rejection 'explicitly blank expected digest' {
        & $verifier -MapPath $map -ExpectedSha256 ''
    }
    Expect-Rejection 'nonhex expected digest' {
        & $verifier -MapPath $map -ExpectedSha256 ('g' * 64)
    }
    [IO.File]::WriteAllBytes($map, [byte[]]::new(128))
    Expect-Rejection 'truncated map or LFS pointer' {
        & $verifier -MapPath $map -ExpectedSha256 $baseline
    }
    Remove-Item -LiteralPath $map
    Expect-Rejection 'missing map snapshot' {
        & $verifier -MapPath $map -ExpectedSha256 $baseline
    }
    Write-Host 'Map snapshot policy tests passed (synthetic data only; NO Unreal cook).'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
