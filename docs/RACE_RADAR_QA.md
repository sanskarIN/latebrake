# Live race radar minimap — v0.5.62 source development

> **Source only, not playable/installed-device qualified.** The radar uses
> actual Unreal actor positions when available, but the UE 5.8 Editor/UHT
> build, cooked map, Win64 EXE, Android APK/AAB and device screenshots have
> not been produced or tested here.

## New racing feature

The earlier north-up HUD circuit map showed the next official gate and
player position but did not show the opponent or the player's heading.

`Track/RaceMiniMap.h/.cpp` adds a testable HUD-only snapshot containing:

- Player map point with actual Unreal yaw converted to a north-up screen
  heading vector (screen Y increases downward).
- Real next checkpoint position from the existing four-gate circuit
  layout and authoritative `RaceProgress::GetNextCheckpoint()`.
- Optional opponent map point from **the actual spawned rival actor**,
  not an estimated waypoint or lap index.
- Optional **direct 2D separation in meters**, clearly labeled direct,
  **not** rank, track separation, lead or projected lap time.

`ALatebrakeRaceGameMode::TryGetRivalRadarPosition` validates that the
rival actor exists, its physics body is race-ready and its 3D location is
finite. The HUD only shows rival markers during an eligible circuit
race with a real rival. Time Attack, absent or unhealthy rivals,
unverified player restarts, NaN/Infinity, and off-display actor
locations do not generate fabricated center/edge markers.

This complements the previously published
`CircuitMap::NormalizeCircuitMap` utility; the new snapshot explicitly
**rejects** off-display vehicles rather than treating clamping as a
valid live physical location. An authoritative but fixed gate can still
be shown when a car is temporarily unavailable.

## Gameplay integration

The existing full-size, noncompact HUD circuit map is now titled
`RACE RADAR / NORTH UP`. It uses cyan for player position and yaw,
orange for rival, and gold for the next scoring checkpoint. It prints
`RIVAL ...m DIRECT` only when both physical car positions pass the
validation boundary.

Compact mobile UI remains intentionally unchanged to avoid occluding
the existing touch steering and pedals. No scoring, steering, physics,
collision, career, persistence, multiplayer networking, or opponent
speed-balancing logic is modified by these display calculations.

## Regression commands

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^(RaceMiniMap|CircuitMap)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'RaceMiniMapBindingTests.py' -v
```

`RaceMiniMapTests.cpp` links the actual production implementation
and covers marker ordering, real 3m separation, four cardinal/wrapped
yaw directions, all official circuit gate conditions, absent/disabled
rival, invalid restart/player, extreme/off-display positions and
zero-distance overlaps. Unreal Python source contracts require the
real GameMode and HUD callsites. All existing strict suites still run
through the full GitHub Actions matrix.

## Engine/device acceptance still required

1. Compile actual UE 5.8 project targets, including UHT and the new
   production `RaceMiniMap.cpp` compilation unit.
2. Author and cook the genuine serialized original circuit map.
3. Start Circuit and Career events and verify orange opponent marker
   matches the actual moving rival on-screen and follows recovery.
4. Switch to Time Attack, destroy/unpossess/disable the rival, and
   ensure no ghost rival marker or incorrect direct separation appears.
5. Verify player heading on all four track directions and after
   player RECOVER/RESTART, pause, menu or orientation changes.
6. On desktop resolutions from narrow to ultrawide, inspect radar
   margins, legibility and visual overlap. On Android, ensure existing
   touch controls stay clear and no untested radar is shown.
7. Record actual engine revision, map/package hashes, captures and
   FPS in [qualification issue #13](https://github.com/sanskarIN/latebrake/issues/13).

**Do not publish a playable/stable claim on portable C++20 CI alone.**
