<# 
.SYNOPSIS
    Fail-closed fixture tests for inline Unreal candidate workflow scripts.
.DESCRIPTION
    Uses disposable YAML and never invokes licensed UE or game packages.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$validator = Join-Path $PSScriptRoot 'Test-UnrealCandidateWorkflowInlineSyntax.ps1'
$workflow = Join-Path $PSScriptRoot '../.github/workflows/build-unreal-candidate.yml'
& $validator -WorkflowPath $workflow
if (-not $?) { throw 'Actual manual workflow did not parse.' }

$fixture = Join-Path ([System.IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString('N') + '.yml')
try {
    $source = Get-Content -LiteralPath $workflow -Raw
    $needle = '          if ([string]::IsNullOrWhiteSpace($env:LATEBRAKE_UNREAL_ROOT)) {'
    if (-not $source.Contains($needle)) {
        throw 'Unreal workflow source changed: update malformed-script regression fixture.'
    }
    $mutant = $source.Replace($needle, '          if (')
    Set-Content -LiteralPath $fixture -Value $mutant -Encoding utf8
    $rejected = $false
    try {
        & $validator -WorkflowPath $fixture *> $null
    } catch {
        $rejected = $true
    }
    if (-not $rejected) {
        throw 'Malformed inline script did not fail closed.'
    }
    Write-Host 'Malformed manual workflow inline PowerShell fixture rejected.'
} finally {
    Remove-Item -LiteralPath $fixture -Force -ErrorAction SilentlyContinue
}
