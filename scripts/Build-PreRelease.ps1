<#
.SYNOPSIS
    Build a real Latebrake Windows or Android development package with a locally licensed UE 5.8 install.
.DESCRIPTION
    Requires Unreal Engine 5.8 and its authorized platform SDK/toolchain on Windows.
    This helper has not been qualified in an Unreal environment yet.
.EXAMPLE
    pwsh scripts/Build-PreRelease.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8' -Platform Win64 -OutputDirectory 'D:/LatebrakeBuilds/Win64'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EngineRoot,
    [Parameter(Mandatory)][ValidateSet('Win64', 'Android')][string]$Platform,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repo 'Latebrake.uproject'
$uat = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
    throw "Project missing: $project"
}
if (-not (Test-Path -LiteralPath $uat -PathType Leaf)) {
    throw "Unreal automation tool not found: $uat. Install licensed UE 5.8."
}
$project = (Resolve-Path -LiteralPath $project).Path
# Engine/Maps/Entry is an engine placeholder; a shipped game needs a
# genuinely authored and saved project map, not a fabricated text .umap.
$contentRoot = Join-Path $repo 'Content'
if (-not (Test-Path -LiteralPath $contentRoot -PathType Container)) {
    throw 'No project Content directory/cooked .umap exists. Open the project in a licensed Unreal editor and save a real level first.'
}
$maps = @(Get-ChildItem -LiteralPath $contentRoot -Filter '*.umap' -File -Recurse -ErrorAction Stop)
if ($maps.Count -eq 0) {
    throw 'No actual Unreal .umap found. Do not package a game using only /Engine/Maps/Entry.'
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$out = (Resolve-Path -LiteralPath $OutputDirectory).Path

$uatArguments = @(
    'BuildCookRun',
    "-project=$project",
    '-noP4',
    '-build',
    '-cook',
    '-stage',
    '-pak',
    '-archive',
    "-archivedirectory=$out",
    "-platform=$Platform",
    '-clientconfig=Development',
    '-utf8output'
)
if ($Platform -eq 'Android') {
    $uatArguments += '-package'
} else {
    $uatArguments += '-prereqs'
}
Write-Host "Running Unreal automation for $Platform. A successful process exit does not replace real playtesting."
& $uat @uatArguments
if ($LASTEXITCODE -ne 0) {
    throw "Unreal BuildCookRun failed, exit code $LASTEXITCODE"
}
if ($Platform -eq 'Win64') {
    $exes = @(Get-ChildItem -LiteralPath $out -Recurse -File -Filter '*Latebrake*.exe' -ErrorAction SilentlyContinue)
    if ($exes.Count -eq 0) {
        throw 'Build completed but did not create a detectable Windows Latebrake EXE.'
    }
} else {
    $apks = @(Get-ChildItem -LiteralPath $out -Recurse -File -Filter '*.apk' -ErrorAction SilentlyContinue)
    if ($apks.Count -eq 0) {
        throw 'Build completed but did not create an Android APK.'
    }
}
Write-Host "BuildCookRun produced candidate $Platform files in $out."
Write-Host 'Run gameplay/device tests, record evidence, and validate artifacts before publishing any pre-release.'
