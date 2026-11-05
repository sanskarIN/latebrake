# Progressive road load — source-level physics integration

**Real Unreal chassis source changed, but NOT engine/runtime-qualified.**
This iteration targets the rigid-body Unreal Engine 5.8 car driven by
the player and the single AI rival, **not** the separate native 2D
Win32 experimental preview.

## Handling problem and change

Previously the prototype configured the simulated chassis with
`SetLinearDamping(0.5f)`. That is a **speed-proportional constant**
resistance applied by Chaos regardless of current road speed. In a
force-driven arcade prototype it can dominate propulsion and make
vehicle speed/coasting feel unconvincing. It is not an aerodynamic
model, tire friction or a complete realistic vehicle system.

This iteration lowers the constant damping to `0.08` and adds a
readable, speed-dependent, **planar force opposing actual chassis
velocity** during a qualified racing tick:

- A simple quadratic drag term grows rapidly near racing speeds,
  rather than a constant percentage drag rate throughout.
- A bounded smooth rolling-resistance ramp vanishes near rest; the
  car is not pulled backward by an invented constant friction force.
- Existing road-surface observation increases resistance 45% after
  **confirmed** off-pavement travel.
- Arcade and Simulation Inspired use different force coefficients.
  The real player and real rival share the same chassis implementation.
- The force is proportional to the actual measured mass, uses the
  actual world-space XY velocity (including reverse and side-slide)
  and never modifies Z velocity, gravity, wheel contact, recovery
  coordinates or race clock.
- Before any force application the policy validates frame duration,
  finite XY velocities, finite realistic mass and bounded motion.
  It limits its own deceleration to no more than 80% of the sampled
  planar speed in one frame. Failed samples neutralize the physics
  driver's current frame rather than requesting a nonfinite force.

This is **gameplay-tuned simplified drag**, not validated
aerodynamics, vehicle dynamics, tire contact, real-world braking
performance or an official speed benchmark. The existing reverse
limit and safety top-speed cap are deliberately retained.

## Contact-gated road friction — aerodynamic drag remains in flight

The original road-load force included low-speed **rolling resistance**
and extra grass/shoulder slowdown based only on the car's map position.
But a flying, rebounding, or inverted chassis can be directly above a
road while carrying **zero actual spring load**. Applying tire rolling
resistance in that case creates fictitious mid-air braking.

The real Unreal pawn now supplies the previously accepted
`SpringResponse.TotalUpForce / SpringResponse.WeightForce`, clamped to
[0,1], to `ProgressiveRoadLoadSample.SprungLoadFraction`. This is the
same genuine four-spring force frame used for the rear motor, front
steering, brake grip and wheel visuals. There are no new traces.

- **Fully loaded (1.0):** original paved/off-pavement tuning is
  unchanged. Existing standalone clients default to 1.0 only for
  backwards-compatible source fixtures; the real game supplies a value.
- **Partially loaded (0–1):** rolling resistance fades with actual
  positive spring load, and the off-pavement multiplier approaches 1.
- **Fully airborne, on unloaded rebound or upside down (0):** only the
  quadratic aerodynamic component remains, independent of map surface.
  No tire force is invented from visually nearby pavement.
- Nonfinite or out-of-range input fails closed; the previous strict
  frame, velocity, mass and 80%-of-motion one-frame reversal caps remain.

`AirborneRoadLoadTests.cpp` runs 120,000 changing spring-load,
motion-direction and frame-time samples. It explicitly checks no-load
roof and rebound, grass/pavement equivalence in flight, backwards and
sideways drag, exact full-contact baseline and 30/60/120 FPS force
equivalence. The Unreal source-binding test verifies the actual
physics-frame connection before any corresponding applied force.

**Actual acceptance still requires Unreal Engine 5.8 Win64 QA:** drive
a packaged car off ramps and over grass/pavement; compare horizontal
velocity on jumps, momentary positive spring load on landing, rolling
coast-down and post-contact reacceleration. Inspect 30/60/120 FPS,
actual Chaos damping and false road contact. Portable tests cannot
prove gameplay behavior, solver pacing, or collision correctness.

## Portable tests and source binding

`Tests/Standalone/ProgressiveRoadLoadTests.cpp` exercises zero speed,
coasting, three levels of forward drag, backwards movement, sideways
drift, diagonal force direction, off-pavement resistance, the two
handling profiles, near-zero continuity, long frames, nonfinite speed,
invalid mass and one-frame reversal limits.

`Tests/Editor/UnrealProgressiveRoadLoadBindingTests.py` verifies the
**actual** Unreal pawn consumes that policy after its real circuit /
Chaos readiness checks and before accelerating, that load is a
separate XY `AddForce` and that it cannot write results/rewards.
The C++ test is compiled with strict warnings on three OSes in CI.
Neither test runs Unreal Engine or measures real gameplay.

## Real playable acceptance gate

1. **Compile in genuine Unreal Engine 5.8**, package cooked Win64 and
   confirm the physics component applies the force every active frame.
2. On an installed build drive the six selectable cars in both
   handling modes. Measure **0–60 km/h**, coast-down from **100 km/h**,
   60→0 braking, and steady-state top speed in straight-line travel.
3. Compare against the previous driving build with identical
   hardware/physics timestep; tune drag/rolling constants to avoid
   stalling before intended car top speeds or making cars too fast.
4. Validate driving clockwise through all original four gates, with
   handbrake slides, reverse, nitro and rival AI. Check **no Z
   interference** on slopes, bumps or Chaos contacts.
5. Test off-pavement transitions and recovery; confirm no abnormal
   speed reversal, acceleration on coasting, crashes or NaN force.
6. Verify 30/60/120 FPS behavior and hitch/focus/pausing safety,
   log performance; test an installed Windows build, not source only.

The game still needs genuine driveable build evidence, polished art,
real wheel/suspension simulation, more tracks and professional UX.
See [playable release gate #148](https://github.com/sanskarIN/latebrake/issues/148).

## Smooth nitro release without a hard planar speed snap

The simplified Unreal car's old forward top-speed guard directly
called `SetPhysicsLinearVelocity` on the XY plane whenever planar speed
exceeded `MaxSpeed * CurrentNitroSpeedCapScale`. Even with the gearbox's
existing motor taper and a smoothly changing nitro multiplier, this could
instantaneously remove planar momentum at a single physics frame when the
boost ended. It is especially noticeable while drifting or landing and is
not a convincing force-driven car response.

The four-corner Chaos draft now uses `PlanarSpeedGovernor.h`:

- A strictly validated policy observes **actual** world XY chassis velocity,
  real chassis mass, qualified frame duration and the current active speed
  cap. It shares the same 15,000 cm/s input bound as `ProgressiveRoadLoad`.
- At or below the cap, it adds **zero** resistance. Above the cap, it
  requests an opposite planar **force** proportional to excess speed, capped
  at 650 cm/s² (Arcade) or 500 cm/s² (Simulation Inspired). Its own
  one-frame reduction cannot cross below the cap or reverse the car.
- The gearbox's existing gradual motor taper remains intact and all base,
  nitro and drafting drive still share it. Spring loads, brake and lateral
  tire force are untouched; the governor adds only horizontal resistance.
- The optional invalid-policy guard runs **before** driving forces. A valid
  earlier spring dispatch is never undone by a failed governor input.
- The separate **reverse-speed gear ceiling** still uses its existing
  instant reverse component clamp. This iteration replaces only the
  old global planar top-speed snap, not the entire speed-governance system.

`PlanarSpeedGovernorTests.cpp` numerically verifies neutral, under-cap,
gentle and large overspeed cases; yaw/sideways/reverse direction, bounded
deceleration, invalid inputs, changing nitro cap, and 120,000 synthetic
frames. It integrates the governor alone at 30/60/120 FPS to verify
monotone convergence without a speed snap. New source integration tests
check the actual player/rival Chaos Tick uses this output and does not
reintroduce the old top-speed XY overwrite.

**Limitations and real engine QA:** This is a gameplay-tuned arcade force,
not physically calibrated aero, downforce or a tire solver. Under several
simultaneous forces, it cannot promise a strict instantaneous velocity cap.
A real installed UE 5.8 Win64 run must compare 60–200 km/h acceleration,
steady-state terminal speed, nitro held/released at maximum speed,
simultaneous steering/drift and reverse, airborne landing and full-boost
hitch recovery at 30/60/120 FPS. Assess whether the car overshoots its
intended top speed, if drag feels too aggressive, and whether AI finish
times change. Only measured engine QA may authorize tuning or a playable
release.
