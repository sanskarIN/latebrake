<#
.SYNOPSIS
    Create a genuine original LateBrake track material in Unreal Editor 5.8.
.DESCRIPTION
    This is an optional AUTHORED ART stage, not a game build. Never fabricate
    .uasset bytes outside Unreal. The parent does not alter live race physics.
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EngineRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repo 'Latebrake.uproject'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$versionFile = Join-Path $EngineRoot 'Engine/Build/Build.version'
$script = Join-Path $PSScriptRoot 'Unreal/CreateTrackMaterial.py'
$asset = Join-Path $repo 'Content/Latebrake/Materials/M_LatebrakeTrackPBR.uasset'
$receipt = Join-Path $repo 'Saved/LatebrakeAutomation/track-pbr-result.json'

foreach ($required in @($project, $editor, $versionFile, $script)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Mandatory Unreal asset-authoring input unavailable: $required"
    }
}
$engine = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
if ($engine.MajorVersion -ne 5 -or $engine.MinorVersion -ne 8) {
    throw 'Track PBR asset generation requires Unreal Engine version 5.8.'
}
$projectVersion = Get-Content -LiteralPath $project -Raw | ConvertFrom-Json
if ($projectVersion.EngineAssociation -cne '5.8') {
    throw 'Project EngineAssociation does not match required 5.8.'
}

# A previously successful receipt must never certify a different run.
if (Test-Path -LiteralPath $receipt -PathType Leaf) {
    Remove-Item -LiteralPath $receipt -Force
}
$projectPath = (Resolve-Path -LiteralPath $project).Path
$scriptPath = (Resolve-Path -LiteralPath $script).Path
& $editor $projectPath "-ExecutePythonScript=$scriptPath" '-unattended' '-nop4' '-nosplash'
if ($LASTEXITCODE -ne 0) {
    throw "Unreal track material authoring failed (exit $LASTEXITCODE)."
}
if (-not (Test-Path -LiteralPath $asset -PathType Leaf) -or
    -not (Test-Path -LiteralPath $receipt -PathType Leaf)) {
    throw 'The real Unreal material asset and fresh receipt must both exist.'
}
$proof = Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
$size = (Get-Item -LiteralPath $asset).Length
$hash = (Get-FileHash -LiteralPath $asset -Algorithm SHA256).Hash.ToLowerInvariant()
if ($proof.schema_version -ne 1 -or
    $proof.status -cne 'saved_by_unreal_editor' -or
    $proof.engine_version -cne '5.8' -or
    $proof.asset -cne '/Game/Latebrake/Materials/M_LatebrakeTrackPBR' -or
    $proof.operation -cnotin @('created','reused') -or
    $size -lt 1024 -or $proof.bytes -ne $size -or
    $proof.sha256 -cne $hash) {
    throw 'The fresh Unreal PBR asset receipt does not match the saved bytes.'
}
Write-Host "Real original Unreal material: $asset"
Write-Host 'This is source-authored ART ONLY, not verified cooked gameplay.'
Write-Host 'Review the material and package it with an actual Unreal 5.8 Win64 candidate.'
