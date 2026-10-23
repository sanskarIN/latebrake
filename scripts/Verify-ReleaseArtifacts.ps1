<#
.SYNOPSIS
    Qualify a Win64 + Android Unreal game build pair for MANUAL release review.
.DESCRIPTION
    Requires both exact post-UAT SHA256 archives and two platform-specific,
    human-authored installed-game QA reports. Do NOT use source-only tests,
    renamed binaries, or a generic yes/no evidence flag to approve a release.
    This does not upload artifacts or independently prove the QA is truthful.
.EXAMPLE
    pwsh scripts/Verify-ReleaseArtifacts.ps1 -Win64Directory 'D:/LatebrakeCandidates/Win64' -AndroidApk 'D:/LatebrakeCandidates/Android/Android/Latebrake.apk' -Win64ManifestJson 'D:/LatebrakeCandidates/Win64/Latebrake-candidate-manifest.json' -AndroidManifestJson 'D:/LatebrakeCandidates/Android/Latebrake-candidate-manifest.json' -Win64EvidenceJson 'D:/QA/win64-qa.json' -AndroidEvidenceJson 'D:/QA/android-qa.json'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Win64Directory,
    [Parameter(Mandatory)][string]$AndroidApk,
    [Parameter(Mandatory)][string]$Win64ManifestJson,
    [Parameter(Mandatory)][string]$AndroidManifestJson,
    [Parameter(Mandatory)][string]$Win64EvidenceJson,
    [Parameter(Mandatory)][string]$AndroidEvidenceJson
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$verifier = Join-Path $PSScriptRoot 'Verify-PlatformPackage.ps1'
# Each verifier rejects an altered, extra or missing archive file, fake EXE
# or APK structure, absent installed-device QA, and mismatched source SHA.
& $verifier -Platform Win64 -PackagePath $Win64Directory -EvidenceJson $Win64EvidenceJson -ManifestJson $Win64ManifestJson
if (-not $?) { throw 'Win64 archive/platform verification failed.' }
& $verifier -Platform Android -PackagePath $AndroidApk -EvidenceJson $AndroidEvidenceJson -ManifestJson $AndroidManifestJson
if (-not $?) { throw 'Android archive/platform verification failed.' }

# Source revision and engine version were previously compared, but the
# map SHA256 was not. Two clean-HEAD candidates can still cook different
# untracked editor-authored .umap bytes. Reject an inconsistent pair.
& (Join-Path $PSScriptRoot 'Test-CandidatePairIdentity.ps1') -Win64ManifestJson $Win64ManifestJson -AndroidManifestJson $AndroidManifestJson
if (-not $?) { throw 'Win64/Android candidate identity is inconsistent.' }
Write-Host 'Manual dual-platform release review may proceed. This is NOT automatic proof of gameplay or a published release.'
