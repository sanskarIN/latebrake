<#
.SYNOPSIS
    Check a REAL launched UE game's runtime startup markers in its log.
.DESCRIPTION
    This reads a log captured after running a genuinely packaged LateBrake
    executable. It never launches or mocks an Unreal game. A passing result
    means the runtime logged all nine collision-bearing track blocks, four
    unique checkpoint actors and 93 cosmetic mesh instances in seven ISM
    groups, but cannot prove collision, physics or rendering worked.
    Gameplay/device approval still requires human testing.
.EXAMPLE
    pwsh scripts/Verify-RuntimeStartupLog.ps1 -LogPath 'D:/Latebrake/WindowsNoEditor/Latebrake/Saved/Logs/Latebrake.log'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$LogPath
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) {
    throw "No real Unreal runtime log: $LogPath"
}
$file = Get-Item -LiteralPath $LogPath
if ($file.Length -lt 100) {
    throw 'Runtime log is empty or too short for useful startup diagnostics.'
}
$log = Get-Content -LiteralPath $file.FullName -Raw -ErrorAction Stop
if ($log -match 'LATEBRAKE_STARTUP_BLOCKED') {
    throw 'GameMode reported an incomplete circuit. The launch smoke gate failed.'
}
if ($log -match 'LATEBRAKE_GEOMETRY_INCOMPLETE') {
    throw 'GameMode reported failed ground/road/barrier creation. The launch smoke gate failed.'
}
if ($log -match 'LATEBRAKE_SCENERY_INCOMPLETE') {
    throw 'GameMode reported incomplete instanced scenery. The launch smoke gate failed.'
}
$ready = [regex]::Matches($log, '(?m)^.*LATEBRAKE_STARTUP_READY map=([^\r\n]+?) checkpoints=4/4\s*$')
if ($ready.Count -ne 1) {
    throw 'Expected exactly one complete LATEBRAKE_STARTUP_READY marker for the authentic four-gate circuit in this single-run log.'
}
$map = $ready[0].Groups[1].Value.Trim()
if ($map -notmatch '(^|[/\\]|_)LatebrakeCircuit$') {
    throw "Unexpected runtime world: $map; expected the project-owned LatebrakeCircuit level."
}

# Source-authored collision geometry is a REQUIRED third startup category.
# Check counts, singleton occurrence and ordering, not merely the marker word.
$geometry = [regex]::Matches($log, '(?m)^.*LATEBRAKE_GEOMETRY_READY collision=9/9\s*
# creation failed. Require a full, single-instance assembly marker as well.
# The marker logs C++ allocation counts, not successful GPU drawing.
$scenery = [regex]::Matches($log, '(?m)^.*LATEBRAKE_SCENERY_READY groups=7 instances=93 collision=off\s*$')
if ($scenery.Count -ne 1) {
    throw 'Expected exactly one complete LATEBRAKE_SCENERY_READY groups=7 instances=93 collision=off marker from the same real game run.'
}
if ($scenery[0].Index -ge $ready[0].Index) {
    throw 'Scenery marker was not recorded before the final checkpoint startup marker.'
}
if ($scenery[0].Index -ge $geometry[0].Index) {
    throw 'Expected complete scenery before the final collision geometry and gate audit markers.'
}
Write-Host "Real runtime startup markers confirmed: map=$map checkpoints=4/4 collision-blocks=9/9 scenery=7/93 instances-collision=off"
Write-Host 'NOT FULL QA: this does not prove vehicle controls, rendering, lap progression, saves, Android or performance.'
)
if ($geometry.Count -ne 1) {
    throw 'Expected exactly one genuine 9/9 collision-bearing block creation marker from the same run.'
}
if ($geometry[0].Index -ge $ready[0].Index) {
    throw 'Collision geometry must be created before final checkpoint startup READY.'
}

# A gate-only startup marker can succeed even if the instanced scenery
# creation failed. Require a full, single-instance assembly marker as well.
# The marker logs C++ allocation counts, not successful GPU drawing.
$scenery = [regex]::Matches($log, '(?m)^.*LATEBRAKE_SCENERY_READY groups=7 instances=93 collision=off\s*$')
if ($scenery.Count -ne 1) {
    throw 'Expected exactly one complete LATEBRAKE_SCENERY_READY groups=7 instances=93 collision=off marker from the same real game run.'
}
if ($scenery[0].Index -ge $ready[0].Index) {
    throw 'Scenery marker was not recorded before the final checkpoint startup marker.'
}
Write-Host "Real runtime startup markers confirmed: map=$map checkpoints=4/4 scenery=7/93 collision=off"
Write-Host 'NOT FULL QA: this does not prove vehicle controls, rendering, lap progression, saves, Android or performance.'
