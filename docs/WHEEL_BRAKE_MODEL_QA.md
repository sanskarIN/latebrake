# Contact-phase wheel-brake model — source candidate, not qualified ABS

This candidate changes the shared Unreal player/rival pawn's service and
mechanical brake pathway. It remains unmerged and has not passed licensed
UE5.8/UHT, real Chaos driving, cooked Win64 or installed-device acceptance.
The published v0.5.97 tag remains an immutable earlier source snapshot.

## Implemented model and authority boundary

The existing road-tangent admission remains the hard brake-force ceiling.
Four independent modeled rotors use the actual measured road-point velocity,
accepted positive spring load and measured road normal. The SI adapter uses
cm/s divided by 100, kg*cm/s² divided by 100, and returns newtons multiplied
by 100. Normal load is the conservative spring projection `UpForce * Nz`,
not an invented wheel-contact sensor or a division that amplifies grip.

Each rotor solves a monotone brake-slip force law and static brake
complementarity with implicit integration. Root work is bounded to 36
iterations; frame integration uses at most 61 substeps of at most 1/600s.
Brake/contact work, numerical loss and modeled rotor energy are finite and
checked for passivity under the prescribed road-point speed. The actual
Chaos body is not integrated inside that solver; these energy checks are
not proof of the coupled in-engine rigid body's energy or stopping distance.

Individual service pressure first respects a torque feed-forward ceiling
from the admitted force, measured spring load, authored target slip and
modeled rotor inertia. Sudden grip/load loss lowers that ceiling immediately;
the minimum valve floor cannot override it. Modeled slip then releases
pressure further and rebuilds with hysteresis/hold timing. Mechanical handbrake bypasses the rear
slip controller, including when the service pedal is also held. Low-speed
holding and unsupported contacts do not fabricate ABS activity. New contact
phases initialize modeled rolling speed from body-point motion; coasting
and airborne wheel rotation are not independently simulated. Rotor RPM,
pressure, slip and work are modeled values, not measured hydraulic/wheel
sensors, certified ABS or a complete driven-wheel tire solver.

Candidate front/rear torque and inertia presets differ across the six
fictional garage cars while preserving the current 29cm wheel radius.
They are authored tuning values, not measured manufacturer specifications.
Service heat scales effective torque pressure; it does not silently change
the validated profile/inertia. The existing arcade brake-heat policy remains
a separate approximation rather than using modeled work as measured heat.

## Delivery, lifecycle and rendering

Proposals are computed before lateral/yaw allocation. Their accepted full
3D magnitude consumes that same grip budget. A proposal's rotor/pressure
state becomes visible only after the actual road-point AddForceAtLocation
commands are submitted and its unique receipt is committed. Stale,
double-settled, rejected or orphan receipts cannot create delivered work.
Reset clears model state without reusing receipt identities; pause/focus,
invalid physics, restart and recovery discard transient wheel estimates.
No reward, lap, checkpoint, profile save or race-clock authority is added.

Four visual wheel phases consume the committed modeled angular integral,
not the final instantaneous RPM. A modeled rear lock can therefore stop
one decorative wheel while others continue rolling. The correction replaces
only that frame's coasting increment and applies once per receipt. Rendering
never creates physical wheel contact or braking force; the cylinders and
spokes remain non-colliding decorative components.

## Diagnostics and source checks

The existing bounded development suspension cadence emits
`LATEBRAKE_WHEEL_BRAKES v=1`, with `source=modeled`, per-pawn actor ID,
committed receipt, actual road-point speeds, modeled rotor speeds/slip/
pressure, spring-projected normal-load estimates and commanded force in N.
The older drive record's service force remains the actual planar-forward
projection. Neither log measures the complete Chaos contact solver force.

The portable regressions cover 100,000 rotor energy/property samples,
12,000 varying four-corner frames, profile changes, thermal fade, reverse
symmetry, parking-brake precedence, stale/orphan receipt rejection and
independent wheel rendering. A prescribed flat-body stopping harness is
checked across all six garage profiles at 30/60/120 FPS. From 40m/s,
its flat prescribed-body stops are roughly 90.6–90.9m, varying by less than
0.02% between frame rates for each profile. These are synthetic model
results, not measured game stops. The harness keeps mass at 1100kg and
does not reproduce each car's complete in-engine configuration.
Parser fixtures are synthetic observations,
not engine evidence. Exact current CI logs establish the test counts.

Audit an original Development-build log with:

```bash
python scripts/AnalyzeWheelBrakeTelemetry.py path/to/Latebrake.log --require-player
```

The strict versioned parser checks finite SI channels, receipt order per
actor, pressure bounds, conservative tire-load bounds and nonpositive
commanded contact power. An active-mask bit means that corner has a modeled
brake channel; a zero bit does not prove loss of road contact. Actor IDs
identify simultaneous pawns within one run, so use a fresh log for each
run. Reported work covers only sampled command frames at the diagnostic
cadence. It is neither total race work nor a measured Chaos response.
The JSON deliberately retains `unreal_runtime_verified=false`; parsing a
log alone cannot qualify the game or publish a playable build.

## Real Unreal and installed acceptance

1. Compile UE5.8 Editor/UHT/Game for the exact reviewed source; fix actual
   engine/compiler failures before claiming runtime readiness.
2. In the Editor, compare `bUseWheelBrakeModel=true` and `false` on the same
   isolated car/map. The false value retains the old admitted road-tangent
   brakes and clears modeled rotor state. This is a lab A/B switch, not a
   new player-menu promise.
3. Drive each vehicle in Arcade and Simulation Inspired at 30/60/120 FPS.
   Record real distance/time and input/force telemetry on dry/low-grip road,
   grades/banks, crest landing, split support, reverse and spin recovery.
4. Verify separate front/rear modeled pressure, low-speed stop/hold, service
   heat fade, mechanical rear locking and both-pedal precedence. Check
   visual phase continuity, spoke axes, camera, steering and loss of contact.
5. Test the actual rival's corner anticipation, full ordered gates and races:
   rotor pressure can reduce braking below the old nominal budget. Existing
   AI planning is not newly certified by these mathematical tests.
6. Verify pause/focus loss, restart/recovery and garage profile changes;
   reject stale telemetry. Measure worst frame time with multiple cars,
   especially 100ms frames, and tune actual physics-substep integration.
7. Cook and install genuine Win64 packages and retain source/engine/map/
   package identities and original logs/video under gate #148. Source CI
   alone cannot permit physics merge or a playable-release claim.
