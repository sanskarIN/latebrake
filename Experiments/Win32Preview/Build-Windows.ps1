<#
.SYNOPSIS
    Build ONLY the self-contained, native LateBrake 2D preview and setup.
.DESCRIPTION
    Run on Windows x64 using a Visual Studio 2022 x64 Native Tools shell,
    Python 3 and the Windows SDK. No Unreal Engine compilation or gameplay QA.
#>
[CmdletBinding()]
param([string]$OutputDirectory = (Join-Path $PSScriptRoot 'out'))
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'This preview build needs Windows MSVC/SDK.' }
foreach ($tool in @('cl.exe','link.exe','python.exe')) {
  if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
    throw "Missing $tool. Run in a VS x64 Native Tools prompt with Python 3."
  }
}
$root = (Resolve-Path -LiteralPath $PSScriptRoot).Path
[void](New-Item -ItemType Directory -Path $OutputDirectory -Force)
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
$source = Join-Path $root 'LateBrakePreview.cpp'
$gameObj = Join-Path $output 'LateBrakePreview.obj'
$gameExe = Join-Path $output 'LateBrakePreviewV3.exe'
$setupObj = Join-Path $output 'Setup.obj'
$setupExe = Join-Path $output 'LateBrakeSetupV3.exe'
$gameOptions = @('/nologo','/O2','/GS-','/Zl','/GR-','/std:c++20','/c',"/Fo$gameObj",$source)
& cl.exe @gameOptions
if ($LASTEXITCODE -ne 0) { throw 'Windows preview C++ compilation failed.' }
& link.exe /nologo /MACHINE:X64 /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /NODEFAULTLIB "/OUT:$gameExe" $gameObj kernel32.lib user32.lib gdi32.lib
if ($LASTEXITCODE -ne 0) { throw 'Windows preview PE linking failed.' }
& python.exe (Join-Path $root 'EmbedPayload.py') $gameExe (Join-Path $output 'payload.inc')
if ($LASTEXITCODE -ne 0) { throw 'Installer embed generation failed.' }
& cl.exe /nologo /O2 /GS- /Zl /GR- /std:c++20 /c "/I$output" "/Fo$setupObj" (Join-Path $root 'Setup.cpp')
if ($LASTEXITCODE -ne 0) { throw 'Setup C++ compilation failed.' }
& link.exe /nologo /MACHINE:X64 /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup /NODEFAULTLIB "/OUT:$setupExe" $setupObj kernel32.lib user32.lib advapi32.lib
if ($LASTEXITCODE -ne 0) { throw 'Setup PE linking failed.' }
Get-FileHash -Algorithm SHA256 -LiteralPath $gameExe,$setupExe | Format-Table Path,Hash -AutoSize
Write-Host 'Native preview and per-user setup BUILT, not Windows-device verified.'
