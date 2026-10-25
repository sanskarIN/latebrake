# Restart-grid placement verification — v0.5.50 source development

> **Not a playable or installed-device qualification.** This feature has
> source-level regression coverage only. Genuine UE 5.8 Editor/UHT and game
> compilation, cooked circuit, Windows/Android packaged runtime and Chaos
> failure-injection QA remain outstanding.

## Why this gate exists

Before v0.5.50, `ALatebrakeRaceGameMode::RestartRace` reset the clock,
penalty history and checkpoints, then called the Blueprint-compatible
**void** pawn `RecoverTo`. That method intentionally ignores its
`TryRecoverTo` return value. A failed engine teleport could therefore
leave a healthy simulated car at its **old position** while the reset
race began timing. Physics readiness alone cannot prove it reached the
expected start grid.

The source now distinguishes an ordinary initial player spawn (which
must still pass the existing live physics readiness checks) from an
**explicit RESTART** requiring verified engine placement.

## Engine integration

- `Racing/RestartPlacementGuard.h` is an engine-free finite state
  machine: `InitialSpawn`, `AwaitingPlacement`,
  `RejectedPlacement`, `VerifiedPlacement`. Its initial state
  preserves the normal Unreal `APlayerStart` path.
- After career replay and save checks succeed, the GameMode calls
  `OnRestartRequested()` **before** race reset, engine unpause or
  physical relocation. Previously valid grid authorization expires.
- `ALatebrakeCarPawn::TryRecoverTo(GetStartPosition(), GetStartRotation())`
  applies the existing production teleport verifier to the actual
  position/rotation, post-move body and residual motion. Missing pawns
  and unverified moves report failure. Only the corresponding pending
  attempt may be acknowledged.
- `HasLocalPlayerDriver()` requires both a verified start-placement
  state and live chassis readiness. The existing authoritative clock,
  ordered-gate scoring, paid recovery and rival drive conditions use
  the shared local-player-readiness boundary.
- The player's **and rival's shared base** `ALatebrakeCarPawn::Tick`
  also passes the new placement bit to `CanApplyPlayerDrive`.
  On failure it neutralizes commands and motion before any thrust or
  yaw torque, including cases where Unreal remains unpaused.
- The HUD shows a distinct restart-grid warning instead of misleadingly
  reporting only an ordinary unavailable physics chassis.

A rejection is **latched**. Menu close, focus regain or a late/duplicate
callback cannot silently restore racing. Another explicit RESTART may
re-attempt the placement. The game does not apply a player recovery
time fee for resetting to the initial grid.

This safeguard **does not claim** that an unsuccessfully moved body
was automatically rolled back, is road-safe, or has been physically
recovered; it only refuses further scoring/driving authorization.

## Portable test procedures

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^(RestartPlacementGuard|PlayerDriveReadiness|RecoveryTeleportTransaction)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

`RestartPlacementGuardTests.cpp` links the **real** production
`RaceProgress.cpp` and verifies initial-spawn eligibility, pending/
rejected blocking, no phantom elapsed time, checkpoint denial, paid
recovery denial, repeated/unexpected outcome rejection and fresh
verified-retry eligibility. The portable drive matrix covers the new
seventh readiness bit independently. Python contracts assert that the
**actual** Unreal restart and pawn motor paths call the shared rules.
Passing these tests does **not** compile Unreal.

## Required live-engine acceptance

1. Compile `LatebrakeEditor` and the relevant game targets using a real
   authorized UE 5.8 workstation. Record engine build and source hashes.
2. Build a real cookable circuit, package a Win64 candidate, test a
   regular successful RESTART from a running race and after a finished
   result; confirm position, yaw, existing five-second paid RECOVER
   penalty logic, clock reset, and ordered-gate rebase.
3. With a **development-only** test double for engine movement, force a
   rejected `SetActorLocationAndRotation`, nonfinite/misaligned
   post-move pose and disabled post-move chassis. Confirm frozen clock,
   rejected gate awards, zero driving torque, blocked paid recoveries,
   visible warning and no credit/medal/save contamination.
4. From rejected state, close the menu, foreground the app and resume
   world updates; the latch must remain blocking. Trigger a fresh
   RESTART with a valid placement: check normal inputs and timing resume.
5. Repeat on installed Android with touch steering, accelerator, BRAKE,
   manual pause, orientation change and resumed application, including
   focus interruption while the restart placement is rejected.
6. Confirm the AI remains stopped while no local verified restart exists.
   Exercise successful and failed rival recovery telemetry separately.
7. Attach device/package SHA256, screenshots, logs and gameplay results
   to [Android QA issue #13](https://github.com/sanskarIN/latebrake/issues/13).
   Do not promote a stable/playable release until the full gate passes.
