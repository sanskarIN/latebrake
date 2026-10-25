<#
.SYNOPSIS
    Offline engine-build metadata fingerprint fixtures. No Unreal installed.
.DESCRIPTION
    Uses only disposable synthetic Build.version JSON, never an actual UE SDK.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$validator = Join-Path $PSScriptRoot 'Test-EngineBuildSnapshot.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ('LatebrakeUEBuildFixture-' + [Guid]::NewGuid().ToString('N'))
$engineDirectory = Join-Path $temp 'Engine/Build'
New-Item -ItemType Directory -Path $engineDirectory -Force | Out-Null
$versionPath = Join-Path $engineDirectory 'Build.version'
function Expect-Rejection {
    param([string]$Case, [scriptblock]$Test)
    $caught = $false
    try { & $Test *> $null } catch { $caught = $true }
    if (-not $caught) { throw "Synthetic engine-build policy failed to reject: $Case" }
    Write-Host "Rejected engine-build fixture: $Case"
}

try {
    $original = '{"MajorVersion":5,"MinorVersion":8,"Changelist":12345678}'
    [IO.File]::WriteAllText($versionPath, $original)
    $digest = (Get-FileHash -LiteralPath $versionPath -Algorithm SHA256).Hash.ToLowerInvariant()
    & $validator -EngineRoot $temp -ExpectedSha256 $digest
    if (-not $?) { throw 'Valid synthetic UE 5.8 metadata should pass.' }

    $mutated = '{"MajorVersion":5,"MinorVersion":8,"Changelist":87654321}'
    if ($mutated.Length -ne $original.Length) { throw 'Mutation fixture must retain exact byte length.' }
    [IO.File]::WriteAllText($versionPath, $mutated)
    Expect-Rejection 'same length different UE 5.8 changelist' {
        & $validator -EngineRoot $temp -ExpectedSha256 $digest
    }
    [IO.File]::WriteAllText($versionPath, $original)

    Expect-Rejection 'different expected lowercase fingerprint' {
        & $validator -EngineRoot $temp -ExpectedSha256 ('a' * 64)
    }
    Expect-Rejection 'truncated expected digest' {
        & $validator -EngineRoot $temp -ExpectedSha256 ('a' * 63)
    }
    Expect-Rejection 'uppercase digest' {
        & $validator -EngineRoot $temp -ExpectedSha256 ($digest.ToUpperInvariant())
    }

    [IO.File]::WriteAllText($versionPath, '{"MajorVersion":5,"MinorVersion":7,"Changelist":12345678}')
    $wrongVersionDigest = (Get-FileHash -LiteralPath $versionPath -Algorithm SHA256).Hash.ToLowerInvariant()
    Expect-Rejection 'matching hash but unsupported installed Unreal 5.7' {
        & $validator -EngineRoot $temp -ExpectedSha256 $wrongVersionDigest
    }
    [IO.File]::WriteAllText($versionPath, $original)
    Remove-Item -LiteralPath $versionPath -Force
    Expect-Rejection 'missing Unreal build metadata' {
        & $validator -EngineRoot $temp -ExpectedSha256 $digest
    }
    Write-Host 'All synthetic exact-engine metadata tests passed; no UE build was performed.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
