# Physically bounded reverse overspeed — experimental source QA

**SOURCE-ONLY PR #295 feature, not a verified playable Win64 game.**
The published v0.5.93 source snapshot still used the historical
instantaneous reverse-speed limiter; the **current draft** replaces
the actual rigid-body Tick's velocity replacement with a finite,
measured-heading, grip/axle-limited Chaos force. This is not a full
gearbox, differential, tire/wheel simulation, ABS or certified
braking distance. The historical `ArcadeReverseSpeedLimit.h` reference
model is retained for old tests, **not called by the live pawn**.

## Why the change matters

Previously, if a wall collision or reverse throttle made the signed
vehicle-backwards motion exceed **48.6 km/h Arcade (1,350 cm/s)** or
**36 km/h Simulation Inspired (1,000 cm/s)**, the Tick immediately
replaced the measured XY velocity. That abrupt correction could
remove physical momentum at an unpredictable point in a frame and
cause a visible speed jump.

Now the `ReverseSpeedGovernor.h` source policy uses the actual Chaos
planar velocity and heading and returns a **world XY resistive force**.
The shared player/rival pawn submits it with `Chassis->AddForce`,
never `SetPhysicsLinearVelocity`. Lateral sliding and vertical fall
speed are not rewritten by this feature.

The reverse targets are **soft**. The governor reacts continuously
to overspeed, with up to **850 cm/s² Arcade / 650 cm/s² Simulation**
before real tire/garage/damage and measured rear-contact reductions,
without overshooting the reverse target due to its force alone.
A missing/airborne rear axle returns zero governor force. Negative
motor thrust is blocked at the cap, but ordinary reverse power is
preserved below it. Holding a service brake, handbrake or opposite
pedal blocks the extra governor force: no independent brake stacking.
Only otherwise valid real Chaos/race frames may submit this force.

The resulting deceleration also consumes the **same-frame lateral
friction-circle budget**, scaled by the actual whole-body road
contact only once. The motor engine-heat observer receives zero
throttle when the reverse motor is denied at its cap. All handling
input, race-clock, checkpoint, money, career and save authority
remain unchanged.

These are engineering target numbers, **not observed performance**.
Other concurrent Chaos collisions, slope, suspension, road drag,
service braking, dynamic tire response and substeps can change
actual measured speed or transiently exceed the reverse target.

## Portable verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel 4
ctest --test-dir Tests/Standalone/build -C Release -R '^ReverseSpeedGovernor$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*ReverseSpeedBindingTests.py' -v
```

The dedicated strict C++20 test covers 100,000 independent heading,
contact, grip, braking and drift samples, plus 30/60/120 FPS
reverse-to-cap force integration, near-cap continuity, forward motion,
lateral momentum, invalid inputs and no-contact conditions. **An additional
60,000 deterministic compositions** combine this governor's rear-wheel
force with the production `CalculateLateralGripForce` policy, preserving
the shared per-frame friction-circle budget over varying handling modes,
slip angles, chassis mass, suspension load and wet-road grip. These remain
portable source calculations; installed Chaos handling has not been tested. Python
source bindings check actual force dispatch, motor/heat gating,
the shared lateral budget and removal of the old runtime speed snap.
These only test source code, **not Unreal compilation or gameplay**.

## Real UE5.8 / Windows acceptance procedure

1. On a licensed UE5.8 Windows engine runner, compile the **exact
   draft PR head** with UHT/Editor, cook the original map, package
   a Development Win64 candidate and verify the executable/source
   manifest. Do not reuse an older pre-release ZIP as an EXE.
2. Drive in Arcade and Simulation modes on flat road and grass,
   at 30, 60 and 120 FPS. Approach the respective reverse targets
   with held reverse pedal. No frame should instantly rewrite
   horizontal/vertical motion; car should settle toward a soft cap.
3. Reproduce a backward collision overspeed with genuine replay
   or physical vehicle motion, without hand-editing velocities.
   Record actual longitudinal/backward speed, lateral skid, spring
   contact and frame time over the following seconds. Check that
   the recovery is continuous, never an instant XY teleport.
4. Test a reverse drift with large sideways speed: the force must
   point only along the car's planar forward axis, without
   artificially removing drift. Verify partial rear wheel loading,
   complete airborne separation and reattachment; no tire force
   without real contact.
5. During overspeed, apply a service brake or opposite pedal;
   confirm the governor does not stack a second brake, and that
   motor heat does not accumulate from blocked reverse thrust.
   Test engine cooling after recovery, steering, camera,
   checkpoints and official race-clock progression.
6. Analyze true non-shipping playtest logs using the original
   hash-bound Win64 session receipt. The existing
   `LATEBRAKE_DRIVE v=1` trace reports motor/braking/lateral/yaw
   **requested** forces but does not yet include either speed-governor
   force; do not infer measured total deceleration from it.
   Preserve video, FPS and installer/package identity separately.

**No real engine build, installed test or gameplay release is claimed.**
Keep the physical handling PR draft until this and [issue #148](
https://github.com/sanskarIN/latebrake/issues/148) are satisfied.
