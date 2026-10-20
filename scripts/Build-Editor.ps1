<#
.SYNOPSIS
    Compile the genuine LatebrakeEditor target with a licensed Unreal 5.8 toolchain.
.DESCRIPTION
    UnrealBuildTool (via Build.bat) is required. This is not a portable
    CMake build and cannot run on GitHub's ordinary hosted Windows image.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$EngineRoot
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repo 'Latebrake.uproject'
$build = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
$versionFile = Join-Path $EngineRoot 'Engine/Build/Build.version'
if (-not (Test-Path -LiteralPath $build -PathType Leaf)) {
    throw "Unreal Engine Build.bat missing: $build. Install authorized UE 5.8."
}
if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
    throw "Project descriptor missing: $project"
}
if (-not (Test-Path -LiteralPath $versionFile -PathType Leaf)) {
    throw "Cannot verify Unreal Engine version: $versionFile"
}
$v = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json -ErrorAction Stop
if ($v.MajorVersion -ne 5 -or $v.MinorVersion -ne 8) {
    throw "Incompatible engine: expected UE 5.8; found $($v.MajorVersion).$($v.MinorVersion)"
}

$project = (Resolve-Path -LiteralPath $project).Path
Write-Host "Compiling real LatebrakeEditor Win64 Development with licensed UE 5.8."
& $build 'LatebrakeEditor' 'Win64' 'Development' "-Project=$project" '-WaitMutex'
if ($LASTEXITCODE -ne 0) {
    throw "UnrealBuildTool failed while compiling LatebrakeEditor (exit $LASTEXITCODE)."
}
Write-Host 'Editor build command completed. Continue with Unreal map authoring and runtime tests.'
