<#
.SYNOPSIS
  On a licensed UE5.8 Windows workstation, inspect one original 1.7-km
  full-width road as a *static complex-as-simple* collision mesh.
.DESCRIPTION
  This NEVER changes LatebrakeCircuit, the original 68-slab laboratory,
  career/gameplay maps or source releases. This is NOT a cooked/installed
  playable build and does NOT validate actual Chaos tire contact.
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EngineRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repo 'Latebrake.uproject'
$engine = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$versionFile = Join-Path $EngineRoot 'Engine/Build/Build.version'
$script = Join-Path $PSScriptRoot 'Unreal/ImportWorldScaleContinuousRoad.py'
$verifier = Join-Path $repo 'Tests/Standalone/VerifyWorldScaleCollisionRoadObj.py'
$build = Join-Path $repo 'out/ContinuousRoadLabCmake'
$source = Join-Path $repo 'Tests/Standalone'
$dataFolder = Join-Path $repo 'Saved/LatebrakeAutomation'
$obj = Join-Path $dataFolder 'SM_WorldScaleContinuousRoad.obj'
$mtl = Join-Path $dataFolder 'SM_WorldScaleContinuousRoad.mtl'
$receiptFile = Join-Path $dataFolder 'world-scale-continuous-road-import-result.json'
$map = Join-Path $repo 'Content/Maps/LatebrakeWorldScaleContinuousCollisionLab.umap'
$asset = Join-Path $repo 'Content/LatebrakeExperimental/SM_WorldScaleContinuousRoad.uasset'
$exportTarget = 'latebrake_world_scale_collision_obj_exporter'

foreach ($input in @($project, $engine, $versionFile, $script, $verifier)) {
    if (-not (Test-Path -LiteralPath $input -PathType Leaf)) {
        throw "Missing genuine continuous road import input: $input"
    }
}
$engineVersion = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
$projectDescriptor = Get-Content -LiteralPath $project -Raw | ConvertFrom-Json
if ($engineVersion.MajorVersion -ne 5 -or $engineVersion.MinorVersion -ne 8 -or
    $projectDescriptor.EngineAssociation -cne '5.8') {
    throw 'Independent continuous road Editor inspection requires UE 5.8 exactly.'
}
if ((Test-Path -LiteralPath $map) -or (Test-Path -LiteralPath $asset)) {
    throw 'Independent continuous road map/asset already exists; never overwrite user content.'
}
& cmake -S $source -B $build -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw 'Source CMake configure failed.' }
& cmake --build $build --config Release --target $exportTarget latebrake_world_scale_smooth_collision_tests --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Full-width C++ geometry/exporter build failed.' }
Push-Location $build
try {
    & ctest -C Release -R '^WorldScale(SmoothCollisionSurface|CollisionRoadObjContract)$' --no-tests=error --output-on-failure
    if ($LASTEXITCODE -ne 0) {
        throw 'Continuous physical surface or independent exported OBJ contract failed.'
    }
} finally {
    Pop-Location
}
$executables = @(
    (Join-Path $build "Release/$exportTarget.exe"),
    (Join-Path $build "$exportTarget.exe")
)
$exporter = $executables | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
    Select-Object -First 1
if (-not $exporter) { throw 'Expected original compiled OBJ generator absent.' }
if (-not (Test-Path -LiteralPath $dataFolder)) {
    New-Item -ItemType Directory -Path $dataFolder -Force | Out-Null
}
& $exporter $obj
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $obj -PathType Leaf) -or
    -not (Test-Path -LiteralPath $mtl -PathType Leaf)) {
    throw 'Compiled C++ continuous physical road source OBJ/MTL export failed.'
}
$objHash = (Get-FileHash -LiteralPath $obj -Algorithm SHA256).Hash.ToLowerInvariant()
$mtlHash = (Get-FileHash -LiteralPath $mtl -Algorithm SHA256).Hash.ToLowerInvariant()
# Previous Editor failures must not be mistaken for successful source state.
if (Test-Path -LiteralPath $receiptFile -PathType Leaf) {
    Remove-Item -LiteralPath $receiptFile -Force
}
$objKey = 'LATEBRAKE_CONTINUOUS_ROAD_OBJ_SHA256'
$mtlKey = 'LATEBRAKE_CONTINUOUS_ROAD_MTL_SHA256'
$previousObj = [Environment]::GetEnvironmentVariable($objKey, 'Process')
$previousMtl = [Environment]::GetEnvironmentVariable($mtlKey, 'Process')
try {
    [Environment]::SetEnvironmentVariable($objKey, $objHash, 'Process')
    [Environment]::SetEnvironmentVariable($mtlKey, $mtlHash, 'Process')
    & $engine (Resolve-Path -LiteralPath $project).Path "-ExecutePythonScript=$script" '-unattended' '-nop4' '-nosplash'
    $exitCode = $LASTEXITCODE
} finally {
    [Environment]::SetEnvironmentVariable($objKey, $previousObj, 'Process')
    [Environment]::SetEnvironmentVariable($mtlKey, $previousMtl, 'Process')
}
if ($exitCode -ne 0 -or
    -not (Test-Path -LiteralPath $map -PathType Leaf) -or
    -not (Test-Path -LiteralPath $asset -PathType Leaf) -or
    -not (Test-Path -LiteralPath $receiptFile -PathType Leaf)) {
    throw 'Unreal Editor failed to persist a REAL isolated collision mesh/map + receipt.'
}
$r = Get-Content -LiteralPath $receiptFile -Raw | ConvertFrom-Json
$mapHash = (Get-FileHash -LiteralPath $map -Algorithm SHA256).Hash.ToLowerInvariant()
$assetHash = (Get-FileHash -LiteralPath $asset -Algorithm SHA256).Hash.ToLowerInvariant()
if ($r.schema_version -ne 1 -or
    $r.status -cne 'saved_and_reloaded_by_unreal_editor' -or
    $r.engine_version -cne '5.8' -or
    $r.map -cne '/Game/Maps/LatebrakeWorldScaleContinuousCollisionLab' -or
    $r.road_mesh -cne '/Game/LatebrakeExperimental/SM_WorldScaleContinuousRoad' -or
    $r.source_obj_sha256 -cne $objHash -or
    $r.source_mtl_sha256 -cne $mtlHash -or
    $r.map_sha256 -cne $mapHash -or
    $r.asset_sha256 -cne $assetHash -or
    $r.map_bytes -ne (Get-Item -LiteralPath $map).Length -or
    $r.asset_bytes -ne (Get-Item -LiteralPath $asset).Length -or
    $r.actors_audited_after_reload -ne 2 -or
    $r.physical_road_width_cm -ne 900 -or
    $r.collision_profile -cne 'BlockAll' -or
    $r.collision_trace_flag -cne 'CTF_USE_COMPLEX_AS_SIMPLE' -or
    $r.static_road -ne $true -or $r.race_game_mode_disabled -ne $true -or
    $r.road_chaos_contact_tested -ne $false -or
    $r.actual_wheel_qa -cne 'NOT_RUN') {
    throw 'Fresh Unreal collision-road source/asset/map evidence does NOT match.'
}
& (Join-Path $PSScriptRoot 'Verify-UnrealMapHeader.ps1') -MapPath $map
if (-not $?) { throw 'New Unreal lab failed genuine map-header validation.' }
Write-Host "Verified isolated continuous physical-road UE Editor source import: $map"
Write-Host "Original static mesh: $asset"
Write-Host 'NOT a cooked collision/vehicle-contact QA result or playable game build.'
