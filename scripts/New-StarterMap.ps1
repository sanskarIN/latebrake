<#
.SYNOPSIS
    Create/check the authentic starter circuit .umap using Unreal Editor 5.8.
.DESCRIPTION
    This launches the licensed Unreal Editor Python scripting environment.
    No .umap is authored by PowerShell or fabricated as text; the actual
    serialized map is created/saved by the engine. Running twice is safe.
.EXAMPLE
    pwsh scripts/New-StarterMap.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EngineRoot
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repo 'Latebrake.uproject'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$versionFile = Join-Path $EngineRoot 'Engine/Build/Build.version'
$python = Join-Path $PSScriptRoot 'Unreal/CreateStarterMap.py'
$map = Join-Path $repo 'Content/Maps/LatebrakeCircuit.umap'
$receipt = Join-Path $repo 'Saved/LatebrakeAutomation/starter-map-result.json'

if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
    throw "Unreal project missing: $project"
}
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) {
    throw "Unreal Editor executable not found: $editor. Install authorized UE 5.8 first."
}
if (-not (Test-Path -LiteralPath $python -PathType Leaf)) {
    throw "Editor map creation script missing: $python"
}
if (-not (Test-Path -LiteralPath $versionFile -PathType Leaf)) {
    throw "Unreal engine version metadata unavailable: $versionFile"
}
$engine = Get-Content -LiteralPath $versionFile -Raw -ErrorAction Stop | ConvertFrom-Json -ErrorAction Stop
if ($engine.MajorVersion -ne 5 -or $engine.MinorVersion -ne 8) {
    throw "Starter level must be authored with Unreal Engine 5.8, found $($engine.MajorVersion).$($engine.MinorVersion)"
}
$descriptor = Get-Content -LiteralPath $project -Raw -ErrorAction Stop | ConvertFrom-Json -ErrorAction Stop
if ($descriptor.EngineAssociation -cne '5.8') {
    throw "Latebrake.uproject must target Unreal Engine 5.8, found $($descriptor.EngineAssociation)"
}

$project = (Resolve-Path -LiteralPath $project).Path
$python = (Resolve-Path -LiteralPath $python).Path

Write-Host "Launching actual Unreal Editor to create/check /Game/Maps/LatebrakeCircuit."
Write-Host 'Editor-only Python Script and Editor Scripting Utilities plugins must be available.'
# Do not accept a receipt from a previous Editor run. Python exceptions may
# not force a nonzero UnrealEditor.exe exit status in every engine build.
if (Test-Path -LiteralPath $receipt -PathType Leaf) {
    Remove-Item -LiteralPath $receipt -Force
}
& $editor $project "-ExecutePythonScript=$python" '-unattended' '-nop4' '-nosplash'
if ($LASTEXITCODE -ne 0) {
    throw "Unreal Editor map authoring failed (exit code $LASTEXITCODE). Read the Editor log."
}
if (-not (Test-Path -LiteralPath $map -PathType Leaf)) {
    throw "Unreal exited without writing the required real project map: $map"
}
if (-not (Test-Path -LiteralPath $receipt -PathType Leaf)) {
    throw 'Unreal Editor did not write a fresh successful map-authoring receipt. Check Python/Unreal logs even if the process exited zero.'
}
# A same-size mutation after Editor save must never satisfy the
# authoring receipt. Verify explicit UE 5.8 provenance, map identity and
# cryptographic byte equality BEFORE accepting header plausibility.
& (Join-Path $PSScriptRoot 'Test-MapAuthoringReceipt.ps1') -MapPath $map -ReceiptPath $receipt
if (-not $?) { throw 'Unreal Editor map receipt did not match the saved map bytes.' }
& (Join-Path $PSScriptRoot 'Verify-UnrealMapHeader.ps1') -MapPath $map
if (-not $?) { throw 'Unreal starter map failed serialized-package header sanity check.' }
Write-Host "Unreal created/confirmed map: $map"
Write-Host 'Commit the actual binary Content/Maps/LatebrakeCircuit.umap via Git LFS if appropriate.'
Write-Host 'Next: compile Editor + game targets, cook/package and perform real installation QA.'
