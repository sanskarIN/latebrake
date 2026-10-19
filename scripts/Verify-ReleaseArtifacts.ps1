<#
.SYNOPSIS
    Validate evidence for real platform artifacts before a Latebrake pre-release.
.DESCRIPTION
    This checks expected files and signatures, sizes and hashes. It is NOT a
    substitute for Unreal Engine compilation, actual gameplay or device tests.
.EXAMPLE
    pwsh scripts/Verify-ReleaseArtifacts.ps1 -Win64Directory 'D:/Latebrake/Win64' -AndroidApk 'D:/Latebrake/Latebrake.apk' -EvidenceJson 'D:/Latebrake/test-evidence.json'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Win64Directory,
    [Parameter(Mandatory)][string]$AndroidApk,
    [Parameter(Mandatory)][string]$EvidenceJson
)
$ErrorActionPreference = 'Stop'
function Assert-ValidFile {
    param([string]$Path, [int64]$MinimumSize, [byte[]]$Magic)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing artifact: $Path"
    }
    $item = Get-Item -LiteralPath $Path
    if ($item.Length -lt $MinimumSize) {
        throw "Artifact appears too small ($($item.Length) bytes): $Path"
    }
    $stream = [System.IO.File]::OpenRead($item.FullName)
    try {
        for ($i = 0; $i -lt $Magic.Length; $i++) {
            if ($stream.ReadByte() -ne $Magic[$i]) {
                throw "Invalid executable/package signature: $Path"
            }
        }
    } finally {
        $stream.Dispose()
    }
    $hash = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Output "$($item.Name) | $($item.Length) bytes | SHA256 $hash"
}
if (-not (Test-Path -LiteralPath $Win64Directory -PathType Container)) {
    throw "Win64 package directory does not exist: $Win64Directory"
}
$exe = Get-ChildItem -LiteralPath $Win64Directory -Filter '*Latebrake*.exe' -File -Recurse |
    Select-Object -First 1
if ($null -eq $exe) {
    throw 'No Latebrake Windows executable found'
}
$paks = @(Get-ChildItem -LiteralPath $Win64Directory -File -Recurse |
    Where-Object { $_.Extension -in '.pak', '.ucas' })
if ($paks.Count -eq 0) {
    throw 'No cooked Unreal game content (.pak/.ucas) found in Windows package'
}
if (-not (Test-Path -LiteralPath $EvidenceJson -PathType Leaf)) {
    throw 'Release evidence JSON is missing'
}
$evidence = Get-Content -LiteralPath $EvidenceJson -Raw | ConvertFrom-Json -ErrorAction Stop
if ($evidence.windows_playtested -ne $true -or $evidence.android_playtested -ne $true -or
    [string]::IsNullOrWhiteSpace([string]$evidence.unreal_version) -or
    [string]::IsNullOrWhiteSpace([string]$evidence.windows_hardware) -or
    [string]::IsNullOrWhiteSpace([string]$evidence.android_device)) {
    throw 'Evidence must record verified playtests, engine version, Windows hardware and Android device'
}
Write-Output 'Windows application:'
Assert-ValidFile -Path $exe.FullName -MinimumSize 102400 -Magic ([byte[]](0x4d,0x5a))
Write-Output 'Android APK:'
Assert-ValidFile -Path $AndroidApk -MinimumSize 1048576 -Magic ([byte[]](0x50,0x4b))
Write-Output 'Package structure and human test evidence present. Actual game behavior is not automatically certified by this script.'
