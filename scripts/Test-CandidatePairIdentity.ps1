<#
.SYNOPSIS
    Compare Win64 and Android source/map identities for manual release review.
.DESCRIPTION
    Validates candidate manifest metadata only. Does NOT check file hashes,
    authenticate Unreal binaries, or certify installed QA. The caller
    Verify-ReleaseArtifacts.ps1 performs independent per-platform package
    and evidence verification BEFORE this pair policy.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Win64ManifestJson,
    [Parameter(Mandatory)][string]$AndroidManifestJson
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Read-CandidateIdentity {
    param([string]$Path, [string]$Platform)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing $Platform candidate manifest: $Path"
    }
    $info = Get-Content -LiteralPath $Path -Raw -ErrorAction Stop |
        ConvertFrom-Json -ErrorAction Stop
    if ($info.schema_version -ne 1 -or
        $info.project -cne 'LateBrake' -or
        $info.platform -cne $Platform -or
        $info.status -cne 'UNVERIFIED_CANDIDATE_NOT_PLAYTESTED' -or
        $info.source_tree_tracked_changes -ne $false -or
        [string]$info.source_commit -cnotmatch '^[a-fA-F0-9]{40}$' -or
        [string]$info.starter_map_sha256 -cnotmatch '^[a-fA-F0-9]{64}$' -or
        [string]$info.unreal_engine_version -cnotmatch '^5[.]8([.]|$)') {
        throw "Malformed or unqualified $Platform candidate identity."
    }
    return $info
}

$windows = Read-CandidateIdentity -Path $Win64ManifestJson -Platform 'Win64'
$android = Read-CandidateIdentity -Path $AndroidManifestJson -Platform 'Android'
if ($windows.source_commit -ine $android.source_commit) {
    throw 'Windows and Android packages come from different source commits.'
}
if ($windows.unreal_engine_version -cne $android.unreal_engine_version) {
    throw 'Windows and Android packages used different Unreal Engine versions.'
}
if ($windows.starter_map_sha256 -ine $android.starter_map_sha256) {
    throw 'Windows and Android packages were cooked from different circuit map bytes.'
}

Write-Host "Win64/Android metadata agree on source, UE version and circuit SHA256: $($windows.starter_map_sha256)"
Write-Host 'Metadata equality is NOT game compilation, installed-device QA or playable release evidence.'
