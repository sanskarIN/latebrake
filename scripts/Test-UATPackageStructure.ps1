<#
.SYNOPSIS
    Reject obviously malformed UAT outputs before archiving an unverified candidate.
.DESCRIPTION
    Structural checks only: signatures, minimum size, cooked-content presence
    and Android ZIP entries. Does NOT prove the bytes were produced by Unreal,
    the game launches, or any device QA occurred. Call only AFTER real RunUAT.
.EXAMPLE
    pwsh scripts/Test-UATPackageStructure.ps1 -Platform Win64 -PackageDirectory 'D:/LatebrakeCandidates/Win64'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Win64', 'Android')][string]$Platform,
    [Parameter(Mandatory)][string]$PackageDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $PackageDirectory -PathType Container)) {
    throw "Candidate package directory is absent: $PackageDirectory"
}
$root = (Get-Item -LiteralPath $PackageDirectory -Force -ErrorAction Stop).FullName
if (((Get-Item -LiteralPath $root -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'A UAT candidate root cannot be a symlink or junction.'
}
$nodes = @(Get-ChildItem -LiteralPath $root -Recurse -Force -ErrorAction Stop)
foreach ($node in $nodes) {
    if (($node.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "A UAT candidate cannot contain symlinks or junctions: $($node.FullName)"
    }
}

function Assert-FileMagic {
    param([string]$FilePath, [int64]$MinSize, [byte[]]$Magic)
    $file = Get-Item -LiteralPath $FilePath -ErrorAction Stop
    if ($file.PSIsContainer -or $file.Length -lt $MinSize) {
        throw "UAT candidate payload missing or too small: $FilePath"
    }
    $stream = [IO.File]::OpenRead($file.FullName)
    try {
        foreach ($byte in $Magic) {
            if ($stream.ReadByte() -ne $byte) {
                throw "UAT candidate payload has an invalid header: $FilePath"
            }
        }
    } finally {
        $stream.Dispose()
    }
}

$files = @($nodes | Where-Object { -not $_.PSIsContainer })
if ($Platform -eq 'Win64') {
    $games = @($files | Where-Object { $_.Name -like '*Latebrake*.exe' })
    $cooked = @($files | Where-Object { $_.Extension -in @('.pak', '.ucas') })
    if ($games.Count -eq 0 -or $cooked.Count -eq 0) {
        throw 'Win64 UAT output must contain a Latebrake EXE and cooked .pak/.ucas assets.'
    }
    foreach ($game in $games) {
        Assert-FileMagic -FilePath $game.FullName -MinSize 102400 -Magic ([byte[]](0x4D,0x5A))
    }
    foreach ($content in $cooked) {
        if ($content.Length -lt 1024) {
            throw "Cooked Win64 payload is implausibly small: $($content.FullName)"
        }
    }
    Write-Host "Win64 candidate structure verified: $($games.Count) EXE(s), $($cooked.Count) cooked asset payload(s)."
} else {
    $apks = @($files | Where-Object { $_.Extension -ieq '.apk' })
    if ($apks.Count -eq 0) {
        throw 'Android UAT output has no .apk file.'
    }
    foreach ($apk in $apks) {
        Assert-FileMagic -FilePath $apk.FullName -MinSize 1048576 -Magic ([byte[]](0x50,0x4B))
        try {
            $zip = [IO.Compression.ZipFile]::OpenRead($apk.FullName)
            try {
                $manifests = @($zip.Entries | Where-Object { $_.FullName -ceq 'AndroidManifest.xml' })
                if ($manifests.Count -ne 1 -or $manifests[0].Length -eq 0) {
                    throw 'APK must contain exactly one nonempty AndroidManifest.xml.'
                }
                $compiled = @($zip.Entries | Where-Object {
                    $_.Length -gt 0 -and
                    ($_.FullName -ceq 'classes.dex' -or
                     $_.FullName -cmatch '^lib/[^/]+/[^/]+[.]so$')
                })
                if ($compiled.Count -eq 0) {
                    throw 'APK is missing compiled Dalvik bytecode or a native Android library.'
                }
            } finally {
                $zip.Dispose()
            }
        } catch {
            throw "Invalid Android UAT candidate ZIP structure ($($apk.Name)): $_"
        }
    }
    Write-Host "Android candidate ZIP structure verified: $($apks.Count) APK(s)."
}
Write-Host 'NOT PLAYTESTED: structural checks are not Unreal provenance, game runtime or device QA.'
