<#
.SYNOPSIS
    Validate a fresh Unreal Editor material-authoring receipt and exact bytes.
.DESCRIPTION
    Structural SHA256 policy only. Passing does not prove the bytes were
    written by licensed Unreal or that the shader renders correctly.
    New-RacingLineMaterial.ps1 deletes any prior receipt before launching UE.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$MaterialPath,
    [Parameter(Mandatory)][string]$ReceiptPath
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $MaterialPath -PathType Leaf)) {
    throw "Genuine material asset is missing: $MaterialPath"
}
if (-not (Test-Path -LiteralPath $ReceiptPath -PathType Leaf)) {
    throw "Fresh Unreal material-authoring receipt is missing: $ReceiptPath"
}
$material = Get-Item -LiteralPath $MaterialPath -Force -ErrorAction Stop
$receiptItem = Get-Item -LiteralPath $ReceiptPath -Force -ErrorAction Stop
if (($material.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or
    ($receiptItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'Material and receipt must be ordinary files, not symbolic links.'
}
if ($material.Length -lt 1024) {
    throw 'Saved material is too small for a plausible serialized Unreal asset.'
}
try {
    $record = Get-Content -LiteralPath $receiptItem.FullName -Raw -ErrorAction Stop |
        ConvertFrom-Json -ErrorAction Stop
} catch {
    throw "Unreadable material-authoring JSON receipt: $_"
}
if ($null -eq $record -or
    ($record.schema_version -isnot [long] -and $record.schema_version -isnot [int]) -or
    $record.schema_version -ne 1 -or
    $record.status -cne 'saved_by_unreal_editor' -or
    $record.engine_version -cne '5.8' -or
    $record.asset -cne '/Game/Materials/M_RacingLineGuide' -or
    $record.operation -cnotin @('created', 'validated_existing') -or
    $record.color_parameter -cne 'Color' -or
    $record.instanced_mesh_usage -isnot [bool] -or
    $record.instanced_mesh_usage -ne $true -or
    ($record.bytes -isnot [long] -and $record.bytes -isnot [int]) -or
    $record.bytes -ne $material.Length -or
    $record.sha256 -isnot [string] -or
    $record.sha256 -cnotmatch '^[a-f0-9]{64}$') {
    throw 'Material receipt has invalid schema, engine, asset, graph contract, length or digest.'
}
$digest = (Get-FileHash -LiteralPath $material.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
if ($digest -cne $record.sha256) {
    throw 'Material authoring receipt SHA256 does not match the exact file bytes.'
}
Write-Host "Material file and fresh Editor receipt hashes match: $digest"
Write-Host 'SOURCE/FILE ONLY: not UE compile, valid shader pixels, cook or installed game.'
