# Verified player recovery teleport — v0.5.46 source development

> This document describes source validation. Genuine Unreal Engine 5.8
> compilation and installed Win64/Android physics QA have **not** happened.

## Previously unsafe recovery

The player RECOVER handler previously charged its five-second time penalty
and incremented `RecoveriesThisRace` **before** calling Unreal
`SetActorLocationAndRotation`. The existing `void RecoverTo` discarded
Unreal's boolean return value entirely. If placement was rejected (or the
actor ended elsewhere), the player could lose time without being recovered.

## Implemented code

- `Racing/RecoveryTeleportVerification.h/.cpp`: pure production C++20
  classification of `EngineRejected`, `NonfinitePose`, `PositionMismatch`,
  `HeadingMismatch`, `Verified`. An accepted move must land within 5 cm
  of the intended **3D** position and within 2° of the target yaw (with
  equivalent 360°-wrapped headings accepted).
- `ALatebrakeCarPawn::TryRecoverTo`: validates finite target coordinates
  and heading, invalidates checkpoint interpolation by incrementing
  `TeleportRevision` **before** the move attempt, zeroes input and live
  chassis motion, calls Unreal with `ETeleportType::TeleportPhysics`, then
  observes the resulting position and rotation and returns true only for a
  verified placement. The existing Blueprint-facing `void RecoverTo`
  delegates to it; restart and rival code retain their API.
- `RecoveryCharge.h`: `TryChargePlayerRecoveryAfterVerifiedTeleport`
  computes exactly the five-second penalty on a local copy of the
  fixed-capacity `RaceProgress` object and recovery count. The actual
  clock, penalty audit and count are updated **only after** the verified
  placement callback reports success. All existing readiness, pause,
  finish, counter-overflow and precision protections still apply.
- `ALatebrakeRaceGameMode::RecoverPlayer`: now performs the staged
  transaction, rather than directly charging and blindly moving.

**Checkpoint integrity:** The pawn advances its teleport generation before
attempting movement, including on failed engine requests. Therefore a
partially moved/unverified pawn cannot bridge pre-recovery checkpoint
positions into a new ordered gate award.

**Limitations:** The transaction executes synchronously on Unreal's game
thread. If a future callback modifies the race clock concurrently, the
assumption would need revisiting. Placement verification reports observed
coordinates; it is not proof of safe road collision, an unblocked spawn,
correct future physics contact or a complete device qualification.

## Source verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^RecoveryTeleportTransaction$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

The focused CTest compiles the actual production verification `.cpp` and
`RaceProgress.cpp`; tests engine-denied movement, mismatched Z and lateral
position, misaligned and wrapped yaw, nonfinite fields, readiness denial
without invoking the callback, unrepresentable penalty, exhausted counter,
race finish, unsuccessful no-charge retry, exact post-success penalty and
post-teleport checkpoint re-baselining. Python tests inspect actual Unreal
source bindings and order, **not** a live UE compiler.

## Required UE 5.8 acceptance

1. Compile with licensed UE 5.8 Editor/UHT and game targets. Confirm
   `RecoveryTeleportVerification.cpp`, pawn and GameMode compile.
2. Install a candidate with an authentic cooked circuit and verify RECOVER
   from a healthy car; observe new coordinates, heading, +5s and +1 count.
3. Using a **development-only test harness**, simulate a failed
   `SetActorLocationAndRotation` return or mismatched observed transform
   without damaging real physics objects. Verify that no time, recovery
   counter, penalty audit or medal benefit is changed.
4. Confirm normal restart and rival recovery still work, the pawn's
   `TeleportRevision` prevents false gate scoring and the next genuine
   forward plane crossing still scores.
5. Test touchscreen RECOVER, button repeat, app background/foreground and
   paid recovery from different ordered gates on physical Android hardware.
   Log source/engine/map/package hashes and tester results; record in
   [issue #13](https://github.com/sanskarIN/latebrake/issues/13).

No stable/playable release can be claimed from source tests alone.
