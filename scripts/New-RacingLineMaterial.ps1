<#
.SYNOPSIS
    Have a genuine licensed Unreal Editor 5.8 author the racing-line material.
.DESCRIPTION
    Requires Editor Python and an authorized Windows UE 5.8 installation.
    Removes stale authoring receipt, executes Editor, compares saved bytes to
    a fresh SHA256-bound Editor receipt. Never claims game or shader QA.
.EXAMPLE
    pwsh ./scripts/New-RacingLineMaterial.ps1 -EngineRoot 'D:/UE_5.8'
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EngineRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $IsWindows -or $PSVersionTable.PSVersion.Major -lt 7) {
    throw 'Material authoring requires Windows PowerShell 7 and genuine UE 5.8.'
}
$repo = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repo 'Latebrake.uproject'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$versionFile = Join-Path $EngineRoot 'Engine/Build/Build.version'
$script = Join-Path $PSScriptRoot 'Unreal/AuthorRacingLineMaterial.py'
$asset = Join-Path $repo 'Content/Materials/M_RacingLineGuide.uasset'
$receipt = Join-Path $repo 'Saved/LatebrakeAutomation/racing-line-material-result.json'

foreach ($required in @($project, $editor, $versionFile, $script)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing genuine project/Unreal material-authoring prerequisite: $required"
    }
}
$engine = Get-Content -LiteralPath $versionFile -Raw -ErrorAction Stop |
    ConvertFrom-Json -ErrorAction Stop
$descriptor = Get-Content -LiteralPath $project -Raw -ErrorAction Stop |
    ConvertFrom-Json -ErrorAction Stop
if ($engine.MajorVersion -ne 5 -or $engine.MinorVersion -ne 8 -or
    $descriptor.EngineAssociation -cne '5.8') {
    throw 'Material authoring requires an installed UE 5.8 engine and 5.8 project.'
}
$project = (Resolve-Path -LiteralPath $project).Path
$script = (Resolve-Path -LiteralPath $script).Path

# Success cannot be inferred from UnrealEditor.exe process exit alone;
# Editor Python exceptions may not always cause a nonzero exit status.
if (Test-Path -LiteralPath $receipt -PathType Leaf) {
    Remove-Item -LiteralPath $receipt -Force
}
Write-Host 'Launching real Unreal Editor to create or validate M_RacingLineGuide.'
& $editor $project "-ExecutePythonScript=$script" '-unattended' '-nop4' '-nosplash'
if ($LASTEXITCODE -ne 0) {
    throw "Unreal material authoring failed with exit code $LASTEXITCODE."
}
if (-not (Test-Path -LiteralPath $asset -PathType Leaf)) {
    throw 'Unreal Editor exited without saving the expected material asset.'
}
if (-not (Test-Path -LiteralPath $receipt -PathType Leaf)) {
    throw 'Unreal Editor did not produce a fresh successful material receipt.'
}
& (Join-Path $PSScriptRoot 'Test-RacingLineMaterialReceipt.ps1') -MaterialPath $asset -ReceiptPath $receipt
if (-not $?) { throw 'Material receipt and actual file bytes failed the policy.' }

Write-Host "Actual Unreal material: $asset"
Write-Host 'Review the graph and shader inside Unreal Material Editor.'
Write-Host 'Commit the real asset with Git LFS and rebuild a clean, pinned Win64 candidate.'
Write-Host 'This authoring step does NOT certify rendered colors, game compilation or gameplay.'
