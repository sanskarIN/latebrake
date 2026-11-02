# Lateral tire grip and braking budget — actual Unreal source QA

**Status: new Unreal source integration; not an UE5.8-compiled, packaged,
installed or calibrated playable racing game.** The single chassis and
four attached cosmetic wheel cylinders remain a rigid-body prototype.

## Why this change matters in the actual race

The previously merged braking model imposes a plausible approximate
longitudinal deceleration limit. The lateral anti-slide model was still
applying `mass × sideways speed × 8.5/s` (Arcade baseline), with just
a frame-dependent reversal clamp. In a high-speed physical slide this
could request **far more than 1 g** sideways force even if the tires
were already using their available grip to brake. That is not
convincing racing-car behavior.

The *real* `ALatebrakeCarPawn::Tick` now uses
`latebrake::CalculateLateralGripForce` to apply an actual **Chaos
body-right force** opposite the measured chassis lateral velocity,
for both the player and the single autonomous rival.

- The requested correction begins with the existing handbrake drive
  profile's base lateral response, scaled **once** by garage grip,
  observed tire condition, confirmed off-pavement status and actual
  collision damage. Upgrades cannot accidentally multiply twice.
- The force is capped by provisional lateral acceleration allowances
  of **1250 cm/s² Arcade and 1050 cm/s² Simulation Inspired**,
  further scaled by qualified effective grip. Higher lateral velocity
  cannot authorize an arbitrarily larger contact force.
- Braking and lateral correction share a heuristic **combined grip
  envelope** (1550/1350 cm/s² per handling mode, before condition).
  The controller takes the **same-frame, already bounded real
  braking demand** from `GripLimitedBraking`, not an estimated
  pedal value. More committed braking can leave less lateral
  authority for maintaining a racing line.
- Human-only handbrake engagement reduces lateral grip to **42%**
  of the otherwise available budget. It maintains separate lower
  longitudinal handbrake deceleration without giving the rival a
  human-controlled handbrake.
- Force sign follows actual physical body-right movement (including
  both sides of a slide), and a single qualified frame cannot remove
  more than 80% of the measured lateral speed with this extra force.
  Invalid force inputs neutralize the driving tick before propulsion.
- This does not rewrite velocity, rotate the actor kinematically,
  reset race checkpoints, grant wins/credits, change saves or bypass
  the existing Chaos contact/physical-speed readiness checks.

**No real tires have been implemented.** This is a finite-grip arcade
approximation. It does not simulate individual tire contact patches,
suspension, normal loads, dynamic weight transfer, combined slip
angles at individual wheels, an ABS system, wheel torque, or
real-world manufacturer handling data.

## Validation in source CI

- `Tests/Standalone/LateralGripBudgetTests.cpp` runs under
  strict C++20 compilers on Windows, Linux and macOS. It validates
  mild and saturated slip, mirrored sideways direction, reverse-side
  correction, off-road grip, garage-tuned grip, Simulation handling,
  handbrake drift, simultaneous braking/cornering, fully spent grip,
  frame hitches, nonfinite physics and 20,000 varied samples.
- `Tests/Editor/UnrealLateralGripBindingTests.py` confirms the
  actual shared physical pawn consumes real velocity/mass plus
  previously qualified brake/heat/surface/tire/damage state, and
  that score/save and actor-teleport authority is unchanged.
- Existing lateral/handbrake/source-composition contract tests
  now **reject reintroduction** of the excessive linear sideways
  damping force.

## Real Win64 acceptance required before calling this playable

1. Use the manual authorized **UE 5.8/UHT compile and cook pipeline**;
   drive an installed candidate with actual chassis physics, not C++
   tests or the separate 2D experimental preview.
2. At **30, 60, 100, 150 km/h**, record transient steering/slalom
   behavior, measured lateral G, sustained turning radius and
   speed loss. Compare 30/60/120 FPS and uneven frame pacing.
3. Enter a deliberate handbrake slide and countersteer both
   directions. Confirm it doesn't instantly cancel slip or become
   uncontrollable; measure how quickly traction returns on release.
4. Brake heavily while turning on pavement, then after confirmed
   off-pavement entry. Confirm lateral authority diminishes without
   kinematic teleportation or force impulses.
5. Repeat with all six garage selections, both handling modes,
   fresh/worn tires, collision damage, pit replacement, nitro,
   rival AI cornering and engine heat, including reverse.
6. Verify no regression to race clock, directional gates, active
   rival completion, career medals, profiles or actual package logs.

Numbers are **starting hypotheses**, not measured or certified. They
must be tuned on a genuine installed game and replaced with proper
original vehicle skeletal meshes and Chaos wheel/suspension models
before a simulation-quality release. See [playable release gate
#148](https://github.com/sanskarIN/latebrake/issues/148).
