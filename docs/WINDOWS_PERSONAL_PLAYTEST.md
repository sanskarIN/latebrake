# Build LateBrake for personal Windows testing

**No ready-to-download LateBrake Windows game EXE has been produced or
verified by the source-only GitHub CI.** This guide builds the **actual
Unreal game** on a Windows PC; it does not download an Unreal installer,
rename a source ZIP, or turn the standalone CMake tests into a game.

## What you need on your Windows PC

- A legally authorized installed **Unreal Engine 5.8**, including
  `UnrealEditor.exe`, `Build.bat` and `RunUAT.bat`.
- Visual Studio's compatible **C++ game-development workload**, Windows
  SDK and other build components required by that UE 5.8 installation.
- **Git**, **Git LFS** and **PowerShell 7 (`pwsh`)**.
- Adequate free disk space for the Editor/UHT compile and cooked Win64
  Development game, and a working Windows graphics driver.

Do not use GitHub's “Download ZIP” as a substitute for a clean Git
checkout: the candidate pipeline deliberately requires traceable Git
source metadata. Run the following once:

```powershell
git clone https://github.com/sanskarIN/latebrake.git
cd latebrake
git lfs install
git lfs pull
```

## Optional: diagnose unmerged PR #377 with a real local UE5.8 build

The regular commands build the checked-out branch (usually main). To
investigate [draft PR #377](https://github.com/sanskarIN/latebrake/pull/377)
without a GitHub self-hosted runner, the owner may use a **fresh separate
clone** on an isolated Windows test PC. First review the entire PR diff,
including C++ and build scripts that will run locally. Never run unreviewed
PR code on a privileged or shared machine.

From PowerShell 7 inside that fresh clone, enter the current **live head
SHA** from the GitHub PR, not one copied from an older document:

~~~powershell
$ExpectedHead = Read-Host 'Paste the current full 40-character PR #377 head SHA'
if ($ExpectedHead -cnotmatch '^[0-9a-f]{40}

Double-click **`Build-Windows-Playtest.cmd`** in the repository root.
The wrapper defaults to
`C:\Program Files\Epic Games\UE_5.8` and uses PowerShell 7.

If the authorized engine lives elsewhere, run this from PowerShell 7
inside the repository:

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:\UE_5.8'
```

To open the game immediately **only after a successful real build**:

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:\UE_5.8' -Launch
```

The script calls the existing `Build-PlayableCandidate.ps1`, which
compiles the real `LatebrakeEditor` target through UE/UHT, asks licensed
Unreal Editor to create/save the genuine project circuit level (if it
does not exist), cooks a Win64 Development game through `RunUAT.bat`,
and verifies the package files and source/map/engine SHA256 manifest.

Each run gets a **new** directory below
`out/Win64Playtests/LateBrake-Win64-<timestamp>-<unique-id>/`.
This location is Git-ignored; previous packages are never replaced.
A successful run prints the **full exact path** of the packaged
`Latebrake.exe` and the SHA256 of that EXE and its candidate manifest.

**Keep all cooked files in their packaged directory.**
`Latebrake.exe` alone is not a portable game: moving it without its
supporting cooked assets can cause startup errors.

## Private diagnostic game run (new)

After a **successful** game package, run the genuine EXE through the
candidate verifier/diagnostic launcher, not an unrelated desktop EXE.
The original build command's `-Launch` option now does this
automatically, then waits for you to close the game:

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:\UE_5.8' -Launch
```

To **relaunch an already-built** private candidate without compiling again,
drag the full built-candidate **folder onto** `Play-Windows-Candidate.cmd`
(which passes the folder as its first argument), or use:

```powershell
pwsh ./scripts/Run-WindowsPlaytest.ps1 -CandidateDirectory 'D:\LatebrakeCandidates\YourRealWin64BuildFolder'
```

The new launcher checks the complete closed-world SHA256 candidate
manifest, verifies Win64 EXE/cooked-content structure, then launches the
packaged `Latebrake.exe` with Unreal's documented `-log` and `-ABSLOG`
arguments. The latter directs the **private diagnostic runtime log**
outside the immutable candidate folder, to a sibling
`LateBrakePlaytestLogs` directory by default. You can customize
`-LogDirectory`, but it must not be inside the game folder. The launcher
also rejects **junction/symlink parent directories** in either candidate
or diagnostic path: a folder that *appears* outside the package must not
resolve through a linked ancestor back inside the fingerprinted game.
For a custom location on another Windows drive, use an ordinary directory
without reparse-point parents.

After **you close the game**, the helper records an equally private
schema-2 JSON receipt with source artifact checksums, UTC times,
process exit code, log presence and **the exact runtime log SHA256/byte
length** (null if the game crashed before writing a log). It also runs
the existing `Verify-RuntimeStartupLog.ps1` check for track
geometry/gate/scenery markers when the log exists; the receipt records
whether this **text-only** audit passed and why it failed. No network upload occurs. A zero exit code
**does not** qualify rendering, user input, racing, saves or hardware
performance.

The launcher **now checks the complete archive again after the process
exits**, independently rehashing every cooked file and rejecting extra
files, changed EXE bytes or a rewritten manifest. The schema-2 receipt
contains `candidate_integrity_at_exit_pass` and
`candidate_integrity_at_exit_error`. It writes the receipt **even if
this final archive check fails**, then exits with an error rather than
printing a misleading success. A game that creates `Saved/`, cache,
configuration files or other output *inside the sealed package* will
trigger this failure even with `-ABSLOG` directed outside; keep the
original log/receipt and inspect the new files. Do not delete or
re-fingerprint changed evidence to make the same session appear clean.

Earlier schema-1 and schema-2 receipts lacked a post-run seal and
remain reviewable as historical, **unverified** observations; the QA
review records `candidate_integrity_at_exit_verified: false`.
New receipts with a negative/contradictory seal are rejected from
further QA review, even if the game archive is later restored.

If the process crashes too early, an Unreal log might not exist;
retain the real UAT/Windows crash information.

Official Epic documentation:
[Unreal Engine command-line argument reference](
https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-command-line-arguments-reference)
describes `ABSLOG` as an absolute log filename argument.

## Analyze accepted race progression in a real Development log

The source-level GameMode now writes a small non-shipping
`LATEBRAKE_RACE_AUDIT v=1` event at race start/restart and after each
**actually accepted** player checkpoint. A local parser can detect
skipped/duplicated gates, inconsistent lap totals or incomplete finishes:

```powershell
python ./scripts/AnalyzeRaceAudit.py 'D:\\LateBrakePlaytestLogs\\YourActualSession.log' --require-finish
```

Use a genuine game log from the private diagnostic runner, not a synthetic
fixture. The parser never certifies game provenance or hardware gameplay.
Compare its output with the original hashed receipt and hands-on video;
read [race audit QA](RACE_AUDIT_QA.md) for limitations and fixtures.

For a **hash-bound private audit report**, use the same recorded runtime
receipt (schema 2) produced by `Run-WindowsPlaytest.ps1`:

```powershell
python ./scripts/Review-WindowsPlaytest.py audit `
  --receipt 'D:\\LateBrakePlaytestLogs\\LateBrake-your-session.json' `
  --output 'D:\\LateBrakeQA\\race-audit-review.json' `
  --require-finish
```

This newer mode rehashes the EXE, complete cooked archive, manifest and
session's original runtime log **before** interpreting gate events. The
result links source SHA, engine Build.version hash, circuit-map hash,
receipt and log SHA256 to reported accepted checkpoints. It rejects
tampered/swapped/missing logs, impossible gate sequences and an absent
reported finish when requested. Both the JSON receipt and log text remain
editable: matching hashes are **not proof of authentic engine compilation,
physical driving or independently observed gameplay**. This command
never launches the EXE, uploads logs, modifies the game archive or
qualifies a release.

The bare `AnalyzeRaceAudit.py` command remains useful for exploratory
parsing of other local log text, but **does not check any receipt or
candidate hashes**. Use the hash-bound review when investigating an
actual personal Windows candidate.

## Record a private, self-reported Win64 QA review

After closing the real game session, the diagnostic runner prints the path
to a **private JSON receipt**. Keep it outside the candidate alongside the
runtime logs. On your own workstation, make an editable checklist tied to
the actual runtime receipt, candidate manifest and EXE hashes:

```powershell
python ./scripts/Review-WindowsPlaytest.py init `
  --receipt 'D:\LateBrakePlaytestLogs\LateBrake-your-session.json' `
  --output 'D:\LateBrakeQA\manual-checklist.json'
```

Edit `manual-checklist.json` *after actual hands-on driving*. For each
check, set `result` to `pass`, `fail`, or `not_tested`; give an actual
observation in `notes` (at least 12 characters for pass/fail). Record
Windows version and CPU/GPU/controller details if you wish. The nine
required checks include scene/physics, brake/reverse, steering/camera,
GO/gates, pause/focus/restart, recovery penalty, save/reload and
hardware performance. **Never substitute source tests for manual
observations.** For example, if no real game started, leave everything
`not_tested` or describe the failure honestly.

```powershell
python ./scripts/Review-WindowsPlaytest.py evaluate `
  --receipt 'D:\LateBrakePlaytestLogs\LateBrake-your-session.json' `
  --checklist 'D:\LateBrakeQA\manual-checklist.json' `
  --output 'D:\LateBrakeQA\manual-review.json'
```

The offline evaluator refuses a changed EXE/manifest, another session's
receipt, missing checks, undocumented pass/fail choices and output paths
inside the sealed package. It now also **independently rehashes every cooked
file** listed in the candidate's schema-2 manifest, rejects any missing
or extra file, duplicate/case-colliding path, unsafe relative path,
symlink or junction (including empty linked folders), and rejects a
modified or malformed file record. The validation is read-only: it does
not overwrite evidence, upload files or run a game. The report always says
`SELF_REPORTED_WIN64_QA_UNVERIFIED` and
`ready_for_release: false`, **even when every test was marked pass**.
That is intentional: user-authored JSON can be edited and does not prove
independent QA.

Before treating a candidate as a genuine playable build, also rerun
`scripts/Test-CandidateManifest.ps1` against the whole archive,
preserve real UHT/UAT build receipts, inspect actual installed driving
and satisfy [native Unreal merge-readiness checklist](UNREAL_NATIVE_MERGE_READINESS.md).
The Python reviewer independently verifies the receipt, every
manifest-listed cooked file and (for schema-2 runtime receipts) the
**recorded log file bytes**. The log must still exist at the exact session
path outside the game archive and its SHA256 and length must match;
renamed, modified, removed, switched or linked logs fail the review.
An actual crash before Unreal wrote a log is recorded with null log
metadata and remains unverified, not silently counted as success.
Older schema-1 runtime receipts remain reviewable but report
`runtime_log_integrity_verified: false`; they have no retroactive log
checksum. New schema-2 reviews expose this field when the stored log is
unchanged, while `ready_for_release` remains **false** regardless.
This is a **second integrity check**, not a replacement for the native
PowerShell package verifier, real UAT provenance, Windows installed
playtesting or independent QA. It cannot authenticate a forged receipt
or prove gameplay. Protect your private hardware details and
do not upload personal QA logs without reviewing them.

## What to test personally

1. Launch the actual `Latebrake.exe` printed by the script. If it fails,
   keep the full UHT/UAT console output and the real Unreal game log.
2. Confirm the circuit, physical car, engine-provided meshes and HUD
   appear; try basic steering, accelerate, reverse, brake and camera switch.
3. Test the 3/2/1/GO gate, each of four sequential checkpoints, laps,
   Time Attack, restart, paid recovery, pause/unpause and focus loss.
4. Drive several sessions, inspect save/reload, offline garage/career,
   rival car, race Results and advisory launch/drift readings.
5. Record Windows version, CPU/GPU, frame rates, issues and the printed
   EXE/candidate-manifest SHA256. Do not report an untested option as passed.

An output directory and an EXE signature alone do not prove that a game
really starts or handles input. A Windows **personal test candidate**
should remain **UNVERIFIED** until you run and inspect it yourself.
The formal QA/release gate is stricter and remains open in
[native Unreal merge-readiness checklist](UNREAL_NATIVE_MERGE_READINESS.md);
Android is independently tracked in
[issue #13](https://github.com/sanskarIN/latebrake/issues/13).

## Troubleshooting

- **Missing Unreal Engine 5.8 component**: install/configure the
  authorized engine or pass the correct `-EngineRoot`; no EXE can be
  built from portable CMake checks alone.
- **PowerShell 7 not found**: install `pwsh` before using the double-click
  entry point.
- **Dirty/untracked source**: commit intentional source changes or use
  a fresh `git clone`; only the Editor-authored starter map exception
  is accepted and separately hashed.
- **UHT/C++ or cooker failure**: inspect the actual Unreal error output.
  This helper does **not** suppress compiler or packaging errors.
- **Missing packaged Latebrake.exe**: do not distribute or rename a
  different EXE. Preserve failed build logs, inspect the real UAT output
  and correct the packaging problem.

This helper does not upload private game files or change GitHub release
status. Source tests can check script structure, but only an installed
UE 5.8 Windows workstation can establish actual compilation/gameplay.
) {
  throw 'Provide a 40-character lowercase Git commit SHA'
}
git fetch --no-tags origin pull/377/head
if ($LASTEXITCODE -ne 0) { throw 'PR fetch failed' }
$FetchedHead = (git rev-parse FETCH_HEAD).Trim()
if ($FetchedHead -cne $ExpectedHead) {
  throw 'Live PR head differs from reviewed SHA; re-review the PR'
}
git switch --detach $ExpectedHead
if ($LASTEXITCODE -ne 0 -or
    (git rev-parse HEAD).Trim() -cne $ExpectedHead) {
  throw 'Could not confirm exact reviewed source'
}
git lfs pull
if ($LASTEXITCODE -ne 0) { throw 'Required LFS content unavailable' }
git status --short
~~~

Inspect any unexpected git status output rather than bypassing source
provenance. With a genuinely installed and licensed UE5.8, replace the
sample engine path below with the real path:

~~~powershell
pwsh ./scripts/Compile-UnrealEditor.ps1 -EngineRoot 'D:\UE_5.8'
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:\UE_5.8' -Launch
~~~

These scripts must genuinely compile Unreal Editor/UHT, cook the original
game and launch the resulting private Win64 candidate. Preserve true source
SHA, Editor/UAT logs, engine and cooked-map hashes, candidate manifest,
runtime receipt and genuine driving observations. Source tests or a generated
EXE alone do **not** establish a playable game.

**This local diagnostic does not replace** the owner-trusted
[manual UE5.8 Editor-then-Win64 validation](UNREAL_DRAFT_PR_VALIDATION.md)
or installed-game acceptance required before the native gameplay PR is
merged. It also does not satisfy the separate explicit **exact-main SHA
approval** for a source-only v0.5.100 pre-release. No release is created
by these local diagnostic commands. Never upload private logs or tokens.

## One-click: personal candidate

Double-click **`Build-Windows-Playtest.cmd`** in the repository root.
The wrapper defaults to
`C:\Program Files\Epic Games\UE_5.8` and uses PowerShell 7.

If the authorized engine lives elsewhere, run this from PowerShell 7
inside the repository:

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:\UE_5.8'
```

To open the game immediately **only after a successful real build**:

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:\UE_5.8' -Launch
```

The script calls the existing `Build-PlayableCandidate.ps1`, which
compiles the real `LatebrakeEditor` target through UE/UHT, asks licensed
Unreal Editor to create/save the genuine project circuit level (if it
does not exist), cooks a Win64 Development game through `RunUAT.bat`,
and verifies the package files and source/map/engine SHA256 manifest.

Each run gets a **new** directory below
`out/Win64Playtests/LateBrake-Win64-<timestamp>-<unique-id>/`.
This location is Git-ignored; previous packages are never replaced.
A successful run prints the **full exact path** of the packaged
`Latebrake.exe` and the SHA256 of that EXE and its candidate manifest.

**Keep all cooked files in their packaged directory.**
`Latebrake.exe` alone is not a portable game: moving it without its
supporting cooked assets can cause startup errors.

## Private diagnostic game run (new)

After a **successful** game package, run the genuine EXE through the
candidate verifier/diagnostic launcher, not an unrelated desktop EXE.
The original build command's `-Launch` option now does this
automatically, then waits for you to close the game:

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:\UE_5.8' -Launch
```

To **relaunch an already-built** private candidate without compiling again,
drag the full built-candidate **folder onto** `Play-Windows-Candidate.cmd`
(which passes the folder as its first argument), or use:

```powershell
pwsh ./scripts/Run-WindowsPlaytest.ps1 -CandidateDirectory 'D:\LatebrakeCandidates\YourRealWin64BuildFolder'
```

The new launcher checks the complete closed-world SHA256 candidate
manifest, verifies Win64 EXE/cooked-content structure, then launches the
packaged `Latebrake.exe` with Unreal's documented `-log` and `-ABSLOG`
arguments. The latter directs the **private diagnostic runtime log**
outside the immutable candidate folder, to a sibling
`LateBrakePlaytestLogs` directory by default. You can customize
`-LogDirectory`, but it must not be inside the game folder. The launcher
also rejects **junction/symlink parent directories** in either candidate
or diagnostic path: a folder that *appears* outside the package must not
resolve through a linked ancestor back inside the fingerprinted game.
For a custom location on another Windows drive, use an ordinary directory
without reparse-point parents.

After **you close the game**, the helper records an equally private
schema-2 JSON receipt with source artifact checksums, UTC times,
process exit code, log presence and **the exact runtime log SHA256/byte
length** (null if the game crashed before writing a log). It also runs
the existing `Verify-RuntimeStartupLog.ps1` check for track
geometry/gate/scenery markers when the log exists; the receipt records
whether this **text-only** audit passed and why it failed. No network upload occurs. A zero exit code
**does not** qualify rendering, user input, racing, saves or hardware
performance.

The launcher **now checks the complete archive again after the process
exits**, independently rehashing every cooked file and rejecting extra
files, changed EXE bytes or a rewritten manifest. The schema-2 receipt
contains `candidate_integrity_at_exit_pass` and
`candidate_integrity_at_exit_error`. It writes the receipt **even if
this final archive check fails**, then exits with an error rather than
printing a misleading success. A game that creates `Saved/`, cache,
configuration files or other output *inside the sealed package* will
trigger this failure even with `-ABSLOG` directed outside; keep the
original log/receipt and inspect the new files. Do not delete or
re-fingerprint changed evidence to make the same session appear clean.

Earlier schema-1 and schema-2 receipts lacked a post-run seal and
remain reviewable as historical, **unverified** observations; the QA
review records `candidate_integrity_at_exit_verified: false`.
New receipts with a negative/contradictory seal are rejected from
further QA review, even if the game archive is later restored.

If the process crashes too early, an Unreal log might not exist;
retain the real UAT/Windows crash information.

Official Epic documentation:
[Unreal Engine command-line argument reference](
https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-command-line-arguments-reference)
describes `ABSLOG` as an absolute log filename argument.

## Analyze accepted race progression in a real Development log

The source-level GameMode now writes a small non-shipping
`LATEBRAKE_RACE_AUDIT v=1` event at race start/restart and after each
**actually accepted** player checkpoint. A local parser can detect
skipped/duplicated gates, inconsistent lap totals or incomplete finishes:

```powershell
python ./scripts/AnalyzeRaceAudit.py 'D:\\LateBrakePlaytestLogs\\YourActualSession.log' --require-finish
```

Use a genuine game log from the private diagnostic runner, not a synthetic
fixture. The parser never certifies game provenance or hardware gameplay.
Compare its output with the original hashed receipt and hands-on video;
read [race audit QA](RACE_AUDIT_QA.md) for limitations and fixtures.

For a **hash-bound private audit report**, use the same recorded runtime
receipt (schema 2) produced by `Run-WindowsPlaytest.ps1`:

```powershell
python ./scripts/Review-WindowsPlaytest.py audit `
  --receipt 'D:\\LateBrakePlaytestLogs\\LateBrake-your-session.json' `
  --output 'D:\\LateBrakeQA\\race-audit-review.json' `
  --require-finish
```

This newer mode rehashes the EXE, complete cooked archive, manifest and
session's original runtime log **before** interpreting gate events. The
result links source SHA, engine Build.version hash, circuit-map hash,
receipt and log SHA256 to reported accepted checkpoints. It rejects
tampered/swapped/missing logs, impossible gate sequences and an absent
reported finish when requested. Both the JSON receipt and log text remain
editable: matching hashes are **not proof of authentic engine compilation,
physical driving or independently observed gameplay**. This command
never launches the EXE, uploads logs, modifies the game archive or
qualifies a release.

The bare `AnalyzeRaceAudit.py` command remains useful for exploratory
parsing of other local log text, but **does not check any receipt or
candidate hashes**. Use the hash-bound review when investigating an
actual personal Windows candidate.

## Record a private, self-reported Win64 QA review

After closing the real game session, the diagnostic runner prints the path
to a **private JSON receipt**. Keep it outside the candidate alongside the
runtime logs. On your own workstation, make an editable checklist tied to
the actual runtime receipt, candidate manifest and EXE hashes:

```powershell
python ./scripts/Review-WindowsPlaytest.py init `
  --receipt 'D:\LateBrakePlaytestLogs\LateBrake-your-session.json' `
  --output 'D:\LateBrakeQA\manual-checklist.json'
```

Edit `manual-checklist.json` *after actual hands-on driving*. For each
check, set `result` to `pass`, `fail`, or `not_tested`; give an actual
observation in `notes` (at least 12 characters for pass/fail). Record
Windows version and CPU/GPU/controller details if you wish. The nine
required checks include scene/physics, brake/reverse, steering/camera,
GO/gates, pause/focus/restart, recovery penalty, save/reload and
hardware performance. **Never substitute source tests for manual
observations.** For example, if no real game started, leave everything
`not_tested` or describe the failure honestly.

```powershell
python ./scripts/Review-WindowsPlaytest.py evaluate `
  --receipt 'D:\LateBrakePlaytestLogs\LateBrake-your-session.json' `
  --checklist 'D:\LateBrakeQA\manual-checklist.json' `
  --output 'D:\LateBrakeQA\manual-review.json'
```

The offline evaluator refuses a changed EXE/manifest, another session's
receipt, missing checks, undocumented pass/fail choices and output paths
inside the sealed package. It now also **independently rehashes every cooked
file** listed in the candidate's schema-2 manifest, rejects any missing
or extra file, duplicate/case-colliding path, unsafe relative path,
symlink or junction (including empty linked folders), and rejects a
modified or malformed file record. The validation is read-only: it does
not overwrite evidence, upload files or run a game. The report always says
`SELF_REPORTED_WIN64_QA_UNVERIFIED` and
`ready_for_release: false`, **even when every test was marked pass**.
That is intentional: user-authored JSON can be edited and does not prove
independent QA.

Before treating a candidate as a genuine playable build, also rerun
`scripts/Test-CandidateManifest.ps1` against the whole archive,
preserve real UHT/UAT build receipts, inspect actual installed driving
and satisfy [native Unreal merge-readiness checklist](UNREAL_NATIVE_MERGE_READINESS.md).
The Python reviewer independently verifies the receipt, every
manifest-listed cooked file and (for schema-2 runtime receipts) the
**recorded log file bytes**. The log must still exist at the exact session
path outside the game archive and its SHA256 and length must match;
renamed, modified, removed, switched or linked logs fail the review.
An actual crash before Unreal wrote a log is recorded with null log
metadata and remains unverified, not silently counted as success.
Older schema-1 runtime receipts remain reviewable but report
`runtime_log_integrity_verified: false`; they have no retroactive log
checksum. New schema-2 reviews expose this field when the stored log is
unchanged, while `ready_for_release` remains **false** regardless.
This is a **second integrity check**, not a replacement for the native
PowerShell package verifier, real UAT provenance, Windows installed
playtesting or independent QA. It cannot authenticate a forged receipt
or prove gameplay. Protect your private hardware details and
do not upload personal QA logs without reviewing them.

## What to test personally

1. Launch the actual `Latebrake.exe` printed by the script. If it fails,
   keep the full UHT/UAT console output and the real Unreal game log.
2. Confirm the circuit, physical car, engine-provided meshes and HUD
   appear; try basic steering, accelerate, reverse, brake and camera switch.
3. Test the 3/2/1/GO gate, each of four sequential checkpoints, laps,
   Time Attack, restart, paid recovery, pause/unpause and focus loss.
4. Drive several sessions, inspect save/reload, offline garage/career,
   rival car, race Results and advisory launch/drift readings.
5. Record Windows version, CPU/GPU, frame rates, issues and the printed
   EXE/candidate-manifest SHA256. Do not report an untested option as passed.

An output directory and an EXE signature alone do not prove that a game
really starts or handles input. A Windows **personal test candidate**
should remain **UNVERIFIED** until you run and inspect it yourself.
The formal QA/release gate is stricter and remains open in
[native Unreal merge-readiness checklist](UNREAL_NATIVE_MERGE_READINESS.md);
Android is independently tracked in
[issue #13](https://github.com/sanskarIN/latebrake/issues/13).

## Troubleshooting

- **Missing Unreal Engine 5.8 component**: install/configure the
  authorized engine or pass the correct `-EngineRoot`; no EXE can be
  built from portable CMake checks alone.
- **PowerShell 7 not found**: install `pwsh` before using the double-click
  entry point.
- **Dirty/untracked source**: commit intentional source changes or use
  a fresh `git clone`; only the Editor-authored starter map exception
  is accepted and separately hashed.
- **UHT/C++ or cooker failure**: inspect the actual Unreal error output.
  This helper does **not** suppress compiler or packaging errors.
- **Missing packaged Latebrake.exe**: do not distribute or rename a
  different EXE. Preserve failed build logs, inspect the real UAT output
  and correct the packaging problem.

This helper does not upload private game files or change GitHub release
status. Source tests can check script structure, but only an installed
UE 5.8 Windows workstation can establish actual compilation/gameplay.
