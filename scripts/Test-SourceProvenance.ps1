<#
.SYNOPSIS
    Require a traceable LateBrake source checkout before real Unreal packaging.
.DESCRIPTION
    Git HEAD alone does not identify untracked source: an untracked .cpp, .h,
    .ini or plugin descriptor can silently alter a locally cooked game even
    when every tracked file is clean. Fail closed on staged/unstaged changes
    and all nonignored untracked files. The one intentional exception is the
    authentic, editor-authored Content/Maps/LatebrakeCircuit.umap: its exact
    bytes are fingerprinted separately by Write-CandidateManifest.ps1.

    This checks source provenance ONLY. No engine, cooked binary or playtest
    is authenticated by a passing check.
.EXAMPLE
    pwsh scripts/Test-SourceProvenance.ps1
#>
[CmdletBinding()]
param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$ExpectedCommit
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $RepoRoot -PathType Container)) {
    throw "Source checkout directory does not exist: $RepoRoot"
}
$root = (Resolve-Path -LiteralPath $RepoRoot).Path
$gitRoot = @(& git -C $root rev-parse --show-toplevel 2>$null)
if ($LASTEXITCODE -ne 0 -or $gitRoot.Count -ne 1 -or
    [string]::IsNullOrWhiteSpace($gitRoot[0])) {
    throw 'Source provenance requires a real Git checkout.'
}
$top = [IO.Path]::GetFullPath([string]$gitRoot[0])
if (-not $top.TrimEnd([char[]]@('/', '\')).Equals(
        ([IO.Path]::GetFullPath($root)).TrimEnd([char[]]@('/', '\')),
        [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Source provenance must run at the checkout root, not a nested or unrelated directory.'
}
$sha = @(& git -C $root rev-parse --verify HEAD 2>$null)
if ($LASTEXITCODE -ne 0 -or $sha.Count -ne 1 -or
    $sha[0] -cnotmatch '^[a-fA-F0-9]{40}$') {
    throw 'Source provenance requires a committed Git HEAD.'
}

# A real candidate may run UHT, Editor, map authoring, UAT and hashing
# over many minutes. A clean worktree at both ends is insufficient if HEAD
# switched in between: pin every checkpoint to the originally reviewed SHA.
if (-not [string]::IsNullOrWhiteSpace($ExpectedCommit)) {
    if ($ExpectedCommit -cnotmatch '^[a-fA-F0-9]{40}, rather than only their
# directory names. Use the original porcelain format; treat unknown/quoted
# paths conservatively, never attempting to unescape a possible code file.
$changes = @(& git -C $root -c core.quotePath=true status --porcelain=v1 --untracked-files=all 2>$null)
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect complete Git source status.'
}
foreach ($change in $changes) {
    if ($change -ceq '?? Content/Maps/LatebrakeCircuit.umap') {
        # The actual serialized file must still be validated and its
        # SHA256 recorded after genuine Unreal Editor map authoring.
        continue
    }
    throw "Uncommitted or untracked source blocks candidate provenance: $change"
}
Write-Host "Git source is traceable at HEAD $($sha[0]); only the optional editor-authored map may be untracked."
Write-Host 'This check does NOT qualify the Unreal engine, package, map cook or hardware playtest.'
 -or
        $sha[0] -cne $ExpectedCommit) {
        throw 'Source HEAD changed from the expected candidate commit; discard this build.'
    }
}

# -uall enumerates individual untracked files, rather than only their
# directory names. Use the original porcelain format; treat unknown/quoted
# paths conservatively, never attempting to unescape a possible code file.
$changes = @(& git -C $root -c core.quotePath=true status --porcelain=v1 --untracked-files=all 2>$null)
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to inspect complete Git source status.'
}
foreach ($change in $changes) {
    if ($change -ceq '?? Content/Maps/LatebrakeCircuit.umap') {
        # The actual serialized file must still be validated and its
        # SHA256 recorded after genuine Unreal Editor map authoring.
        continue
    }
    throw "Uncommitted or untracked source blocks candidate provenance: $change"
}
Write-Host "Git source is traceable at HEAD $($sha[0]); only the optional editor-authored map may be untracked."
Write-Host 'This check does NOT qualify the Unreal engine, package, map cook or hardware playtest.'
