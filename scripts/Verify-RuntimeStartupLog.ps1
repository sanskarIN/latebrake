<#
.SYNOPSIS
    Check a REAL launched UE game's runtime startup marker in its log.
.DESCRIPTION
    This reads a log captured after running a genuinely packaged LateBrake
    executable. It never launches or mocks an Unreal game, and a matching
    line only confirms the GameMode started and spawned four gates.
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
    throw 'GameMode reported missing checkpoint actors. The launch smoke gate failed.'
}
$ready = [regex]::Matches($log, '(?m)^.*LATEBRAKE_STARTUP_READY map=([^\r\n]+?) checkpoints=4/4\s*$')
if ($ready.Count -ne 1) {
    throw 'Expected exactly one complete LATEBRAKE_STARTUP_READY marker for the authentic four-gate circuit in this single-run log.'
}
$map = $ready[0].Groups[1].Value.Trim()
if ($map -notmatch '(^|[/\\]|_)LatebrakeCircuit$') {
    throw "Unexpected runtime world: $map; expected the project-owned LatebrakeCircuit level."
}
Write-Host "Real runtime startup marker confirmed in supplied log: map=$map checkpoints=4/4"
Write-Host 'NOT FULL QA: this does not prove vehicle controls, rendering, lap progression, saves, Android or performance.'
