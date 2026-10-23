<#
.SYNOPSIS
    Verify an unchanged circuit map snapshot by SHA256.
.DESCRIPTION
    Checks only byte identity, not Unreal map authenticity or game playability.
    Call Verify-UnrealMapHeader.ps1 separately before the actual Unreal cook.
    A hash captured after cooking cannot prove that the same map was cooked.
.EXAMPLE
    pwsh scripts/Test-CandidateMapSnapshot.ps1 -MapPath 'Content/Maps/LatebrakeCircuit.umap' -ExpectedSha256 ('a' * 64)
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$MapPath,
    [Parameter(Mandatory)][string]$ExpectedSha256
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($ExpectedSha256 -cnotmatch '^[a-fA-F0-9]{64}$') {
    throw 'Expected circuit map SHA256 must be exactly 64 hexadecimal characters.'
}
if (-not (Test-Path -LiteralPath $MapPath -PathType Leaf)) {
    throw "Circuit map snapshot missing: $MapPath"
}
$map = Get-Item -LiteralPath $MapPath -Force -ErrorAction Stop
if (($map.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'Circuit map snapshot cannot refer to a symbolic link or reparse point.'
}
if ($map.Length -lt 1024) {
    throw 'Circuit map snapshot is implausibly small (or an LFS pointer).'
}
$actual = (Get-FileHash -LiteralPath $map.FullName -Algorithm SHA256).Hash
if ($actual -ine $ExpectedSha256) {
    throw 'Circuit map changed from the pre-cook snapshot. Reject candidate and rebuild.'
}
Write-Host "Circuit map snapshot unchanged: SHA256 $($actual.ToLowerInvariant())"
Write-Host 'This verifies bytes only; genuine Unreal Editor/cook/runtime and devices remain unverified.'
