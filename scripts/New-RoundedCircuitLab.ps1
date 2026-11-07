<#
.SYNOPSIS
    Build a separate UE5.8 rounded collision-road Editor physics-lab map.
.DESCRIPTION
    Requires licensed Unreal Editor 5.8 and C++20/CMake. This is NOT a
    playable game, Windows installer or replacement for LatebrakeCircuit.
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EngineRoot, [switch]$WorldScale)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repo 'Latebrake.uproject'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$version = Join-Path $EngineRoot 'Engine/Build/Build.version'
$python = Join-Path $PSScriptRoot 'Unreal/CreateRoundedCircuitLab.py'
$build = Join-Path $repo 'out/RoundedPhysicsLabCmake'
$source = Join-Path $repo 'Tests/Standalone'
$data = Join-Path $repo 'Saved/LatebrakeAutomation/rounded-road-pieces.json'
$receipt = Join-Path $repo 'Saved/LatebrakeAutomation/rounded-physics-lab-result.json'
$map = Join-Path $repo 'Content/Maps/LatebrakeRoundedPhysicsLab.umap'
$exportTarget = 'latebrake_rounded_circuit_lab_exporter'
$labPackage = '/Game/Maps/LatebrakeRoundedPhysicsLab'
$labVariant = 'prototype'
if ($WorldScale) {
    $exportTarget = 'latebrake_world_scale_rounded_circuit_lab_exporter'
    $labPackage = '/Game/Maps/LatebrakeWorldScaleRoundedPhysicsLab'
    $labVariant = 'world_scale'
    $data = Join-Path $repo 'Saved/LatebrakeAutomation/world-scale-rounded-road-pieces.json'
    $receipt = Join-Path $repo 'Saved/LatebrakeAutomation/world-scale-rounded-physics-lab-result.json'
    $map = Join-Path $repo 'Content/Maps/LatebrakeWorldScaleRoundedPhysicsLab.umap'
}
foreach ($path in @($project, $editor, $version, $python)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Unreal Editor physics-lab input unavailable: $path"
    }
}
$engineVersion = Get-Content -LiteralPath $version -Raw | ConvertFrom-Json
$descriptor = Get-Content -LiteralPath $project -Raw | ConvertFrom-Json
if ($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 8 -or
    $descriptor.EngineAssociation -cne '5.8') {
    throw 'Isolated physics lab requires licensed Unreal Engine 5.8.'
}
if (Test-Path -LiteralPath $map -PathType Any) {
    throw 'Rounded physics lab exists; refuse to overwrite or alter user map.'
}
& cmake -S $source -B $build -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& cmake --build $build --config Release --target $exportTarget --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Canonical rounded road C++ exporter build failed.' }
$candidates = @(
    (Join-Path $build "Release/$exportTarget.exe"),
    (Join-Path $build "$exportTarget.exe")
)
$exporter = $candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
if (-not $exporter) { throw 'Compiled rounded road exporter EXE missing.' }
$payload = & $exporter
if ($LASTEXITCODE -ne 0 -or @($payload).Count -ne 1) {
    throw 'Canonical road exporter did not return one valid JSON row.'
}
$parsed = $payload | ConvertFrom-Json -ErrorAction Stop
if ($parsed.schema_version -ne 1 -or $parsed.piece_count -ne 68 -or
    @($parsed.pieces).Count -ne 68 -or $parsed.units -cne 'cm') {
    throw 'Rounded collision JSON has incorrect shape or units.'
}
$dir = Split-Path -Parent $data
if (-not (Test-Path -LiteralPath $dir)) {
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
}
[IO.File]::WriteAllText($data, [string]$payload + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
if (Test-Path -LiteralPath $receipt -PathType Leaf) {
    Remove-Item -LiteralPath $receipt -Force
}
$expectedDataHash = (Get-FileHash -LiteralPath $data -Algorithm SHA256).Hash.ToLowerInvariant()
$projectPath = (Resolve-Path -LiteralPath $project).Path
$scriptPath = (Resolve-Path -LiteralPath $python).Path
# Pass the identity of the *actual compiled C++ export* to the Editor.
# A valid-looking but mutated 68-element JSON is not an authorized road.
$envName = 'LATEBRAKE_ROUNDED_GEOMETRY_SHA256'
$originalEnvironment = [Environment]::GetEnvironmentVariable($envName, 'Process')
$modeEnvName = 'LATEBRAKE_ROUNDED_LAB_MODE'
$originalMode = [Environment]::GetEnvironmentVariable($modeEnvName, 'Process')
try {
    [Environment]::SetEnvironmentVariable($envName, $expectedDataHash, 'Process')
    [Environment]::SetEnvironmentVariable($modeEnvName, $labVariant, 'Process')
    & $editor $projectPath "-ExecutePythonScript=$scriptPath" '-unattended' '-nop4' '-nosplash'
    $editorExitCode = $LASTEXITCODE
} finally {
    [Environment]::SetEnvironmentVariable($envName, $originalEnvironment, 'Process')
    [Environment]::SetEnvironmentVariable($modeEnvName, $originalMode, 'Process')
}
if ($editorExitCode -ne 0) { throw 'Genuine Unreal Editor map authoring failed.' }
if (-not (Test-Path -LiteralPath $map -PathType Leaf) -or
    -not (Test-Path -LiteralPath $receipt -PathType Leaf)) {
    throw 'Unreal did not produce both a genuine physics-lab map and fresh receipt.'
}
$r = Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
$mapHash = (Get-FileHash -LiteralPath $map -Algorithm SHA256).Hash.ToLowerInvariant()
$dataHash = (Get-FileHash -LiteralPath $data -Algorithm SHA256).Hash.ToLowerInvariant()
if ($r.schema_version -ne 1 -or $r.status -cne 'saved_by_unreal_editor' -or
    $r.engine_version -cne '5.8' -or
    $r.map -cne $labPackage -or $r.lab_variant -cne $labVariant -or
    $r.pieces -ne 68 -or $r.ground_pieces -ne 1 -or
    $r.audited_actors -ne 69 -or $r.verified_after_reload -ne $true -or
    $r.game_mode -cne 'GameModeBase' -or
    $r.source_identity_verified -ne $true -or
    $r.geometry_sha256 -cne $expectedDataHash -or
    $r.geometry_sha256 -cne $dataHash -or
    $r.map_sha256 -cne $mapHash -or
    $r.map_bytes -ne (Get-Item -LiteralPath $map).Length) {
    throw 'Lab map or C++ source export failed fresh hash-linked evidence.'
}
& (Join-Path $PSScriptRoot 'Verify-UnrealMapHeader.ps1') -MapPath $map
if (-not $?) { throw 'Serialized Unreal map header check failed.' }
Write-Host "Original collision-road physics lab ($labVariant): $map"
Write-Host '68 physical road pieces and one ground; rendering and driving NOT verified.'
