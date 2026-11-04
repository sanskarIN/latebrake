<#
.SYNOPSIS
    Reject invalid synthetic racing-line material receipts and mutated bytes.
.DESCRIPTION
    Fixtures are NOT Unreal .uasset files; no licensed Editor or game runs.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Expect-Rejection {
    param([string]$Case, [scriptblock]$Action)
    $rejected = $false
    try { & $Action *> $null } catch { $rejected = $true }
    if (-not $rejected) { throw "Accepted an invalid guide material receipt: $Case" }
    Write-Host "Rejected material receipt fixture: $Case"
}

$gate = Join-Path $PSScriptRoot 'Test-RacingLineMaterialReceipt.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ('LateBrakeMaterialReceipt-' + [Guid]::NewGuid().ToString('N'))
[void](New-Item -Path $temp -ItemType Directory -Force)
try {
    $material = Join-Path $temp 'SYNTHETIC_NOT_UNREAL_MATERIAL.uasset'
    $receipt = Join-Path $temp 'synthetic-editor-receipt.json'
    $bytes = [byte[]]::new(2048)
    for ($i = 0; $i -lt $bytes.Length; ++$i) { $bytes[$i] = [byte]($i % 241) }
    [IO.File]::WriteAllBytes($material, $bytes)
    $digest = (Get-FileHash -LiteralPath $material -Algorithm SHA256).Hash.ToLowerInvariant()
    $valid = @{
        schema_version = 1
        status = 'saved_by_unreal_editor'
        engine_version = '5.8'
        asset = '/Game/Materials/M_RacingLineGuide'
        operation = 'created'
        color_parameter = 'Color'
        instanced_mesh_usage = $true
        bytes = 2048
        sha256 = $digest
    }
    function Write-Receipt([hashtable]$Value) {
        [IO.File]::WriteAllText($receipt, ($Value | ConvertTo-Json -Compress))
    }
    Write-Receipt $valid
    & $gate -MaterialPath $material -ReceiptPath $receipt
    if (-not $?) { throw 'Matching synthetic material/receipt must pass integrity policy.' }
    $valid.operation = 'validated_existing'
    Write-Receipt $valid
    & $gate -MaterialPath $material -ReceiptPath $receipt
    if (-not $?) { throw 'Validated-existing material receipt must match saved bytes.' }

    $mutated = [byte[]]::new($bytes.Length)
    [Array]::Copy($bytes, $mutated, $bytes.Length)
    $mutated[620] = [byte]($mutated[620] -bxor 1)
    [IO.File]::WriteAllBytes($material, $mutated)
    Expect-Rejection 'mutated same-length material bytes' {
        & $gate -MaterialPath $material -ReceiptPath $receipt
    }
    [IO.File]::WriteAllBytes($material, $bytes)

    foreach ($case in @(
        @{ Label='wrong engine'; Key='engine_version'; Value='5.7' },
        @{ Label='wrong asset'; Key='asset'; Value='/Game/Materials/Other' },
        @{ Label='fake operation'; Key='operation'; Value='fake' },
        @{ Label='wrong version'; Key='schema_version'; Value=2 },
        @{ Label='wrong status'; Key='status'; Value='not_saved' },
        @{ Label='bad Color parameter'; Key='color_parameter'; Value='Other' },
        @{ Label='disabled instancing'; Key='instanced_mesh_usage'; Value=$false },
        @{ Label='string instancing'; Key='instanced_mesh_usage'; Value='true' },
        @{ Label='wrong size'; Key='bytes'; Value=2047 },
        @{ Label='string size'; Key='bytes'; Value='2048' },
        @{ Label='tampered checksum'; Key='sha256'; Value=('f' * 64) }
    )) {
        $bad = $valid.Clone()
        $bad[$case.Key] = $case.Value
        Write-Receipt $bad
        Expect-Rejection $case.Label {
            & $gate -MaterialPath $material -ReceiptPath $receipt
        }
    }
    [IO.File]::WriteAllText($receipt, 'NOT JSON')
    Expect-Rejection 'invalid JSON' { & $gate -MaterialPath $material -ReceiptPath $receipt }
    Remove-Item -LiteralPath $receipt -Force
    Expect-Rejection 'missing receipt' { & $gate -MaterialPath $material -ReceiptPath $receipt }
    Write-Receipt $valid
    Remove-Item -LiteralPath $material -Force
    Expect-Rejection 'missing material' { & $gate -MaterialPath $material -ReceiptPath $receipt }

    Write-Host 'All material authoring receipt negative fixtures passed (NOT an Unreal asset).'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
