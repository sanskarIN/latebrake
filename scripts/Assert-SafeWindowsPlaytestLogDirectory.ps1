<#
.SYNOPSIS
    Choose a private diagnostic directory without crossing a junction.
.DESCRIPTION
    The supplied path must be outside the immutable game candidate.
    Every existing ancestor of both paths is inspected: lexical relative
    checks alone can be bypassed by a linked *parent* directory. A successful
    call creates a regular diagnostic directory if it does not exist and
    returns its absolute path. Does not launch an executable or certify QA.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$CandidateRoot,
    [Parameter(Mandatory)][string]$LogDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-NoReparseAncestor {
    param([Parameter(Mandatory)][string]$FullPath)
    $cursor = [IO.Path]::GetFullPath($FullPath)
    while ($true) {
        # Inspect ancestors themselves, not the resolved target: a junction
        # outside CandidateRoot could otherwise write *inside* the archive.
        $node = Get-Item -LiteralPath $cursor -Force -ErrorAction SilentlyContinue
        if ($null -ne $node) {
            if (($node.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Junction/symlink ancestor is forbidden in private playtest paths: $cursor"
            }
            if (-not $node.PSIsContainer) {
                throw "A private playtest path ancestor must be a directory: $cursor"
            }
        }
        $parent = [IO.Directory]::GetParent($cursor)
        if ($null -eq $parent) { break }
        $cursor = $parent.FullName
    }
}

$root = [IO.Path]::GetFullPath($CandidateRoot)
$absoluteLogs = [IO.Path]::GetFullPath($LogDirectory)
if (-not (Test-Path -LiteralPath $root -PathType Container)) {
    throw 'The Win64 candidate directory must exist.'
}
Assert-NoReparseAncestor -FullPath $root

$relative = [IO.Path]::GetRelativePath($root, $absoluteLogs)
if ($relative -eq '.' -or
    ($relative -ne '..' -and
     -not $relative.StartsWith('..' + [IO.Path]::DirectorySeparatorChar))) {
    throw 'LogDirectory must be outside the immutable packaged candidate folder.'
}
Assert-NoReparseAncestor -FullPath $absoluteLogs
if (Test-Path -LiteralPath $absoluteLogs) {
    if (-not (Test-Path -LiteralPath $absoluteLogs -PathType Container)) {
        throw 'LogDirectory must be an ordinary directory, not a file.'
    }
} else {
    [void](New-Item -ItemType Directory -Path $absoluteLogs -Force -ErrorAction Stop)
}
# Recheck the newly created directory, including its ancestors.
Assert-NoReparseAncestor -FullPath $absoluteLogs
Write-Output $absoluteLogs
