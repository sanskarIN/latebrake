# LateBrake v0.5.9 — Fail-closed Race Clock Readiness

**Unreleased source development only; no playable game package.**

## Problem

The v0.5.8 source-side `CircuitStartupAudit` refused checkpoint
scoring if the original track failed collision, Actor creation,
safe spawn or four-gate checks. But GameMode's `Tick` still called
`Race.Advance` unconditionally. A player could see elapsed race
time and apparently ongoing results on an incomplete course with
no valid way to finish.

## Source changes

- `Source/Latebrake/Public/Racing/RaceClockReadiness.h` adds the
  allocation-free `AdvanceRaceClockIfReady`. Its inputs are the
  **actual** `RaceProgress` object, source course-readiness flag,
  paused status and frame duration.
- It advances only when ready, not paused, still racing and frame
  duration positive and finite. The policy reports success only if
  the original clock truly advanced, not when a floating-point
  increment rounded to zero.
- `Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp` uses the
  policy in `Tick` instead of unconditional `Race.Advance`.
- A shared `CanScoreCheckpoint` predicate prevents callbacks from
  advancing gates during world pause, after a finish or when the
  source circuit audit failed. Both scoring and the clock use it.
- `LatebrakeRaceGameMode.h` exposes the source readiness flag to
  `LatebrakeRaceHUD.cpp`, which shows `TRACK NOT READY` in
  prototype racing HUD/menu if startup failed.
- `Tests/Standalone/RaceClockReadinessTests.cpp` links the real
  portable race progression code and exercises startup failure,
  pause, bad frame duration, normal time, actual finish and reset.

## Genuine Unreal acceptance — still pending

Compile with authorized Unreal Engine 5.8 UHT/editor/game targets,
author/cook the real serialized track map, run installed Win64 and
Android builds, and observe the blocked startup warning on screen
after deliberately introducing a safe temporary course fault.
Confirm elapsed time stays at zero, progress/rewards remain
unavailable, and normal racing, pause, recovery, restart and saves
work after restoring the valid course.

A passed standalone CTest cannot establish game-world timer,
physics, Unreal HUD rendering, Android input or packaged EXE/APK QA.
