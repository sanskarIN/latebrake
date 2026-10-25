<#
.SYNOPSIS
    Verify the exact map bytes attested by a fresh licensed Unreal Editor run.
.DESCRIPTION
    This checks versioned authoring receipt fields and SHA256 byte identity.
    It cannot prove that the receipt was emitted by a genuine UE installation,
    that a valid .umap cooks, or that the resulting game is playable.
    New-StarterMap.ps1 removes the previous receipt before launching Unreal,
    verifies engine/project versions and checks the Unreal package header.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$MapPath,
    [Parameter(Mandatory)][string]$ReceiptPath
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $MapPath -PathType Leaf)) {
    throw "Serialized starter map missing: $MapPath"
}
if (-not (Test-Path -LiteralPath $ReceiptPath -PathType Leaf)) {
    throw "Fresh editor map-authoring receipt missing: $ReceiptPath"
}
$map = Get-Item -LiteralPath $MapPath -Force -ErrorAction Stop
$receiptFile = Get-Item -LiteralPath $ReceiptPath -Force -ErrorAction Stop
if (($map.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0 -or
    ($receiptFile.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'Map and receipt must be ordinary files, not symbolic links or reparse points.'
}
if ($map.Length -lt 1024) {
    throw 'Map is too small to be an authored Unreal binary.'
}
try {
    $receipt = Get-Content -LiteralPath $receiptFile.FullName -Raw -ErrorAction Stop |
        ConvertFrom-Json -ErrorAction Stop
} catch {
    throw "Invalid editor map-authoring JSON receipt: $_"
}
if ($null -eq $receipt -or
    $receipt.schema_version -isnot [long] -and $receipt.schema_version -isnot [int] -or
    $receipt.schema_version -ne 1 -or
    $receipt.status -cne 'saved_by_unreal_editor' -or
    $receipt.engine_version -cne '5.8' -or
    $receipt.map -cne '/Game/Maps/LatebrakeCircuit' -or
    $receipt.operation -cnotin @('created', 'loaded') -or
    ($receipt.bytes -isnot [long] -and $receipt.bytes -isnot [int]) -or
    $receipt.bytes -ne $map.Length -or
    $receipt.sha256 -isnot [string] -or
    $receipt.sha256 -cnotmatch '^[a-f0-9]{64}$') {
    throw 'Editor map receipt has incompatible schema, engine, map identity, operation, length or digest.'
}

& (Join-Path $PSScriptRoot 'Test-CandidateMapSnapshot.ps1') -MapPath $map.FullName -ExpectedSha256 $receipt.sha256
if (-not $?) { throw 'Editor receipt hash differs from the actual serialized map.' }
Write-Host "Unreal map authoring receipt matches actual saved bytes: $($receipt.sha256)"
Write-Host 'NOT an Unreal engine compile, successful cook, map load or gameplay qualification.'
