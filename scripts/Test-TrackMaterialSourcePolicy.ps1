<#
.SYNOPSIS
    Synthetic negative test for tracked material SHA256 gate. NO Unreal.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function MustReject {
    param([string]$Name, [scriptblock]$Action)
    $rejected = $false
    try { & $Action *> $null } catch { $rejected = $true }
    if (-not $rejected) { throw "Material gate accepted unsafe synthetic fixture: $Name" }
    Write-Host "Rejected fixture: $Name"
}
$gate = Join-Path $PSScriptRoot 'Test-TrackMaterialSource.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ('LatebrakeTrackMaterial-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null
try {
    & git -C $temp init -q
    if ($LASTEXITCODE -ne 0) { throw 'Fixture git init failed.' }
    [IO.File]::WriteAllText((Join-Path $temp 'README.md'), 'SYNTHETIC NOT UNREAL')
    & git -C $temp add README.md
    & git -C $temp -c user.name=LatebrakeTest -c user.email=sanskarin@outlook.in commit -qm 'Fixture'
    if ($LASTEXITCODE -ne 0) { throw 'Fixture first commit failed.' }
    & $gate -RepoRoot $temp
    MustReject 'absent material required' { & $gate -RepoRoot $temp -RequireMaterial }
    MustReject 'absent material pinned' { & $gate -RepoRoot $temp -ExpectedSha256 ('a' * 64) }

    $path = Join-Path $temp 'Content/Latebrake/Materials/M_LatebrakeTrackPBR.uasset'
    [IO.Directory]::CreateDirectory((Split-Path $path -Parent)) | Out-Null
    [IO.File]::WriteAllBytes($path, [Text.Encoding]::UTF8.GetBytes(('TEST_NOT_UNREAL' * 150)))
    MustReject 'untracked material' { & $gate -RepoRoot $temp -RequireMaterial }
    & git -C $temp add -- Content/Latebrake/Materials/M_LatebrakeTrackPBR.uasset
    & git -C $temp -c user.name=LatebrakeTest -c user.email=sanskarin@outlook.in commit -qm 'Fixture art'
    if ($LASTEXITCODE -ne 0) { throw 'Fixture material commit failed.' }
    $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    & $gate -RepoRoot $temp -RequireMaterial -ExpectedSha256 $hash
    MustReject 'wrong snapshot' { & $gate -RepoRoot $temp -ExpectedSha256 ('a' * 64) }
    MustReject 'malformed snapshot' { & $gate -RepoRoot $temp -ExpectedSha256 'bad' }
    [IO.File]::WriteAllBytes($path, [Text.Encoding]::UTF8.GetBytes(('ALTERED_NOT_UNREAL' * 150)))
    MustReject 'uncommitted modification' { & $gate -RepoRoot $temp -RequireMaterial }
    & git -C $temp restore --worktree -- Content/Latebrake/Materials/M_LatebrakeTrackPBR.uasset
    if ($LASTEXITCODE -ne 0) { throw 'Fixture restore failed.' }

    # The source file exists and is tracked, but is NOT hydrated LFS content.
    [IO.File]::WriteAllText($path, 'version https://git-lfs.github.com/spec/v1')
    & git -C $temp add -- Content/Latebrake/Materials/M_LatebrakeTrackPBR.uasset
    & git -C $temp -c user.name=LatebrakeTest -c user.email=sanskarin@outlook.in commit -qm 'Fixture pointer'
    if ($LASTEXITCODE -ne 0) { throw 'Fixture pointer commit failed.' }
    MustReject 'unhydrated Git LFS pointer' { & $gate -RepoRoot $temp -RequireMaterial }
    Write-Host 'Material provenance policy: synthetic fixtures passed, no Unreal executed.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
