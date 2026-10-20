<#
.SYNOPSIS
    Validate an individually packaged Unreal platform build before any upload.
.DESCRIPTION
    This deliberately refuses a source archive or dummy renamed EXE/APK.
    Hashes and structure checks cannot replace manual installed playtests.
.EXAMPLE
    pwsh scripts/Verify-PlatformPackage.ps1 -Platform Win64 -PackagePath 'D:/LatebrakeBuilds/Win64' -EvidenceJson 'D:/Latebrake/windows-evidence.json' -ManifestJson 'D:/LatebrakeBuilds/Win64/Latebrake-candidate-manifest.json'
    pwsh scripts/Verify-PlatformPackage.ps1 -Platform Android -PackagePath 'D:/LatebrakeBuilds/Android/Latebrake.apk' -EvidenceJson 'D:/Latebrake/android-evidence.json' -ManifestJson 'D:/LatebrakeBuilds/Android/Latebrake-candidate-manifest.json'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Win64','Android')][string]$Platform,
    [Parameter(Mandatory)][string]$PackagePath,
    [Parameter(Mandatory)][string]$EvidenceJson,
    [string]$ManifestJson
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-ExecutableSignature {
    param([string]$File, [int64]$MinBytes, [byte[]]$Signature)
    $item = Get-Item -LiteralPath $File -ErrorAction Stop
    if ($item.PSIsContainer -or $item.Length -lt $MinBytes) {
        throw "Artifact is missing or unexpectedly small: $File"
    }
    $stream = [System.IO.File]::OpenRead($item.FullName)
    try {
        foreach ($byte in $Signature) {
            if ($stream.ReadByte() -ne $byte) {
                throw "Wrong executable/package header: $File"
            }
        }
    } finally {
        $stream.Dispose()
    }
    return $item.FullName
}

# Provenance is required at the actual release QA gate. Never accept an
# EXE/APK whose exact bytes are not recorded in a post-UAT manifest.
if ([string]::IsNullOrWhiteSpace($ManifestJson)) {
    throw 'Explicit -ManifestJson is required to match the candidate to its recorded hashes.'
}
& (Join-Path $PSScriptRoot 'Test-CandidateManifest.ps1') -ManifestJson $ManifestJson -Platform $Platform
if (-not $?) { throw 'Candidate archive failed SHA256/file-list validation.' }

$manifestRoot = (Get-Item -LiteralPath $ManifestJson -ErrorAction Stop).Directory.FullName
$resolvedPackage = (Resolve-Path -LiteralPath $PackagePath -ErrorAction Stop).Path
$prefix = $manifestRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
    [System.IO.Path]::DirectorySeparatorChar
if (-not ($resolvedPackage.Equals($manifestRoot, [StringComparison]::OrdinalIgnoreCase) -or
          $resolvedPackage.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase))) {
    throw 'PackagePath must refer to files inside the verified manifest archive.'
}

if (-not (Test-Path -LiteralPath $EvidenceJson -PathType Leaf)) {
    throw "Missing human-authored installed-playtest evidence: $EvidenceJson"
}
$evidence = Get-Content -LiteralPath $EvidenceJson -Raw | ConvertFrom-Json -ErrorAction Stop
if ($evidence.platform -ne $Platform -or
    $evidence.installed_and_playtested -ne $true -or
    $evidence.unreal_version -notmatch '^5\.8(\.|$)' -or
    [string]::IsNullOrWhiteSpace([string]$evidence.build_commit) -or
    [string]::IsNullOrWhiteSpace([string]$evidence.test_hardware) -or
    [string]::IsNullOrWhiteSpace([string]$evidence.tester) -or
    [string]::IsNullOrWhiteSpace([string]$evidence.test_date) -or
    [string]::IsNullOrWhiteSpace([string]$evidence.test_notes)) {
    throw 'Incomplete platform-specific test evidence; actual installed UE 5.8 playtest is required.'
}
# Human QA must refer to the identical source revision as the archive:
# a hardware test of some OTHER executable is not qualifying evidence.
$buildManifest = Get-Content -LiteralPath $ManifestJson -Raw |
    ConvertFrom-Json -ErrorAction Stop
if (([string]$evidence.build_commit).Trim() -cne
    ([string]$buildManifest.source_commit).Trim()) {
    throw 'QA evidence build_commit does not match the verified candidate manifest source_commit.'
}
# All test evidence fields are self-declarations from a responsible tester.
# This script does not automatically prove their truth.
if ($Platform -eq 'Win64') {
    if (-not (Test-Path -LiteralPath $PackagePath -PathType Container)) {
        throw "Windows package directory not found: $PackagePath"
    }
    $game = Get-ChildItem -LiteralPath $PackagePath -File -Recurse -Filter '*Latebrake*.exe' |
        Select-Object -First 1
    $cooked = @(Get-ChildItem -LiteralPath $PackagePath -File -Recurse |
        Where-Object { $_.Extension -in '.pak','.ucas' })
    if ($null -eq $game -or $cooked.Count -eq 0) {
        throw 'Windows package requires an actual Latebrake EXE AND a cooked .pak/.ucas payload.'
    }
    $artifact = Assert-ExecutableSignature -File $game.FullName -MinBytes 102400 -Signature ([byte[]](0x4d,0x5a))
    $artifacts = @($artifact) + @($cooked | ForEach-Object { $_.FullName })
} else {
    $apk = Get-Item -LiteralPath $PackagePath -ErrorAction Stop
    if ($apk.Extension -ne '.apk') {
        throw 'Android package must be an actual .apk, not a source ZIP or renamed bundle.'
    }
    $artifact = Assert-ExecutableSignature -File $apk.FullName -MinBytes 1048576 -Signature ([byte[]](0x50,0x4b))
    # APK must be a real ZIP container with an Android application manifest.
    try {
        $zip = [System.IO.Compression.ZipFile]::OpenRead($artifact)
        try {
            if (-not ($zip.Entries | Where-Object { $_.FullName -eq 'AndroidManifest.xml' })) {
                throw 'APK does not contain AndroidManifest.xml.'
            }
            if (-not ($zip.Entries | Where-Object { $_.FullName -eq 'classes.dex' -or $_.FullName -like 'lib/*/*.so' })) {
                throw 'APK contains no compiled Android application or native library.'
            }
        } finally {
            $zip.Dispose()
        }
    } catch {
        throw "Invalid Android package contents: $_"
    }
    $artifacts = @($artifact)
}
foreach ($file in $artifacts) {
    $item = Get-Item -LiteralPath $file
    $hash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Output "$($item.FullName) | $($item.Length) bytes | SHA256 $hash"
}
Write-Host 'Structure, hash and stated playtest metadata checked. This is not an automatic gameplay certificate.'
Write-Host 'Do not upload without a responsible human verifying installed gameplay on target hardware.'
