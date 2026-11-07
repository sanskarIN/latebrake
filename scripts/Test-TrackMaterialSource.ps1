<#
.SYNOPSIS
    Require a traceable original Unreal track material for realistic preview candidates.
.DESCRIPTION
    Read-only. The exact .uasset must already be committed at HEAD, hydrated
    from LFS and optionally match a pre-build SHA256. This is not UE proof.
#>
[CmdletBinding()]
param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [switch]$RequireMaterial,
    [string]$ExpectedSha256
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$relative = 'Content/Latebrake/Materials/M_LatebrakeTrackPBR.uasset'
if (-not (Test-Path -LiteralPath $RepoRoot -PathType Container)) {
    throw 'Missing repository root for material provenance.'
}
$root = (Resolve-Path -LiteralPath $RepoRoot).Path
$material = Join-Path $root $relative
if (-not (Test-Path -LiteralPath $material -PathType Any)) {
    if ($RequireMaterial -or $PSBoundParameters.ContainsKey('ExpectedSha256')) {
        throw 'Required genuine material missing. Create with UE5.8, commit via Git LFS, then rebuild.'
    }
    Write-Host 'No authored track material: prototype fallback only, NOT qualified realistic art.'
    return
}

# A file beneath a junction may resolve outside the source tree.
$cursor = $root
foreach ($part in @('Content', 'Latebrake', 'Materials')) {
    $cursor = Join-Path $cursor $part
    $node = Get-Item -LiteralPath $cursor -Force -ErrorAction Stop
    if (-not $node.PSIsContainer -or
        ($node.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Material path contains a junction, symlink or non-directory.'
    }
}
$file = Get-Item -LiteralPath $material -Force -ErrorAction Stop
if ($file.PSIsContainer -or
    ($file.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'Authored material must be a regular file, not a link.'
}
$gitRoot = @(& git -C $root rev-parse --show-toplevel 2>$null)
if ($LASTEXITCODE -ne 0 -or $gitRoot.Count -ne 1 -or
    -not ([IO.Path]::GetFullPath([string]$gitRoot[0])).TrimEnd([char[]]@('/', '\')).Equals(
        ([IO.Path]::GetFullPath($root)).TrimEnd([char[]]@('/', '\')),
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Track material must belong to the same Git checkout.'
}
$tracked = @(& git -C $root ls-files --error-unmatch -- $relative 2>$null)
if ($LASTEXITCODE -ne 0 -or $tracked.Count -ne 1 -or
    $tracked[0] -cne $relative) {
    throw 'Untracked material cannot be attributed to candidate source commit.'
}
$changes = @(& git -C $root status --porcelain=v1 --untracked-files=all -- $relative 2>$null)
if ($LASTEXITCODE -ne 0 -or $changes.Count -ne 0) {
    throw 'Staged or modified material differs from the committed source.'
}
# Git LFS pointer is a tiny text placeholder and cannot be cooked.
if ($file.Length -lt 1024) {
    throw 'Material is too small: missing genuine Unreal bytes or Git LFS not hydrated.'
}
$actual = (Get-FileHash -LiteralPath $material -Algorithm SHA256).Hash.ToLowerInvariant()
if ($PSBoundParameters.ContainsKey('ExpectedSha256') -and
    ($ExpectedSha256 -cnotmatch '^[a-f0-9]{64}$' -or $ExpectedSha256 -cne $actual)) {
    throw 'Original material SHA256 changed since the pre-build snapshot.'
}
Write-Host "Tracked material SHA256: $actual"
Write-Host 'Byte identity is NOT proof of Unreal material validity or rendered appearance.'
