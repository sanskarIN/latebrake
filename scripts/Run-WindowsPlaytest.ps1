<#
.SYNOPSIS
    Run an already-built, private LateBrake Win64 candidate with separate logs.
.DESCRIPTION
    Verifies the complete candidate manifest and package structure BEFORE
    launching packaged Latebrake.exe. Unreal's documented -ABSLOG argument
    stores diagnostics beside, not inside, the immutable game archive.
    A JSON receipt records process exit and hashes, never gameplay success.
.EXAMPLE
    pwsh ./scripts/Run-WindowsPlaytest.ps1 -CandidateDirectory 'D:/LateBrakeCandidates/LateBrake-Win64-1234'
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$CandidateDirectory,
    [string]$LogDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $IsWindows -or $PSVersionTable.PSVersion.Major -lt 7) {
    throw 'Run personal Win64 candidates on Windows with PowerShell 7.'
}
if (-not (Test-Path -LiteralPath $CandidateDirectory -PathType Container)) {
    throw "The actual packaged Win64 candidate directory is missing: $CandidateDirectory"
}
$rootItem = Get-Item -LiteralPath $CandidateDirectory -Force -ErrorAction Stop
if (($rootItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'The candidate directory must not be a symlink or junction.'
}
$root = $rootItem.FullName
$manifest = Join-Path $root 'Latebrake-candidate-manifest.json'

# Never launch an EXE from a malformed or modified candidate directory.
# Manifest and structural checks are NECESSARY, not proof of actual UE origin.
& (Join-Path $PSScriptRoot 'Test-CandidateManifest.ps1') -ManifestJson $manifest -Platform Win64
if (-not $?) { throw 'A changed/invalid Win64 manifest blocks private runtime testing.' }
& (Join-Path $PSScriptRoot 'Test-UATPackageStructure.ps1') -Platform Win64 -PackageDirectory $root
if (-not $?) { throw 'A malformed Win64 archive blocks private runtime testing.' }

$executables = @(Get-ChildItem -LiteralPath $root -Recurse -File -ErrorAction Stop |
    Where-Object { $_.Name -ieq 'Latebrake.exe' })
if ($executables.Count -ne 1) {
    throw "Expected exactly one packaged Latebrake.exe; found $($executables.Count)."
}
$game = $executables[0]
$gameSha256 = (Get-FileHash -LiteralPath $game.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
$manifestSha256 = (Get-FileHash -LiteralPath $manifest -Algorithm SHA256).Hash.ToLowerInvariant()

# Runtime logs must remain OUTSIDE the fingerprinted game archive. An extra
# Saved/Logs file inside the candidate would invalidate its closed-world
# manifest on subsequent QA checks.
if ([string]::IsNullOrWhiteSpace($LogDirectory)) {
    $LogDirectory = Join-Path (Split-Path -Parent $root) 'LateBrakePlaytestLogs'
}
$absoluteLogs = [IO.Path]::GetFullPath($LogDirectory)
$relative = [IO.Path]::GetRelativePath($root, $absoluteLogs)
if ($relative -eq '.' -or
    ($relative -ne '..' -and -not $relative.StartsWith('..' + [IO.Path]::DirectorySeparatorChar))) {
    throw 'LogDirectory must be outside the immutable packaged candidate folder.'
}
if (Test-Path -LiteralPath $absoluteLogs) {
    $logsItem = Get-Item -LiteralPath $absoluteLogs -Force -ErrorAction Stop
    if (-not $logsItem.PSIsContainer -or
        ($logsItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'LogDirectory must be an ordinary directory, not a junction or file.'
    }
} else {
    [void](New-Item -ItemType Directory -Path $absoluteLogs -Force -ErrorAction Stop)
}
$session = 'LateBrake-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') +
    '-' + [Guid]::NewGuid().ToString('N').Substring(0, 8)
$log = Join-Path $absoluteLogs ($session + '.log')
$receipt = Join-Path $absoluteLogs ($session + '.json')
if ((Test-Path -LiteralPath $log -PathType Leaf) -or
    (Test-Path -LiteralPath $receipt -PathType Leaf)) {
    throw 'Refusing to overwrite an existing game log or session receipt.'
}
Write-Host "Launching real candidate: $($game.FullName)"
Write-Host "Private Unreal log: $log"
Write-Host 'Play the game yourself; close it to record exit information.'
Write-Host 'This is a diagnostic run, NOT an automated gameplay qualification.'
$started = [DateTimeOffset]::UtcNow
$exitCode = $null
$startFailure = $null
try {
    # -ABSLOG is documented by Epic. Explicitly quote a path with spaces
    # because PowerShell Start-Process joins ArgumentList elements.
    $argsForUnreal = @('-log', ('-ABSLOG="' + $log + '"'))
    $process = Start-Process -FilePath $game.FullName -WorkingDirectory $game.DirectoryName -ArgumentList $argsForUnreal -PassThru -Wait -ErrorAction Stop
    $exitCode = $process.ExitCode
} catch {
    $startFailure = $_.Exception.Message
}
$ended = [DateTimeOffset]::UtcNow

$report = [ordered]@{
    schema_version = 1
    status = 'UNVERIFIED_PERSONAL_RUNTIME_PROBE'
    gameplay_verified = $false
    source = 'locally_launched_packaged_win64_candidate'
    candidate_directory = $root
    executable_path = $game.FullName
    executable_sha256 = $gameSha256
    candidate_manifest_sha256 = $manifestSha256
    started_utc = $started.ToString('o')
    ended_utc = $ended.ToString('o')
    runtime_seconds = [math]::Round(($ended - $started).TotalSeconds, 3)
    process_exit_code = $exitCode
    process_start_error = $startFailure
    unreal_log_path = $log
    unreal_log_present = (Test-Path -LiteralPath $log -PathType Leaf)
    note = 'EXE exit status and a log never prove playable gameplay; record hands-on QA separately.'
}
$report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $receipt -Encoding utf8 -NoNewline
Write-Host "Private session receipt: $receipt"
if ($report.unreal_log_present) {
    Write-Host "Unreal wrote a diagnostic log: $log"
} else {
    Write-Warning 'No ABSLOG file was produced; inspect startup/crash and packaged Saved/Logs.'
}
if ($null -ne $startFailure) {
    throw "The packaged EXE could not start: $startFailure. Session diagnostics: $receipt"
}
Write-Host "Packaged game exited with code $exitCode."
if ($exitCode -ne 0) {
    throw "The game returned a nonzero process exit code; inspect $receipt and $log."
}
Write-Host 'Process exited normally. Only hands-on controls, rendering, racing and save tests can confirm gameplay.'
