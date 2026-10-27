<#
.SYNOPSIS
    Reject malformed local game candidates without starting any executable.
.DESCRIPTION
    No Unreal Engine, game assets or real executable is used. A disposable
    fake candidate must not be allowed to start or produce runtime evidence.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$temp = Join-Path ([IO.Path]::GetTempPath()) ('LateBrakeRuntimeProbePolicy-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $temp -Force)
try {
    $missing = Join-Path $temp 'missing'
    $logs = Join-Path $temp 'logs'
    $runner = Join-Path $PSScriptRoot 'Run-WindowsPlaytest.ps1'
    $rejectedMissing = $false
    try {
        & $runner -CandidateDirectory $missing -LogDirectory $logs *> $null
    } catch {
        $rejectedMissing = $true
    }
    if (-not $rejectedMissing -or (Test-Path -LiteralPath $logs)) {
        throw 'A missing game directory was accepted or wrote runtime evidence.'
    }
    $fake = Join-Path $temp 'fake-win64'
    [void](New-Item -ItemType Directory -Path $fake)
    [IO.File]::WriteAllBytes((Join-Path $fake 'Latebrake.exe'), [byte[]](0x4D,0x5A))
    $rejectedFake = $false
    try {
        & $runner -CandidateDirectory $fake -LogDirectory $logs *> $null
    } catch {
        $rejectedFake = $true
    }
    if (-not $rejectedFake -or (Test-Path -LiteralPath $logs)) {
        throw 'A fake Win64 EXE without a valid manifest was accepted or launched.'
    }
    Set-Content -LiteralPath (Join-Path $fake 'Latebrake-candidate-manifest.json') -Value '{"schema_version":2,"platform":"Win64"}' -Encoding utf8
    $rejectedMalformed = $false
    try {
        & $runner -CandidateDirectory $fake -LogDirectory $logs *> $null
    } catch {
        $rejectedMalformed = $true
    }
    if (-not $rejectedMalformed -or (Test-Path -LiteralPath $logs)) {
        throw 'A fake Win64 EXE with a malformed manifest was accepted.'
    }
    Write-Host 'Private Win64 runtime launcher rejected absent, unsigned and malformed candidates.'
    Write-Host 'These checks do not launch or produce a real packaged game.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
