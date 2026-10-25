# Verified rival recovery outcomes — v0.5.49 source development

> **Source development, not an Unreal-tested opponent or playable release.**
> All existing v0.5.46 source-preview limitations remain. No licensed
> UE 5.8 Editor/UHT compile, cooked circuit or installed Win64/Android
> racing session was performed for this change.

## Problem and corrected contract

The autonomous rival formerly requested an emergency recovery after eight
seconds of low-speed observation. The route tracker incremented its bounded
recovery count and the Unreal caller invoked **`void RecoverTo(...)`**, which
discarded the result of `SetActorLocationAndRotation`. A rejected or
misplaced move was therefore counted indistinguishably from a physically
verified successful recovery.

The new behavior preserves historical **attempt** counting while giving
the outcome an auditable meaning:

1. After eight qualifying low-speed seconds the portable
   `RivalRouteProgress::Observe` creates one pending attempt, consumes one
   finite budget unit, resets the stall timer and returns `NeedsRecovery`.
2. While unresolved, subsequent observations cannot advance a waypoint,
   issue another recovery or increment approximate laps.
3. `ALatebrakeRivalCar::Tick` calls the real pawn's
   `TryRecoverTo` rather than its legacy `void RecoverTo`. This verifies
   post-move location, three-axis orientation, chassis collision and
   simulation, mass and finite/bounded linear and angular velocity using
   the same production recovery policy as the local player's paid recovery.
4. The rival calls `Route.ReportRecoveryResult(bRecovered)` exactly once,
   after placement. A verified outcome increases the **successful**
   counter; rejected/misplaced/unstable placement increases **failed**.
   The original attempt budget is never refunded.
5. After failure, the AI cancels the previous throttle/steer command and
   requests braking for this tick. A fresh low-speed interval can authorize
   another bounded attempt if chassis readiness still allows AI ticking.

Duplicate or unsolicited acknowledgement cannot credit a recovery, and
race reset/reconfiguration clears attempts, results and any pending flag.

**HUD semantics:** The non-compact circuit display calls the old count
`RECOVERY ATTEMPTS` and, after one or more attempts, shows
`RIVAL TELEPORTS VERIFIED X | REJECTED Y`. These are local approximate
rival recovery diagnostics, **not** lap victories, player penalties,
competition results or hardware QA certification.

## Portable regression coverage

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^(RivalRoute|RivalRecoveryOutcome|RecoveryTeleportTransaction)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

- `RivalRouteTests.cpp`: ordering, pending exclusion, exact-once
  acknowledgement, mixed success/failure 100,000-attempt saturation,
  fresh 8s retry policy, event/replay/reset state isolation.
- `RivalRecoveryOutcomeTests.cpp`: compiles and links the actual
  `RecoveryTeleportVerification.cpp` to the actual route policy. Tests
  genuine verified, displaced, corrupted-rotation, disabled-physics and
  unstable-angular-velocity snapshots.
- `RivalRecoveryFeedbackBindingTests.py`: checks actual Unreal
  `TryRecoverTo` -> `ReportRecoveryResult` -> control neutralization
  ordering; GameMode/HUD diagnostic forwarding. This only inspects source
  code and cannot prove Unreal runtime behavior.

## Hardware acceptance needed

1. Compile `ALatebrakeRivalCar`, `ALatebrakeCarPawn`, GameMode and HUD
   with installed licensed UE 5.8 UHT/Editor/game targets.
2. Author and cook the original circuit, package/install Win64 and Android
   candidates, and test normal rival laps without a recovery.
3. Arrange a safe **development-only** stall, observe exactly one pending
   attempt per interval, then a verified recovery with correct outcome and
   car movement. Validate source, engine, circuit and package fingerprints.
4. Inject controlled engine rejection, mismatched final location,
   yaw/pitch/roll discrepancy, disabled collision, nonfinite velocity and
   excessive residual motion **without corrupting production physics
   objects**. Confirm rejected counts, stopped controls and no false
   waypoint progression from an unresolved request.
5. Repeat attempt/failure/retry/reset/finish on real Android devices with
   the player driving simultaneously. Confirm no player race clock,
   checkpoint, medal, credits or saved personal best is affected by rival
   telemetry. Check UI text clipping and performance in both screen sizes.
6. File test evidence under
   [Android qualification #13](https://github.com/sanskarIN/latebrake/issues/13)
   and document any discrepancy before considering a real playable build.

This is a finite, synchronous in-engine recovery-result contract, **not**
proof that a rejected Unreal move leaves the vehicle in a safe spot.
A chassis that loses simulation entirely may require a separate supervised
reinitialization during licensed engine QA; the current rival driving gate
correctly blocks an invalid physics body rather than promising automatic
repair.
