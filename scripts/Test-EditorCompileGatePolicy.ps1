<#
.SYNOPSIS
    Synthetic unavailable-engine rejection fixture (NO Unreal executed).
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetTempPath()) ('LateBrake-editor-gate-' + [Guid]::NewGuid().ToString('N'))
$fake = Join-Path $root 'Missing-Unreal-5.8'
[void](New-Item -ItemType Directory -Path $root -Force)
try {
    $rejected = $false
    try {
        & (Join-Path $PSScriptRoot 'Compile-UnrealEditor.ps1') -EngineRoot $fake *> $null
    } catch {
        $rejected = $true
    }
    if (-not $rejected) {
        throw 'Missing licensed UE was wrongly accepted for a real Editor build.'
    }
    if (Test-Path -LiteralPath $fake) {
        throw 'Missing-engine rejection must not manufacture an Unreal folder.'
    }
    Write-Host 'Unreal compile-only negative fixture passed (NO UE executed).'
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force
}
