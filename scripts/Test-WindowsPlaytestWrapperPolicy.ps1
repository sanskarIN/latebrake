<#
.SYNOPSIS
    Prove a missing Unreal Engine cannot create a fake personal Win64 build.
.DESCRIPTION
    Disposable source-level negative test only. NO UE, EXE or package created.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$temp = Join-Path ([IO.Path]::GetTempPath()) ('LateBrakePrivateWinTest-' + [Guid]::NewGuid().ToString('N'))
$badEngine = Join-Path $temp 'Uninstalled-UE_5.8'
$output = Join-Path $temp 'CandidateOutputs'
[void](New-Item -ItemType Directory -Path $temp -Force)
try {
    $rejected = $false
    try {
        & (Join-Path $PSScriptRoot 'Build-WindowsPlaytest.ps1') -EngineRoot $badEngine -OutputRoot $output *> $null
    } catch {
        $rejected = $true
    }
    if (-not $rejected) {
        throw 'Missing authorized Unreal Engine was wrongly accepted.'
    }
    if (Test-Path -LiteralPath $output) {
        throw 'Failed personal preflight should not create a candidate directory.'
    }
    Write-Host 'Absent UE 5.8 correctly rejects a personal Win64 build with no fake outputs.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
