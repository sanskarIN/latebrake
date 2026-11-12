<#
.SYNOPSIS
    Author real LateBrake vertex-color PBR materials in Unreal Editor 5.8.
.DESCRIPTION
    Engine-only material authoring, never game/native test certification.
    The Python asset script preserves existing assets and writes a receipt.
    This wrapper validates a fresh receipt and SHA-256 of every .uasset.
.EXAMPLE
    pwsh scripts/New-PreviewMaterials.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EngineRoot
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repository = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repository 'Latebrake.uproject'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$version = Join-Path $EngineRoot 'Engine/Build/Build.version'
$source = Join-Path $PSScriptRoot 'Unreal/CreatePreviewMaterials.py'
$receiptPath = Join-Path $repository 'Saved/LatebrakeAutomation/preview-materials-result.json'
$materialDir = Join-Path $repository 'Content/Latebrake/Materials'

foreach ($p in @($project,$editor,$version,$source)) {
    if (-not (Test-Path -LiteralPath $p -PathType Leaf)) {
        throw "Missing real Unreal input: $p"
    }
}
$engine = Get-Content -LiteralPath $version -Raw | ConvertFrom-Json -ErrorAction Stop
if ($engine.MajorVersion -ne 5 -or $engine.MinorVersion -ne 8) {
    throw "Exact Unreal 5.8 required. Engine reports $($engine.MajorVersion).$($engine.MinorVersion)"
}
$descriptor = Get-Content -LiteralPath $project -Raw | ConvertFrom-Json -ErrorAction Stop
if ($descriptor.EngineAssociation -cne '5.8') {
    throw "Project EngineAssociation must be 5.8, got $($descriptor.EngineAssociation)"
}
$project = (Resolve-Path -LiteralPath $project).Path
$source = (Resolve-Path -LiteralPath $source).Path

# Editor scripts can swallow Python exceptions and exit code 0. Reuse of a
# stale receipt would falsely imply package creation. Delete it first.
if (Test-Path -LiteralPath $receiptPath -PathType Leaf) {
    Remove-Item -LiteralPath $receiptPath -Force
}
Write-Host 'Launching actual Unreal 5.8 Editor for six visual materials.'
& $editor $project "-ExecutePythonScript=$source" '-unattended' '-nop4' '-nosplash'
if ($LASTEXITCODE -ne 0) {
    throw "Unreal Editor exited with code $LASTEXITCODE. Inspect Editor logs."
}
if (-not (Test-Path -LiteralPath $receiptPath -PathType Leaf)) {
    throw 'No fresh material receipt; do not claim Editor success.'
}
$receipt = Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json -ErrorAction Stop
if ($receipt.schema_version -ne 1 -or
    $receipt.engine_version -cne '5.8' -or
    $receipt.status -cne 'editor_material_assets_inspected' -or
    $receipt.native_gameplay_validated -ne $false -or
    $receipt.installed_cooked_game_validated -ne $false) {
    throw 'Unexpected material receipt version, status, or native approval.'
}
$names = @(
    'M_LatebrakeRoadPbr',
    'M_LatebrakeSteelPbr',
    'M_LatebrakeCarPaintPbr',
    'M_LatebrakeGlassPbr',
    'M_LatebrakeRubberPbr',
    'M_LatebrakeEmissionPbr'
)
if (@($receipt.materials).Count -ne $names.Count) {
    throw 'Editor did not report exactly six material assets.'
}
$allCreated = $true
for ($i = 0; $i -lt $names.Count; ++$i) {
    $name = $names[$i]
    $item = $receipt.materials[$i]
    $asset = "/Game/Latebrake/Materials/$name"
    $file = Join-Path $materialDir "$name.uasset"
    if ($item.asset -cne $asset) {
        throw "Unexpected material identity: $($item.asset)"
    }
    $wasCreated = $item.operation -ceq 'created_and_compiled_in_unreal_editor'
    $wasPreserved = $item.operation -ceq 'existing_unmodified_requires_visual_review'
    if (-not $wasCreated -and -not $wasPreserved) {
        throw "Unexpected asset action for $name"
    }
    $allCreated = $allCreated -and $wasCreated
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
        throw "Material was not serialized: $file"
    }
    $disk = Get-Item -LiteralPath $file -ErrorAction Stop
    if ($disk.Length -lt 256 -or $disk.Length -ne [long]$item.bytes) {
        throw "Asset length mismatch: $file"
    }
    $sha = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($sha -cne [string]$item.sha256) {
        throw "Asset SHA-256 mismatch: $file"
    }
}
if ([bool]$receipt.all_newly_compiled -ne $allCreated) {
    throw 'Material status contradicts the individual package receipts.'
}
Write-Host 'Six real serialized UE5.8 material assets match their Editor receipts.'
if (-not $allCreated) {
    Write-Warning 'Pre-existing assets were preserved; inspect their vertex-color graphs manually.'
}
Write-Warning 'No gameplay build, native performance, packaged game or physics validation has occurred.'
Write-Host 'Review the assets visually before committing actual .uasset binaries.'
