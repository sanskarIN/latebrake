# Grip-limited braking and genuine coasting QA

**Status: Unreal C++ source integrated; not UE5.8/UHT-compiled or
hands-on Win64 qualified.** This changes the *actual physical
player/AI Chaos chassis*, not the independent experimental 2D preview.

## Why this is a meaningful physics change

The previous simplified car used a speed-proportional braking rate
(`7.0 × chassis forward speed`) even during service braking. At
100 km/h, this could demand nearly 20g from the rigid body before
other forces. A second significant throttle-off damping rate also
hid the more recent realistic coasting/drag changes.

The shared Unreal pawn now:

- Derives **requested forward deceleration** from a bounded force
  policy, initially ~900 cm/s² in Arcade or 1050 cm/s² in Simulation
  Inspired (approximately 0.92g / 1.07g *before* grip/heat limits).
- Scales the brake force by **actual confirmed pavement state**, garage
  grip tuning, observed tire wear, existing collision damage and
  existing brake thermal fade. It does not invent tire contact data.
- Applies the physically signed force opposite **measured forward
  velocity**, including backing up, rather than assuming the driver
  always moves forwards.
- Keeps the intentionally lower-force human handbrake at about
  470 cm/s² and avoids simply summing handbrake and service brake
  forces when both are held.
- Caps the brake's own projected one-frame speed removal at 85% of
  the observed current forward velocity. It fails closed on invalid
  mass, frame timing, grip, heat or velocity.
- Stops using neutral throttle as a hidden extra brake. Neutral
  coasting is now left to qualified progressive road load and the
  already-reduced baseline chassis damping.
- Does **not** modify physics velocity directly, race gates, time,
  rivals' routes, rewards, profile saves, records, garage unlocks
  or original top-speed limits.

This is **a single rigid-body longitudinal approximation**, not
automotive ABS, disk/rotor or hydraulic simulation, real tires, wheel
lock, suspension, braking certification, or calibrated stopping
distance. The input numbers must be tested and adjusted with a
real Unreal physics/gameplay build before release.

## Opposing accelerator / reverse-pedal braking

The suspension draft also replaces the remaining uncapped direction-change
engine-brake branch (`3 × abs(forward speed)`, about 20g at 6500 cm/s).
The actual shared pawn uses `OpposingThrottleBraking.h` with candidate
650/750 cm/s² Arcade/Simulation Inspired ceilings, analog pedal magnitude,
surface/tire/garage/damage grip and measured rear-axle reattachment.
Service/handbrake takes precedence; no extra engine force stacks on it.
Front-only or absent rear support gives no drivetrain braking.

The rear-limited output force is applied once. Its whole-body-equivalent
deceleration is included in the same-frame lateral friction-circle request
before the existing contact scale. Neutral throttle coasts and the original
near-stop direction-change threshold remains. This is still simplified
rigid-body physics requiring real UE5.8 Win64 deceleration, cornering,
reverse, road-edge and airborne tests; see [suspension QA](SPRUNG_CHASSIS_QA.md).

## Automated validation

- `Tests/Standalone/GripLimitedBrakingTests.cpp`: cold/hot service
  brakes, off-road/tire/damage combined degradation, reverse travel,
  handbrake only / both brakes, low-speed and long-frame no-reversal,
  no-brake/coasting, finite/range validation and 10,000 signed/cadence
  checks.
- `Tests/Editor/UnrealGripLimitedBrakingBindingTests.py`: confirms
  actual pawn force application and retains physical safety bounds,
  independent handbrake input, original traction, and scoring isolation.
- Updated the pre-existing arcade force-composition integration
  assertion to enforce **new** bounded-brake and road-load pathways
  rather than the removed instantaneous rate-based service-brake
  behavior.
- `OpposingThrottleBrakingTests.cpp` exercises the actual spring/contact/
  axle/reattachment/lateral policies over 120,000 varying frames, signed
  braking, pedal strength, partial/absent rear support and malformed input.
  Source contracts bind the new brake force, shared motor classification
  and actual lateral budget to the Unreal pawn.

These are portable C++20 + source-binding tests. They DO NOT compile
or execute Unreal, and do not certify actual car feel.

## Genuine UE 5.8 Windows acceptance checklist

1. Use a licensed installed UE 5.8 to build/cook the actual Win64
   game, then use `Build-Windows-Playtest.cmd` and record a real
   installed physical gameplay session.
2. Measure 100→0, 60→0, and 160→0 brake distances; record vehicle
   mass, tire condition, damage, brake heat and handling mode.
   **Tune initial ~0.9–1.1g targets based on measured Chaos motion**.
3. Compare natural lift-off coasting with held service brake at 100
   km/h. The brake must slow the car more decisively but avoid an
   almost instantaneous stop or reverse-speed snap.
4. Test damage, grass/off-road, worn tires, brake fade after repeated
   laps, opposite-direction braking and deliberate handbrake drifts.
5. Confirm AI rivals also complete ordered checkpoints and brake at
   corners with no new deadlocks. Check combined previous steering,
   progressive drag and automatic gearbox at 30/60/120 FPS.
6. Check neutral throttle uphill/downhill and after collisions. The
   model operates on **measured longitudinal velocity**; Chaos
   contacts, longitudinal grip and slopes still need real calibration.
7. Confirm recovery, pause/resume, OS focus interruption, restart,
   pit tire replacement, career medals and persistent profile saves
   behave exactly as before.

Physical installed QA and wheel/contact modeling remain major
blockers under [playable release gate
#148](https://github.com/sanskarIN/latebrake/issues/148).

## Load-distributed road-point braking (unmerged physics draft)

`SprungContactBraking.h` now allocates the already contact-scaled bounded
service-brake request over the four accepted positive spring loads. The
shared player/rival pawn reuses each existing `WorldStatic` ray's actual
`ImpactPoint` and Chaos velocity at that point. It applies each accepted
planar share with `AddForceAtLocation`, allowing genuine rigid-body pitch
and asymmetric yaw moments. Spring forces remain submitted exactly once.
No additional collision trace or runtime allocation is needed.

A contact travelling against the chassis's forward motion receives no
COM-signed brake share: that would add local mechanical energy during a
spin. Slower contacts fade their share; rejected shares are never reassigned.
The handbrake alone uses only rear shares, so flat braking is weaker than
the previous whole-chassis approximation and an airborne rear axle cannot
brake through the front. Service plus handbrake retains one service request.
Lateral/yaw grip and drive telemetry use the accepted aggregate, with no
second contact multiplier. Reverse travel follows the same signed rules.

The portable regression verifies independent road-force pitch/yaw moments,
150,000 deterministic rigid-body point-velocity/load cases with nonpositive
local mechanical power, missing/corrupt contacts, reverse/handbrake behavior
and flat 30/60/120 FPS stopping against the analytic constant-deceleration
reference (within 0.6 cm in that isolated model). This is not measured Chaos
stopping distance, a coupled body solver or wheel-lock/ABS validation.

Before merge, compile actual UE5.8 Editor/UHT and Win64 Game and perform
installed 30/60/120 FPS braking on level road, grades, split support and
bumps. Inspect pitch, wheel re-contact, yaw stability, reverse stops and
handbrake-only rear contact, including an unloaded rear axle and both pedals.
Verify no false local power sign from rapidly rotating bodies, no excessive
pitch/roll contact oscillation, preserved pauses/restarts/countdown, and
accurate `service_force` logs. A planar brake on a banked/sloped surface
still approximates a tire force: this is not a road-tangent per-wheel tire
solver or calibrated production handling. Gate #148 remains outstanding.
