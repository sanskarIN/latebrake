# Non-shipping authoritative race-audit event log

**Development source only — no verified UE 5.8 build or installed game.**
This feature records **accepted** player gates, event configuration, race
attempts and finish state from the actual `RaceProgress` callsite in the
Unreal GameMode. It does **not** simulate driving, verify physical collision
or certify a playable release. No new gameplay scoring or save fields exist.

## Why this helps a real Windows playtest

The startup log already reports circuit geometry and checkpoint actor
construction, but it cannot distinguish a car that genuinely finishes
the course from one that never drives a lap. When the authorized Unreal
Development game eventually runs, this low-volume source instrumentation
will log a bounded event record at each attempt start and **only after**
`Race.PassCheckpoint(index)` accepts an ordered player gate.

Every `begin` event reports:

- `attempt`: monotonic within one running Unreal world;
- `gates` and `laps`: actual selected `RaceProgress` configuration;
- `track`: whether circuit startup was ready at this attempt start;
- `grid`: `-1` for initial spawn (not yet observed), `0` for rejected
  restart placement and `1` for verified restart placement.

Every `gate` event reports the accepted gate index, completed lap count,
next expected gate, six-decimal-place elapsed race seconds, active-timed
lap count and the `RaceProgress` finished flag. There are no raw car
coordinates, user identifiers, network calls or per-frame log spam. All
race-audit logging is compiled out of Shipping builds.

The log format is `LATEBRAKE_RACE_AUDIT v=1`. Example below is
**synthetic**, for explaining the format; it is NOT installed-game evidence:

```text
LogLatebrakeRaceAudit: Display: LATEBRAKE_RACE_AUDIT v=1 action=begin attempt=1 gates=4 laps=1 track=1 grid=-1
LogLatebrakeRaceAudit: Display: LATEBRAKE_RACE_AUDIT v=1 action=gate attempt=1 gate=0 lap=0 next=1 elapsed=1.000000 active=0 finished=0
```

## Local offline validator

Run the real private `Run-WindowsPlaytest.ps1` on an authorized UE 5.8
Windows machine and **keep its original receipt, package manifest and
hashed runtime log together**. Parse only that real log:

```powershell
python ./scripts/AnalyzeRaceAudit.py 'D:\LateBrakePlaytestLogs\YourActualSession.log'
python ./scripts/AnalyzeRaceAudit.py 'D:\LateBrakePlaytestLogs\YourActualSession.log' --require-finish
```

The parser rejects truncated/malformed rows, unsupported schemas, skipped
or duplicate gates, invented finish flags, backdated race clocks,
cross-attempt reuse, impossible active-lap counts, blocked start attempts
that somehow score, and missing completed races when requested. Output
retains `source_authenticity_verified: false`,
`installed_gameplay_verified: false`, and
`ready_for_release: false` even when its counters are internally valid.
A malicious or accidental human edit can reproduce these text lines;
they are not cryptographic runtime attestation.

## Source testing

```powershell
python -m unittest discover -s Tests/Editor -p 'RaceAuditLogTests.py' -v
```

Existing `core-tests.yml` runs this suite alongside all other Unreal
source-contract tests. This does not invoke UnrealHeaderTool or compile a
genuine game target.

## Installed-game acceptance remains required

1. Build the exact source commit with authorized Unreal Engine 5.8,
   verify UHT, cook a genuinely owned circuit, and preserve immutable
   source/map/engine/package checksums.
2. Run the actual Development executable. Verify startup markers with
   `Verify-RuntimeStartupLog.ps1` and hash the session log with the
   existing private receipt tools.
3. Drive all four physical checkpoint gates in correct order at normal
   frame rates, record lap transitions, restart, manual pause/focus
   loss, a failed recovery, and a valid full race finish.
4. Confirm the parser reports correct gate/lap counts **without**
   accepting reverse/teleport/paused crossings; compare against the
   actual screen video and game controls.
5. Inspect packaged Windows and Android gameplay separately, including
   controller/touch input, camera, saves, frame times and errors. Keep
   [playable-release gate #148](https://github.com/sanskarIN/latebrake/issues/148)
   open until independent actual-device evidence passes.

**Never claim a successful parser report proves installed gameplay.**
