<#
.SYNOPSIS
    Exercise private runtime diagnostic path restrictions with disposable data.
.DESCRIPTION
    No Unreal executable or real game archive is used. On hosted Windows,
    junction aliases MUST NOT allow runtime logs inside the game archive.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'The Win64 junction safety fixture must run on Windows.' }

$policy = Join-Path $PSScriptRoot 'Assert-SafeWindowsPlaytestLogDirectory.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ('LateBrakeLogPathPolicy-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $temp -ErrorAction Stop)
try {
    $candidate = Join-Path $temp 'candidate'
    [void](New-Item -ItemType Directory -Path $candidate)
    $good = Join-Path $temp 'logs/deep'
    $selected = & $policy -CandidateRoot $candidate -LogDirectory $good
    if ($selected -cne [IO.Path]::GetFullPath($good) -or
        -not (Test-Path -LiteralPath $good -PathType Container)) {
        throw 'An ordinary external runtime log directory was not created.'
    }

    $inside = Join-Path $candidate 'logs'
    $rejectedInside = $false
    try { & $policy -CandidateRoot $candidate -LogDirectory $inside *> $null }
    catch { $rejectedInside = $true }
    if (-not $rejectedInside -or (Test-Path -LiteralPath $inside)) {
        throw 'Lexically contained runtime log directory was accepted.'
    }

    $fileParent = Join-Path $temp 'ordinary-file'
    Set-Content -LiteralPath $fileParent -Value 'not a folder'
    $rejectedFileParent = $false
    try {
        & $policy -CandidateRoot $candidate -LogDirectory (Join-Path $fileParent 'logs') *> $null
    } catch { $rejectedFileParent = $true }
    if (-not $rejectedFileParent) {
        throw 'A file in an output directory ancestor was accepted.'
    }

    # NTFS directory junctions do not require installing Unreal or building
    # a game and exercise the vulnerability missed by leaf-only checks.
    $alias = Join-Path $temp 'outside-looking-junction'
    [void](New-Item -ItemType Junction -Path $alias -Target $candidate -ErrorAction Stop)
    $escaped = Join-Path $alias 'should-not-exist'
    $rejectedLinkedParent = $false
    try { & $policy -CandidateRoot $candidate -LogDirectory $escaped *> $null }
    catch { $rejectedLinkedParent = $true }
    if (-not $rejectedLinkedParent -or
        (Test-Path -LiteralPath (Join-Path $candidate 'should-not-exist'))) {
        throw 'Junction parent escaped the immutable Win64 candidate boundary.'
    }

    $linkedCandidate = Join-Path $temp 'linked-candidate'
    [void](New-Item -ItemType Junction -Path $linkedCandidate -Target $candidate -ErrorAction Stop)
    $rejectedLinkedCandidate = $false
    try { & $policy -CandidateRoot $linkedCandidate -LogDirectory $good *> $null }
    catch { $rejectedLinkedCandidate = $true }
    if (-not $rejectedLinkedCandidate) {
        throw 'A junction candidate directory was accepted.'
    }
    Write-Host 'Win64 private log path policy rejects contained, linked and file-ancestor paths.'
    Write-Host 'No Unreal game or executable was launched.'
} finally {
    Remove-Item -LiteralPath (Join-Path $temp 'outside-looking-junction') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath (Join-Path $temp 'linked-candidate') -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
