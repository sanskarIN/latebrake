<#
.SYNOPSIS
    Fail-fast sanity check for a serialized Unreal binary .umap.
.DESCRIPTION
    Requires a plausible size and Unreal package file header magic
    (PACKAGE_FILE_TAG, 0x9E2A83C1). This rejects git-lfs pointers and
    renamed arbitrary data. It does NOT prove that the level cooks or loads.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$MapPath
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $MapPath -PathType Leaf)) {
    throw "Missing real engine-authored Unreal map: $MapPath"
}
$item = Get-Item -LiteralPath $MapPath
if ($item.Length -lt 1024) {
    throw "Unreal map is too small or an unpulled Git LFS pointer: $MapPath"
}

$stream = [System.IO.File]::OpenRead($item.FullName)
try {
    $signature = [byte[]]::new(4)
    if ($stream.Read($signature, 0, 4) -ne 4) {
        throw "Unreal map header could not be read: $MapPath"
    }
} finally {
    $stream.Dispose()
}
$tag = [byte[]](0xC1, 0x83, 0x2A, 0x9E)
$tagSwapped = [byte[]](0x9E, 0x2A, 0x83, 0xC1)
$correct = $true
$swapped = $true
for ($i = 0; $i -lt 4; $i++) {
    if ($signature[$i] -ne $tag[$i]) { $correct = $false }
    if ($signature[$i] -ne $tagSwapped[$i]) { $swapped = $false }
}
if (-not ($correct -or $swapped)) {
    throw 'Not a supported Unreal package header. A renamed file or Git LFS pointer is not a genuine level.'
}
Write-Host "Plausible serialized Unreal map: $($item.Name) ($($item.Length) bytes). Engine load/cook still required."
