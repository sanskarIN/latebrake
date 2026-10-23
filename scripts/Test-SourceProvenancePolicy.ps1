<#
.SYNOPSIS
    Disposable Git provenance policy regression, no Unreal installation.
.DESCRIPTION
    Temporary Git history is synthetic and deleted. A passing test only
    verifies that untracked game source cannot silently be attributed to HEAD.
#>
[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Expect-Rejection {
    param([string]$Name, [scriptblock]$Run)
    $failed = $false
    try { & $Run *> $null } catch { $failed = $true }
    if (-not $failed) {
        throw "Unsafe source checkout was accepted: $Name"
    }
    Write-Host "Rejected untraceable source: $Name"
}

$gate = Join-Path $PSScriptRoot 'Test-SourceProvenance.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ("LatebrakeSourceProvenance-" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp -Force | Out-Null
try {
    & git -C $temp init -q
    if ($LASTEXITCODE -ne 0) { throw 'Disposable git init failed.' }
    $readme = Join-Path $temp 'README.md'
    [IO.File]::WriteAllText($readme, 'INITIAL_COMMITTED_SOURCE')
    # The built map is intentionally the only allowed untracked asset.
    # Build intermediates are excluded by .gitignore, not by magic status
    # exemptions that could also silently accept new source code.
    [IO.File]::WriteAllText((Join-Path $temp '.gitignore'),
        "Intermediate/`nSaved/`nBinaries/`n")
    & git -C $temp add README.md .gitignore
    if ($LASTEXITCODE -ne 0) { throw 'Disposable git add failed.' }
    & git -C $temp -c user.name=LatebrakeTest -c user.email=sanskarin@outlook.in commit -qm 'Fixture initial commit'
    if ($LASTEXITCODE -ne 0) { throw 'Disposable git commit failed.' }

    & $gate -RepoRoot $temp
    if (-not $?) { throw 'Clean committed source must be accepted.' }

    $map = Join-Path $temp 'Content/Maps/LatebrakeCircuit.umap'
    [IO.Directory]::CreateDirectory((Split-Path $map -Parent)) | Out-Null
    [IO.File]::WriteAllText($map, 'TEST ONLY: NOT A SERIALIZED UNREAL LEVEL')
    & $gate -RepoRoot $temp
    if (-not $?) { throw 'The dedicated editor-authored map path must be permitted.' }

    [IO.Directory]::CreateDirectory((Join-Path $temp 'Source/Latebrake/Private')) | Out-Null
    $untrackedSource = Join-Path $temp 'Source/Latebrake/Private/FutureCar.cpp'
    [IO.File]::WriteAllText($untrackedSource, 'TEST ONLY: NOT GAME SOURCE')
    Expect-Rejection 'untracked C++ alongside untracked map' {
        & $gate -RepoRoot $temp
    }
    Remove-Item -LiteralPath $untrackedSource

    $untrackedConfig = Join-Path $temp 'TestGame.ini'
    [IO.File]::WriteAllText($untrackedConfig, 'TEST ONLY')
    Expect-Rejection 'untracked config file' { & $gate -RepoRoot $temp }
    Remove-Item -LiteralPath $untrackedConfig

    $wrongMap = Join-Path $temp 'Content/Maps/UnreviewedExtra.umap'
    [IO.File]::WriteAllText($wrongMap, 'TEST ONLY')
    Expect-Rejection 'another untracked map is NOT exempt' { & $gate -RepoRoot $temp }
    Remove-Item -LiteralPath $wrongMap

    [IO.File]::WriteAllText($readme, 'UNCOMMITTED MODIFICATION')
    Expect-Rejection 'modified tracked file' { & $gate -RepoRoot $temp }
    & git -C $temp restore README.md
    if ($LASTEXITCODE -ne 0) { throw 'Fixture reset failed.' }

    [IO.File]::WriteAllText($readme, 'STAGED MODIFICATION')
    & git -C $temp add README.md
    if ($LASTEXITCODE -ne 0) { throw 'Fixture staged add failed.' }
    Expect-Rejection 'staged tracked change' { & $gate -RepoRoot $temp }
    & git -C $temp restore --source=HEAD --staged --worktree README.md
    if ($LASTEXITCODE -ne 0) { throw 'Fixture staged reset failed.' }

    [IO.Directory]::CreateDirectory((Join-Path $temp 'Intermediate')) | Out-Null
    [IO.File]::WriteAllText((Join-Path $temp 'Intermediate/GENERATED.txt'), 'IGNORED BUILD OUTPUT')
    & $gate -RepoRoot $temp
    if (-not $?) { throw 'Ignored build output may remain in working tree.' }

    # Removing the map returns to a completely clean checkout.
    Remove-Item -LiteralPath $map
    & $gate -RepoRoot $temp
    if (-not $?) { throw 'Source must recover after invalid files are removed.' }

    $unborn = Join-Path $temp 'UNBORN_REPO'
    New-Item -ItemType Directory -Path $unborn -Force | Out-Null
    & git -C $unborn init -q
    if ($LASTEXITCODE -ne 0) { throw 'Disposable unborn git init failed.' }
    Expect-Rejection 'repo without a committed HEAD' { & $gate -RepoRoot $unborn }
    Write-Host 'All source provenance policy regressions passed: NO Unreal game was built.'
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
