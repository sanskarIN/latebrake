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
