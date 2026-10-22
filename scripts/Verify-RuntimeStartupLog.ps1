<#
.SYNOPSIS
    Validate startup markers captured from a real Unreal game runtime.
.DESCRIPTION
    Checks log grammar, ordering and exact actor construction counts for
    9 collision-bearing ground/road/barrier blocks, four checkpoint actors
    and 7/93 collision-free scenery groups/instances.
    This is NOT evidence of rendered physics, a runnable EXE or device QA.
.EXAMPLE
    pwsh scripts/Verify-RuntimeStartupLog.ps1 -LogPath 'D:/Latebrake/Saved/Logs/Latebrake.log'
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$LogPath)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) {
    throw "Missing real Unreal runtime log: $LogPath"
}
$file = Get-Item -LiteralPath $LogPath
if ($file.Length -lt 100) {
    throw 'Runtime log is empty or too short for useful startup diagnostics.'
}
$log = Get-Content -LiteralPath $file.FullName -Raw -ErrorAction Stop

foreach ($failure in @('LATEBRAKE_STARTUP_BLOCKED',
                       'LATEBRAKE_SCENERY_INCOMPLETE',
                       'LATEBRAKE_GEOMETRY_INCOMPLETE',
                       'LATEBRAKE_COLLISION_SETUP_FAILED')) {
    if ($log.Contains($failure)) {
        throw "GameMode logged failure marker $failure; startup gate rejected."
    }
}

$ready = [regex]::Matches(
    $log, '(?m)^.*LATEBRAKE_STARTUP_READY map=([^\r\n]+?) checkpoints=4/4\s*$')
if ($ready.Count -ne 1) {
    throw 'Expected exactly one complete LATEBRAKE_STARTUP_READY map/4-gate marker in a single-run log.'
}
$map = $ready[0].Groups[1].Value.Trim()
if ($map -notmatch '(^|[/\\]|_)LatebrakeCircuit$') {
    throw "Unexpected runtime world: $map; expected LatebrakeCircuit level."
}

$scenery = [regex]::Matches(
    $log, '(?m)^.*LATEBRAKE_SCENERY_READY groups=7 instances=93 collision=off\s*$')
if ($scenery.Count -ne 1) {
    throw 'Expected one source-created 7-group/93-instance scenery READY marker.'
}

$geometry = [regex]::Matches(
    $log, '(?m)^.*LATEBRAKE_GEOMETRY_READY collision=9/9\s*$')
if ($geometry.Count -ne 1) {
    throw 'Expected one source-created 9/9 collision-bearing block READY marker.'
}
if ($scenery[0].Index -ge $geometry[0].Index -or
    $geometry[0].Index -ge $ready[0].Index) {
    throw 'Startup marker order must be scenery, geometry, then checkpoint ready.'
}

Write-Host "Runtime log markers parsed: map=$map blocks=9/9 checkpoints=4/4 scenery=7/93"
Write-Host 'NOT FULL QA: textual actor counts do not prove rendering, collision, controls, race progress, saved games, Android or frame time.'
