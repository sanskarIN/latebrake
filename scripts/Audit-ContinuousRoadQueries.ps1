<#
.SYNOPSIS
  On a licensed UE 5.8 Windows workstation, run 2,322 real Editor
  static-complex line queries over the independent continuous road lab.
.DESCRIPTION
  Requires the separate road asset+map created by New-ContinuousRoadLab.
  Does NOT create or mutate the live race/career map, test dynamic Chaos
  tires or wheel suspension, build UE Game, or package a Windows installer.
#>
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EngineRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Latebrake.uproject'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$versionFile = Join-Path $EngineRoot 'Engine/Build/Build.version'
$script = Join-Path $PSScriptRoot 'Unreal/AuditWorldScaleContinuousRoad.py'
$generator = Join-Path $root 'Tests/Standalone/WorldScaleCollisionTracePlan.py'
$folder = Join-Path $root 'Saved/LatebrakeAutomation'
$plan = Join-Path $folder 'world-scale-continuous-ray-probes.json'
$result = Join-Path $folder 'world-scale-continuous-ray-audit-result.json'
$import = Join-Path $folder 'world-scale-continuous-road-import-result.json'
$map = Join-Path $root 'Content/Maps/LatebrakeWorldScaleContinuousCollisionLab.umap'
$asset = Join-Path $root 'Content/LatebrakeExperimental/SM_WorldScaleContinuousRoad.uasset'
$obj = Join-Path $folder 'SM_WorldScaleContinuousRoad.obj'
$mtl = Join-Path $folder 'SM_WorldScaleContinuousRoad.mtl'

foreach ($input in @($project, $editor, $versionFile, $script, $generator,
                      $import, $map, $asset, $obj, $mtl)) {
    if (-not (Test-Path -LiteralPath $input -PathType Leaf)) {
        throw "Required genuine independent collision-query input missing: $input"
    }
}
$version = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
$descriptor = Get-Content -LiteralPath $project -Raw | ConvertFrom-Json
if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 8 -or
    $descriptor.EngineAssociation -cne '5.8') {
    throw 'Static physical-road query audit requires genuine Unreal Engine 5.8.'
}
if (Test-Path -LiteralPath $result) {
    throw 'Existing physics-query evidence must be preserved; move/review manually first.'
}
if (Test-Path -LiteralPath $plan) {
    throw 'Existing signed ray-plan input must be reviewed before regeneration.'
}

$previousReceipt = Get-Content -LiteralPath $import -Raw | ConvertFrom-Json
$originalMapHash = (Get-FileHash -LiteralPath $map -Algorithm SHA256).Hash.ToLowerInvariant()
$originalAssetHash = (Get-FileHash -LiteralPath $asset -Algorithm SHA256).Hash.ToLowerInvariant()
$originalObjHash = (Get-FileHash -LiteralPath $obj -Algorithm SHA256).Hash.ToLowerInvariant()
$originalMtlHash = (Get-FileHash -LiteralPath $mtl -Algorithm SHA256).Hash.ToLowerInvariant()
if ($previousReceipt.status -cne 'saved_and_reloaded_by_unreal_editor' -or
    $previousReceipt.engine_version -cne '5.8' -or
    $previousReceipt.map_sha256 -cne $originalMapHash -or
    $previousReceipt.asset_sha256 -cne $originalAssetHash -or
    $previousReceipt.source_obj_sha256 -cne $originalObjHash -or
    $previousReceipt.source_mtl_sha256 -cne $originalMtlHash -or
    $previousReceipt.road_chaos_contact_tested -ne $false -or
    $previousReceipt.actual_wheel_qa -cne 'NOT_RUN') {
    throw 'Real UE5.8 static road import provenance is missing or has drifted.'
}
& python $generator $plan
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $plan -PathType Leaf)) {
    throw 'Independent full-lap static-complex query-plan generation failed.'
}
$probe = Get-Content -LiteralPath $plan -Raw | ConvertFrom-Json
if ($probe.schema_version -ne 1 -or
    $probe.sample_count -ne 2322 -or @($probe.samples).Count -ne 2322 -or
    $probe.physical_road_width_cm -ne 900 -or
    $probe.station_count -ne 256 -or $probe.extra_seam_stations -ne 2 -or
    $probe.units -cne 'cm') {
    throw 'Source ray-query plan does not cover whole physical route and lap seam.'
}
$planSHA = (Get-FileHash -LiteralPath $plan -Algorithm SHA256).Hash.ToLowerInvariant()
$envKey = 'LATEBRAKE_CONTINUOUS_RAY_PLAN_SHA256'
$oldEnv = [Environment]::GetEnvironmentVariable($envKey, 'Process')
try {
    [Environment]::SetEnvironmentVariable($envKey, $planSHA, 'Process')
    & $editor (Resolve-Path -LiteralPath $project).Path "-ExecutePythonScript=$script" '-unattended' '-nop4' '-nosplash'
    $editorExit = $LASTEXITCODE
} finally {
    [Environment]::SetEnvironmentVariable($envKey, $oldEnv, 'Process')
}
if ($editorExit -ne 0 -or -not (Test-Path -LiteralPath $result -PathType Leaf)) {
    throw 'Genuine UE5.8 Editor did NOT successfully verify 2,322 static collision queries.'
}
$r = Get-Content -LiteralPath $result -Raw | ConvertFrom-Json
if ($r.schema_version -ne 1 -or
    $r.status -cne 'genuine_ue58_static_complex_line_queries_passed' -or
    $r.engine_version -cne '5.8' -or
    $r.map -cne '/Game/Maps/LatebrakeWorldScaleContinuousCollisionLab' -or
    $r.road_mesh -cne '/Game/LatebrakeExperimental/SM_WorldScaleContinuousRoad' -or
    $r.query_plan_sha256 -cne $planSHA -or
    $r.ray_queries_performed -ne 2322 -or
    $r.actual_road_hits -ne 1290 -or $r.actual_ground_hits -ne 1032 -or
    $r.max_height_error_cm -lt 0 -or $r.max_height_error_cm -gt 1.5 -or
    $r.minimum_upward_normal_z -lt 0.98 -or $r.minimum_upward_normal_z -gt 1.001 -or
    $r.query_complex -ne $true -or $r.import_asset_static -ne $true -or
    $r.runtime_track_enabled -ne $false -or
    $r.playable_game_tested -ne $false -or
    $r.dynamic_chaos_wheel_contact_tested -ne $false -or
    $r.map_sha256 -cne $originalMapHash -or
    $r.asset_sha256 -cne $originalAssetHash -or
    $r.source_obj_sha256 -cne $originalObjHash -or
    $r.source_mtl_sha256 -cne $originalMtlHash -or
    (Get-FileHash -LiteralPath $map -Algorithm SHA256).Hash.ToLowerInvariant() -cne $originalMapHash -or
    (Get-FileHash -LiteralPath $asset -Algorithm SHA256).Hash.ToLowerInvariant() -cne $originalAssetHash -or
    (Get-FileHash -LiteralPath $plan -Algorithm SHA256).Hash.ToLowerInvariant() -cne $planSHA) {
    throw 'Unreal Editor road collision ray evidence does not match source, asset or 2322 physical queries.'
}
Write-Host 'Genuine UE5.8 STATIC-COMPLEX collision ray-query evidence verified.'
Write-Host "1290 road hits + 1032 off-road ground hits; road/map/OBJ/probe identities hash-pinned."
Write-Host "Receipt: $result"
Write-Host 'Still NOT verified: dynamic Chaos tire contact, driving, cooking, physics performance, or playable Win64 EXE.'
