<#
.SYNOPSIS
    Verify every file in a real, unverified LateBrake UAT candidate archive.
.DESCRIPTION
    Reject changed, missing, extra or path-escaping files against the
    per-file SHA256 manifest created AFTER actual Unreal BuildCookRun.
    This is a provenance/integrity check, NOT a gameplay certification or
    proof that a package was genuinely produced by Unreal.
.EXAMPLE
    pwsh scripts/Test-CandidateManifest.ps1 -ManifestJson 'D:/Candidates/Win64/Latebrake-candidate-manifest.json' -Platform Win64
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ManifestJson,
    [Parameter(Mandatory)][ValidateSet('Win64', 'Android')][string]$Platform
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $ManifestJson -PathType Leaf)) {
    throw "Candidate manifest missing: $ManifestJson"
}
$manifestFile = Get-Item -LiteralPath $ManifestJson -Force -ErrorAction Stop
if ($manifestFile.Name -cne 'Latebrake-candidate-manifest.json') {
    throw 'Unexpected candidate manifest name.'
}
if (($manifestFile.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'Candidate manifest must be a regular file, not a symlink or reparse point.'
}
$root = $manifestFile.Directory.FullName
$rootItem = Get-Item -LiteralPath $root -Force -ErrorAction Stop
if (($rootItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'Candidate archive root must not be a symlink or reparse point.'
}

# A manifest can name a regular file THROUGH a junction/symlink directory.
# Checking only the final file's attributes would silently follow that link
# outside the verified archive. Inspect every ancestor before opening it.
function Assert-SafeArchiveParents {
    param([string]$ArchiveRoot, [string]$RelativeFile)
    $parent = $ArchiveRoot
    $parts = $RelativeFile.Split('/')
    for ($i = 0; $i -lt ($parts.Length - 1); ++$i) {
        $parent = Join-Path $parent $parts[$i]
        $part = Get-Item -LiteralPath $parent -Force -ErrorAction Stop
        if (-not $part.PSIsContainer -or
            ($part.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Unsafe symlink, junction or non-directory in archive path: $RelativeFile"
        }
    }
}

$manifest = Get-Content -LiteralPath $manifestFile.FullName -Raw |
    ConvertFrom-Json -ErrorAction Stop
if ($manifest.schema_version -ne 2 -or
    $manifest.project -cne 'LateBrake' -or
    $manifest.platform -cne $Platform -or
    $manifest.status -cne 'UNVERIFIED_CANDIDATE_NOT_PLAYTESTED' -or
    $manifest.source_commit -notmatch '^[a-fA-F0-9]{40}$' -or
    $manifest.unreal_engine_version -notmatch '^5\.8(\.|$)' -or
    $manifest.starter_map_sha256 -notmatch '^[a-fA-F0-9]{64}
    $manifest.source_tree_tracked_changes -ne $false) {
    throw 'Wrong project, platform, schema, engine version, status, source provenance or dirty source tree.'
}

$entries = @($manifest.files)
if ($entries.Count -eq 0) {
    throw 'Manifest has no archived files.'
}

$seen = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)
$checked = 0
foreach ($entry in $entries) {
    $name = [string]$entry.path
    if ([string]::IsNullOrWhiteSpace($name) -or
        $name.StartsWith('/') -or
        $name.Contains('\') -or
        $name.Contains(':') -or
        $name.Contains('//') -or
        $name -match '(^|/)\.\.?(/|$)' -or
        -not $seen.Add($name)) {
        throw "Unsafe, duplicate or invalid manifest path: $name"
    }

    Assert-SafeArchiveParents -ArchiveRoot $root -RelativeFile $name
    $file = Join-Path $root ($name.Replace('/', [System.IO.Path]::DirectorySeparatorChar))
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
        throw "Manifest lists a missing file: $name"
    }
    $item = Get-Item -LiteralPath $file
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Manifest entry is a symlink or reparse point: $name"
    }
    $expectedBytes = [int64]$entry.bytes
    $expectedHash = [string]$entry.sha256
    if ($expectedBytes -le 0 -or
        $item.Length -ne $expectedBytes -or
        $expectedHash -notmatch '^[a-fA-F0-9]{64}$') {
        throw "Manifest size or digest invalid for: $name"
    }
    $actualHash = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
    if ($actualHash -ine $expectedHash) {
        throw "SHA256 mismatch for $name; archive changed after fingerprinting."
    }
    ++$checked
}

# Reject even EMPTY linked folders, which contain no manifest file to check.
# PowerShell's default recursive listing does not follow directory symlinks;
# checking directory attributes still sees the links themselves.
$physicalDirectories = @(Get-ChildItem -LiteralPath $root -Directory -Recurse -Force)
foreach ($directory in $physicalDirectories) {
    if (($directory.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Candidate contains a symlink/junction directory: $($directory.FullName)"
    }
}
$physicalFiles = @(Get-ChildItem -LiteralPath $root -File -Recurse -Force |
    Where-Object { $_.FullName -ne $manifestFile.FullName })
foreach ($item in $physicalFiles) {
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Candidate contains unexpected symlink/reparse point: $($item.FullName)"
    }
    $relative = [System.IO.Path]::GetRelativePath($root, $item.FullName).Replace('\', '/')
    if (-not $seen.Contains($relative)) {
        throw "Candidate contains a file missing from manifest: $relative"
    }
}
if ($physicalFiles.Count -ne $checked) {
    throw 'Manifest file count differs from actual candidate archive.'
}
Write-Host "Candidate manifest integrity verified: $checked file(s), platform $Platform."
Write-Host 'NOT PLAYTESTED: a checksum match does not certify an Unreal build or working game.'
 -or
    $manifest.unreal_engine_build_sha256 -cnotmatch '^[a-f0-9]{64}
    $manifest.source_tree_tracked_changes -ne $false) {
    throw 'Wrong project, platform, schema, engine version, status, source provenance or dirty source tree.'
}

$entries = @($manifest.files)
if ($entries.Count -eq 0) {
    throw 'Manifest has no archived files.'
}

$seen = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)
$checked = 0
foreach ($entry in $entries) {
    $name = [string]$entry.path
    if ([string]::IsNullOrWhiteSpace($name) -or
        $name.StartsWith('/') -or
        $name.Contains('\') -or
        $name.Contains(':') -or
        $name.Contains('//') -or
        $name -match '(^|/)\.\.?(/|$)' -or
        -not $seen.Add($name)) {
        throw "Unsafe, duplicate or invalid manifest path: $name"
    }

    Assert-SafeArchiveParents -ArchiveRoot $root -RelativeFile $name
    $file = Join-Path $root ($name.Replace('/', [System.IO.Path]::DirectorySeparatorChar))
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
        throw "Manifest lists a missing file: $name"
    }
    $item = Get-Item -LiteralPath $file
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Manifest entry is a symlink or reparse point: $name"
    }
    $expectedBytes = [int64]$entry.bytes
    $expectedHash = [string]$entry.sha256
    if ($expectedBytes -le 0 -or
        $item.Length -ne $expectedBytes -or
        $expectedHash -notmatch '^[a-fA-F0-9]{64}$') {
        throw "Manifest size or digest invalid for: $name"
    }
    $actualHash = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
    if ($actualHash -ine $expectedHash) {
        throw "SHA256 mismatch for $name; archive changed after fingerprinting."
    }
    ++$checked
}

# Reject even EMPTY linked folders, which contain no manifest file to check.
# PowerShell's default recursive listing does not follow directory symlinks;
# checking directory attributes still sees the links themselves.
$physicalDirectories = @(Get-ChildItem -LiteralPath $root -Directory -Recurse -Force)
foreach ($directory in $physicalDirectories) {
    if (($directory.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Candidate contains a symlink/junction directory: $($directory.FullName)"
    }
}
$physicalFiles = @(Get-ChildItem -LiteralPath $root -File -Recurse -Force |
    Where-Object { $_.FullName -ne $manifestFile.FullName })
foreach ($item in $physicalFiles) {
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Candidate contains unexpected symlink/reparse point: $($item.FullName)"
    }
    $relative = [System.IO.Path]::GetRelativePath($root, $item.FullName).Replace('\', '/')
    if (-not $seen.Contains($relative)) {
        throw "Candidate contains a file missing from manifest: $relative"
    }
}
if ($physicalFiles.Count -ne $checked) {
    throw 'Manifest file count differs from actual candidate archive.'
}
Write-Host "Candidate manifest integrity verified: $checked file(s), platform $Platform."
Write-Host 'NOT PLAYTESTED: a checksum match does not certify an Unreal build or working game.'
 -or
    $manifest.source_tree_tracked_changes -ne $false) {
    throw 'Wrong project, platform, schema, engine version, status, source provenance or dirty source tree.'
}

$entries = @($manifest.files)
if ($entries.Count -eq 0) {
    throw 'Manifest has no archived files.'
}

$seen = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)
$checked = 0
foreach ($entry in $entries) {
    $name = [string]$entry.path
    if ([string]::IsNullOrWhiteSpace($name) -or
        $name.StartsWith('/') -or
        $name.Contains('\') -or
        $name.Contains(':') -or
        $name.Contains('//') -or
        $name -match '(^|/)\.\.?(/|$)' -or
        -not $seen.Add($name)) {
        throw "Unsafe, duplicate or invalid manifest path: $name"
    }

    Assert-SafeArchiveParents -ArchiveRoot $root -RelativeFile $name
    $file = Join-Path $root ($name.Replace('/', [System.IO.Path]::DirectorySeparatorChar))
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
        throw "Manifest lists a missing file: $name"
    }
    $item = Get-Item -LiteralPath $file
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Manifest entry is a symlink or reparse point: $name"
    }
    $expectedBytes = [int64]$entry.bytes
    $expectedHash = [string]$entry.sha256
    if ($expectedBytes -le 0 -or
        $item.Length -ne $expectedBytes -or
        $expectedHash -notmatch '^[a-fA-F0-9]{64}$') {
        throw "Manifest size or digest invalid for: $name"
    }
    $actualHash = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash
    if ($actualHash -ine $expectedHash) {
        throw "SHA256 mismatch for $name; archive changed after fingerprinting."
    }
    ++$checked
}

# Reject even EMPTY linked folders, which contain no manifest file to check.
# PowerShell's default recursive listing does not follow directory symlinks;
# checking directory attributes still sees the links themselves.
$physicalDirectories = @(Get-ChildItem -LiteralPath $root -Directory -Recurse -Force)
foreach ($directory in $physicalDirectories) {
    if (($directory.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Candidate contains a symlink/junction directory: $($directory.FullName)"
    }
}
$physicalFiles = @(Get-ChildItem -LiteralPath $root -File -Recurse -Force |
    Where-Object { $_.FullName -ne $manifestFile.FullName })
foreach ($item in $physicalFiles) {
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Candidate contains unexpected symlink/reparse point: $($item.FullName)"
    }
    $relative = [System.IO.Path]::GetRelativePath($root, $item.FullName).Replace('\', '/')
    if (-not $seen.Contains($relative)) {
        throw "Candidate contains a file missing from manifest: $relative"
    }
}
if ($physicalFiles.Count -ne $checked) {
    throw 'Manifest file count differs from actual candidate archive.'
}
Write-Host "Candidate manifest integrity verified: $checked file(s), platform $Platform."
Write-Host 'NOT PLAYTESTED: a checksum match does not certify an Unreal build or working game.'
