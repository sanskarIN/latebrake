<#
.SYNOPSIS
    Synthetic regression checks for cryptographically bound map receipts.
.DESCRIPTION
    Fixture files are NOT .umap assets and do not emulate licensed Unreal.
    Test only receipt schema, immutable byte identity and rejection behavior.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Expect-Rejection {
    param([string]$Name, [scriptblock]$Action)
    $rejected = $false
    try { & $Action *> $null } catch { $rejected = $true }
    if (-not $rejected) { throw "Accepted unsafe editor map receipt: $Name" }
    Write-Host "Rejected editor map receipt fixture: $Name"
}

$gate = Join-Path $PSScriptRoot 'Test-MapAuthoringReceipt.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ("LatebrakeAuthorReceipt-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null
try {
    $map = Join-Path $temp 'SYNTHETIC_NOT_UNREAL_MAP.umap'
    $receipt = Join-Path $temp 'synthetic-receipt.json'
    $bytes = [byte[]]::new(2048)
    for ($i = 0; $i -lt $bytes.Length; ++$i) { $bytes[$i] = [byte]($i % 251) }
    [IO.File]::WriteAllBytes($map, $bytes)
    $digest = (Get-FileHash -LiteralPath $map -Algorithm SHA256).Hash.ToLowerInvariant()
    $valid = @{
        schema_version = 1
        status = 'saved_by_unreal_editor'
        engine_version = '5.8'
        map = '/Game/Maps/LatebrakeCircuit'
        operation = 'created'
        bytes = 2048
        sha256 = $digest
    }

    function Set-Receipt([hashtable]$Values) {
        [IO.File]::WriteAllText($receipt, ($Values | ConvertTo-Json -Compress))
    }
    Set-Receipt $valid
    & $gate -MapPath $map -ReceiptPath $receipt
    if (-not $?) { throw 'Exactly matching synthetic receipt must pass hash policy.' }
    $valid.operation = 'loaded'
    Set-Receipt $valid
    & $gate -MapPath $map -ReceiptPath $receipt
    if (-not $?) { throw 'Previously loaded but resaved level must have valid operation.' }

    $mutated = [byte[]]::new($bytes.Length)
    [Array]::Copy($bytes, $mutated, $bytes.Length)
    $mutated[500] = [byte]($mutated[500] -bxor 1)
    [IO.File]::WriteAllBytes($map, $mutated)
    Expect-Rejection 'same byte length but different contents' {
        & $gate -MapPath $map -ReceiptPath $receipt
    }
    [IO.File]::WriteAllBytes($map, $bytes)
    & $gate -MapPath $map -ReceiptPath $receipt

    foreach ($case in @(
        @{ Name = 'stale sha'; Key = 'sha256'; Value = ('f' * 64) },
        @{ Name = 'short sha'; Key = 'sha256'; Value = ('f' * 63) },
        @{ Name = 'non-hex sha'; Key = 'sha256'; Value = ('g' * 64) },
        @{ Name = 'wrong engine'; Key = 'engine_version'; Value = '5.7' },
        @{ Name = 'wrong package'; Key = 'map'; Value = '/Game/Maps/OtherMap' },
        @{ Name = 'fake operation'; Key = 'operation'; Value = 'pretended' },
        @{ Name = 'fake status'; Key = 'status'; Value = 'not_saved' },
        @{ Name = 'wrong receipt version'; Key = 'schema_version'; Value = 2 },
        @{ Name = 'wrong size'; Key = 'bytes'; Value = 2047 },
        @{ Name = 'string instead of size'; Key = 'bytes'; Value = '2048' }
    )) {
        $bad = $valid.Clone()
        $bad[$case.Key] = $case.Value
        Set-Receipt $bad
        Expect-Rejection $case.Name {
            & $gate -MapPath $map -ReceiptPath $receipt
        }
    }

    $missing = $valid.Clone()
    $missing.Remove('sha256')
    Set-Receipt $missing
    Expect-Rejection 'missing digest' { & $gate -MapPath $map -ReceiptPath $receipt }

    [IO.File]::WriteAllText($receipt, 'NOT JSON')
    Expect-Rejection 'malformed JSON receipt' { & $gate -MapPath $map -ReceiptPath $receipt }

    Remove-Item -LiteralPath $receipt
    Expect-Rejection 'missing fresh receipt' { & $gate -MapPath $map -ReceiptPath $receipt }
    Set-Receipt $valid
    Remove-Item -LiteralPath $map
    Expect-Rejection 'missing authored map' { & $gate -MapPath $map -ReceiptPath $receipt }

    Write-Host 'All map authoring receipt tests passed (SYNTHETIC BYTES ONLY).'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
