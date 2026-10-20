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
$python = Join-Path $PSScriptRoot 'Unreal/CreateStarterMap.py'
$map = Join-Path $repo 'Content/Maps/LatebrakeCircuit.umap'

if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
    throw "Unreal project missing: $project"
}
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) {
    throw "Unreal Editor executable not found: $editor. Install authorized UE 5.8 first."
}
if (-not (Test-Path -LiteralPath $python -PathType Leaf)) {
    throw "Editor map creation script missing: $python"
}

$project = (Resolve-Path -LiteralPath $project).Path
$python = (Resolve-Path -LiteralPath $python).Path

Write-Host "Launching actual Unreal Editor to create/check /Game/Maps/LatebrakeCircuit."
Write-Host 'Editor-only Python Script and Editor Scripting Utilities plugins must be available.'
& $editor $project "-ExecutePythonScript=$python" '-unattended' '-nop4' '-nosplash'
if ($LASTEXITCODE -ne 0) {
    throw "Unreal Editor map authoring failed (exit code $LASTEXITCODE). Read the Editor log."
}
if (-not (Test-Path -LiteralPath $map -PathType Leaf)) {
    throw "Unreal exited without writing the required real project map: $map"
}
if ((Get-Item -LiteralPath $map).Length -lt 1024) {
    throw 'Project map is too small to be considered a plausible serialized Unreal level.'
}
Write-Host "Unreal created/confirmed map: $map"
Write-Host 'Commit the actual binary Content/Maps/LatebrakeCircuit.umap via Git LFS if appropriate.'
Write-Host 'Next: compile Editor + game targets, cook/package and perform real installation QA.'
