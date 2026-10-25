<#
.SYNOPSIS
    Verify the exact installed Unreal Build.version bytes for a candidate.
.DESCRIPTION
    Unreal's MajorVersion/MinorVersion alone cannot distinguish two different
    5.8 editor builds. Use an immutable SHA256 of Engine/Build/Build.version
    throughout the one-command Editor + UAT + manifest pipeline. This is
    metadata integrity, not proof of the actual engine executable or gameplay.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EngineRoot,
    [Parameter(Mandatory)][string]$ExpectedSha256
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($ExpectedSha256 -cnotmatch '^[a-f0-9]{64}$') {
    throw 'Expected Unreal Build.version SHA256 must be 64 lowercase hex characters.'
}
$versionPath = Join-Path $EngineRoot 'Engine/Build/Build.version'
if (-not (Test-Path -LiteralPath $versionPath -PathType Leaf)) {
    throw "Unreal engine version metadata unavailable: $versionPath"
}
$versionFile = Get-Item -LiteralPath $versionPath -Force -ErrorAction Stop
if (($versionFile.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'Unreal Build.version may not be a symlink or junction.'
}
$engine = Get-Content -LiteralPath $versionFile.FullName -Raw -ErrorAction Stop |
    ConvertFrom-Json -ErrorAction Stop
if ($engine.MajorVersion -ne 5 -or $engine.MinorVersion -ne 8) {
    throw 'Only an installed Unreal Engine 5.8 metadata snapshot is acceptable.'
}
$actual = (Get-FileHash -LiteralPath $versionFile.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -cne $ExpectedSha256) {
    throw 'Unreal Build.version changed from the pinned engine metadata SHA256; discard the candidate.'
}
Write-Host "Pinned Unreal 5.8 Build.version SHA256 unchanged: $actual"
Write-Host 'Build.version identity is NOT engine binary attestation or actual Unreal execution.'
