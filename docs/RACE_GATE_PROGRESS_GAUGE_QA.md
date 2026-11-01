# Race gate progress gauge — source QA

**Status: unreleased Unreal source candidate. NOT a playable release.**
This change targets the actual Unreal HUD, not the separate Win32/GDI experiment.

## Player-facing intent

The expanded desktop HUD's **RACE / INSTRUMENTS** panel gains a labeled
**GATES credited / total** progress bar. A three-lap four-gate event starts at
**0/12**, reaches **4/12** after its first accepted lap, and can only reach
**12/12** when the authoritative race is finished. The progress readout is
based on `RaceProgress::GetCompletedLaps()`, `GetNextCheckpoint()`,
`GetRequiredLaps()`, `GetCheckpointCount()` and `IsFinished()`.
Each checkpoint is an equal discrete visual step: the bar does **not**
represent route distance, real-world speed, lap quality, opponent ranking,
sector timing, rewards or predicted time to finish.

The panel retains labeled nitro, engine-heat and chassis-condition meters;
its desktop rectangle grows vertically to reserve the extra meter. It remains
hidden for compact windows, menus, Android touch, and finished Results.
Gameplay, checkpoint acceptance, vehicle physics, save files and network
behavior are unchanged.

## Safety and test cases

- `RaceGateProgressGaugeTests.cpp` checks start, wraparound, finish,
  every discrete step across 1–8 laps and 1–32 gates, invalid negative,
  out-of-range, inconsistent and integer-overflow-sized snapshots.
- `HudInstrumentClusterTests.cpp` still sweeps desktop viewports for
  right-edge, footer and left text-lane safety at the expanded panel size.
- `RaceGateProgressGaugeBindingTests.py` ensures the real HUD uses the
  authoritative race snapshot and keeps drawing code separate from awards,
  checkpoint grants, vehicle physics and saving.
- No test claims an actual Unreal 5.8 build, an installed game, screenshots,
  usable high-DPI colors, or an Android landscape QA result.

## Real-device review still required

On a licensed Unreal 5.8 Windows workstation, compile with UHT, run the
original circuit and time attack, and observe the gauge at each accepted
gate, at lap transitions, after pause/restart, and on finish. Check UI
separation at 1280×720, 1920×1080, and ultrawide with DPI scaling.
Issue [#236](https://github.com/sanskarIN/latebrake/issues/236) remains open
until the production HUD and menus have real installed QA.
