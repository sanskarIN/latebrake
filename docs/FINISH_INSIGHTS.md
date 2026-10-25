# v0.5.5 — Finished Race Insights (source-only preview)

**Published source-only preview. No playable v0.5.5 game or compiled executable is available.**

## What a player will see

After a real race finishes with a recordable clock, the prototype
Results menu can show the **achieved fastest timed lap**, the
difference between that lap and the **within-run best-sector
theoretical lap**, and a fastest-to-slowest **timed lap spread** when
at least two properly timed laps exist. Non-compact desktop results
HUD gets a compact version of the same statistics. Compact phone HUD
does not add new lines during racing.

The theoretical lap is only a sum of the smallest valid split recorded
for each sector during **this run**. It is **not** an actually achieved
lap, a global benchmark or a persisted personal best. The gap is
clamped by validation: if sector data are incomplete, invalid or
paradoxically slower than the achieved best, no gap is claimed.

The lap spread expresses variability, not competitiveness. A single
valid timed lap has **no** multi-lap spread. Zero-duration or invalid
clock samples cannot fabricate one.

## Pure C++20 implementation

- `RecentTimedLaps.h` implements an eight-record, fixed-size no-heap
  ring. Each record carries physical lap number and strictly positive,
  finite duration; malformed times and zero-duration laps are excluded.
  History never persists across a race restart or player session.
- A valid finished run can show the latest timed lap versus the previous
  genuinely timed lap: negative signed delta = faster, positive =
  slower. With only one timed lap (including a one-lap time attack),
  the comparison is hidden. Once more than eight timed laps exist,
  the ring retains only eight most recent records without allocation.
- `RaceProgress::GetTimedLapCount()` counts strictly positive,
  finite lap durations, separate from ordered completed laps.
- `BuildFinishedRaceInsights(const RaceProgress&)` requires actual
  `RaceState::Finished`, positive float-representable clocks bounded by
  seven days, and valid best-lap data. It calculates the scorecard
  without modifying `RaceProgress`, records, SaveGame, career rewards
  or user data.
- `Tests/Standalone/FinishedRaceInsightsTests.cpp` uses actual
  gate/lap advancement and covers 3-lap best/spread/ideal, 1-lap
  no-spread, zero-clock checkpoints, restart, invalid duration and
  exact seven-day recordability.

## Required Unreal acceptance before claiming gameplay support

- Compile licensed UE 5.8 Editor/UHT + game targets and save/cook an
  authentic `/Game/Maps/LatebrakeCircuit.umap`.
- Complete a real one-lap time attack and three-lap race and verify
  figures against observed sector/lap clocks, including recovery
  time charges and restart.
- Inspect Results menu readability on a small Android landscape
  viewport and larger desktop sizes; verify all menu transitions.
- Ensure all-time records and money/medals are unaffected by this
  local result projection. Review actual completed race saves.
- Record source SHA, device model, engine logs and observed outcomes.
  Do not close hardware/engine QA issues based on portable CTest.

## v0.5.40 published source-only pre-release: finished does not always mean awardable

An ordered last gate still changes the race state to `Finished`, even
if a zero-duration lap is observed. For medals, saved records, credits and
full Results insights, however, **every completed lap** must have (1) a
strictly positive finite duration and (2) its **own successful active
race-clock advance**. A recovery fee alone does not satisfy condition
(2); charged fees remain included in reported lap and sector times.

The new `FinishTimingEligibility.h` shares this boundary between the
authoritative GameMode reward path and the HUD's finished-results
reporting. This does **not** claim real physical driving evidence: an
active clock advance means an eligible race tick occurred; it does not
prove wheel distance or valid checkpoint movement. Actual Unreal 5.8
physics, map and device acceptance testing are still needed.

Unrecordable finishes stay visibly invalid with no medal, new personal
best, race completion increment, earned credits or SaveGame update.
Source-level C++20 and Unreal callsite-binding tests cover this;
no compiled game has been verified.
