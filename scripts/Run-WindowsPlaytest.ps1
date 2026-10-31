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
# Check the candidate AND all output directory ancestors for junctions.
# This blocks paths that look external but resolve back into a cooked archive.
$absoluteLogs = & (Join-Path $PSScriptRoot 'Assert-SafeWindowsPlaytestLogDirectory.ps1') `
    -CandidateRoot $root -LogDirectory $LogDirectory
if (-not $?) { throw 'Unsafe Win64 runtime diagnostic directory.' }
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

# A real Unreal executable may write Saved/, cache or generated files
# inside its packaged directory even when -ABSLOG points elsewhere.
# Preserve the launch result in a private receipt, but NEVER claim the
# cooked candidate is still fingerprint-identical without rechecking
# ALL archive files, plus original manifest and executable hashes.
$archiveIntegrityAtExitPass = $false
$archiveIntegrityAtExitError = $null
try {
    & (Join-Path $PSScriptRoot 'Test-PostRunWin64CandidateIntegrity.ps1') `
        -ManifestJson $manifest -GameExecutable $game.FullName `
        -ExpectedManifestSha256 $manifestSha256 -ExpectedExecutableSha256 $gameSha256
    if (-not $?) {
        throw 'Post-run candidate archive integrity script reported failure.'
    }
    $archiveIntegrityAtExitPass = $true
} catch {
    $archiveIntegrityAtExitError = $_.Exception.Message
}

# Analyze original runtime startup markers when a log exists. The source
# marker grammar can catch missing geometry/track initialization. It cannot
# prove actual rendering, vehicle physics or installed gameplay.
$startupLogMarkersPass = $false
$startupLogError = $null
if (Test-Path -LiteralPath $log -PathType Leaf) {
    try {
        & (Join-Path $PSScriptRoot 'Verify-RuntimeStartupLog.ps1') -LogPath $log
        $startupLogMarkersPass = $true
    } catch {
        $startupLogError = $_.Exception.Message
    }
}

# Seal the exact finished Unreal log alongside the executable and
# manifest. A missing/early-crash log remains a valid *failed* probe,
# represented explicitly with null metadata; no success is inferred.
$logPresent = Test-Path -LiteralPath $log -PathType Leaf
$logSha256 = $null
$logBytes = $null
if ($logPresent) {
    $logItem = Get-Item -LiteralPath $log -Force -ErrorAction Stop
    if (($logItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Refusing to seal a symlinked or junction-redirected runtime log.'
    }
    $logSha256 = (Get-FileHash -LiteralPath $log -Algorithm SHA256).Hash.ToLowerInvariant()
    $logBytes = [int64]$logItem.Length
}

$report = [ordered]@{
    schema_version = 2
    status = 'UNVERIFIED_PERSONAL_RUNTIME_PROBE'
    gameplay_verified = $false
    source = 'locally_launched_packaged_win64_candidate'
    candidate_directory = $root
    executable_path = $game.FullName
    executable_sha256 = $gameSha256
    candidate_manifest_sha256 = $manifestSha256
    candidate_integrity_at_exit_pass = $archiveIntegrityAtExitPass
    candidate_integrity_at_exit_error = $archiveIntegrityAtExitError
    started_utc = $started.ToString('o')
    ended_utc = $ended.ToString('o')
    runtime_seconds = [math]::Round(($ended - $started).TotalSeconds, 3)
    process_exit_code = $exitCode
    process_start_error = $startFailure
    unreal_log_path = $log
    unreal_log_present = $logPresent
    unreal_log_sha256 = $logSha256
    unreal_log_bytes = $logBytes
    startup_log_markers_pass = $startupLogMarkersPass
    startup_log_error = $startupLogError
    note = 'EXE exit status and a log never prove playable gameplay; record hands-on QA separately.'
}
$report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $receipt -Encoding utf8 -NoNewline
Write-Host "Private session receipt: $receipt"
if ($report.unreal_log_present) {
    Write-Host "Unreal wrote a diagnostic log: $log"
} else {
    Write-Warning 'No ABSLOG file was produced; inspect startup/crash and packaged Saved/Logs.'
}
if (-not $startupLogMarkersPass) {
    Write-Warning "Track startup markers are NOT verified. Inspect $receipt and $log."
}
if (-not $archiveIntegrityAtExitPass) {
    # Do not discard crash/log diagnostics if the game wrote into or
    # otherwise modified the sealed package. The private receipt is
    # written first and explicitly marks the archive as NOT preserved.
    throw "Packaged candidate changed or failed integrity after game exit: $archiveIntegrityAtExitError. Private receipt: $receipt"
}
if ($null -ne $startFailure) {
    throw "The packaged EXE could not start: $startFailure. Session diagnostics: $receipt"
}
Write-Host "Packaged game exited with code $exitCode."
if ($exitCode -ne 0) {
    throw "The game returned a nonzero process exit code; inspect $receipt and $log."
}
Write-Host 'Process exited normally. Only hands-on controls, rendering, racing and save tests can confirm gameplay.'
