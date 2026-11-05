# Four-corner Chaos suspension — real vehicle physics development candidate

**Status: source and portable regression candidate only. Not an Unreal Engine
5.8 compiled vehicle, finished physics model, installed Win64 game, licensed
Chaos Vehicles skeletal-wheel implementation or production release.**

## The specific problem being solved

The first LateBrake car was a 3D box-backed rigid body with **pitch and roll
locked**. Four live downward ground queries adjusted arcade traction, but
there was no physical spring/damper force at the corners, no supported body
heave/roll over road-height differences, and the four decorative cylinder
wheels did not follow real ground height.

This change is the start of **one production-quality car-driving foundation**:

- Both human and prototype AI still use the SAME simulated Chaos rigid
  body and original four WorldStatic line traces. The actor ignores itself,
  refuses malformed hits, and samples the **real velocity at each wheel
  anchor** using `GetPhysicsLinearVelocityAtPoint`.
- Each contact has an independently computed **spring and damper**.
  The spring pushes upward at the physical corner using
  `AddForceAtLocation`, allowing body pitch and roll. It does not
  teleport the body or set invented wheel contact positions.
- **Independent suspension dispatch:** after road, spring and axle
  samples pass the common physical validity gates, the four vertical
  forces are submitted before optional surface, heat, transmission,
  braking or HUD policies can reject that driving frame. During the
  pre-green countdown, a mutually exclusive grid path submits them
  once. This avoids dropping physical support when a later gameplay
  policy intentionally blocks motor/steering; it neither bypasses
  ground-contact validation nor adds extra tire force.
- All forces are scaled to **actual chassis mass and UWorld gravity**.
  At default 1,100 kg and 980 cm/s², four corners at the target 59 cm
  ride distance together counteract the current vehicle's weight.
- Travel is bounded: uncompressed spring reach 82 cm, target 59 cm,
  vertical normal threshold **Z >= 0.68**, no downward spring suction,
  no force from absent/too-far/steep surfaces. The maximum force at
  *each* corner is 85% of the mass's total weight, including heavy
  landings; no speculative 10g forces can escape the policy. This
  revised ceiling avoids a simulated bottom-out when falling from the
  existing 180 cm start grid in an idealized 1D vertical model; it is
  **not** evidence of collision clearance on a real Unreal map.
- The four cosmetic spindle meshes now move up/down **independently**
  with sampled vertical travel (between -23 and +25 cm), while their
  existing real-velocity roll and steer animation remains intact.
- The single authoritative `ChassisWheelAnchors.h` layout defines four
  centimeter offsets (FL/FR/RL/RR), with **122 cm front/rear and 94 cm
  left/right**, and a **-29 cm decorative spindle height**. The Unreal
  constructor, four actual `WorldStatic` spring rays, `AddForceAtLocation`
  locations and independently animated decorative wheel spindles use
  the SAME X/Y data. Physics traces begin in the actual chassis-root
  wheel-aligned plane and never follow cosmetic spring travel. The
  earlier inboard 68 cm ground sensor was inappropriate for physical
  load transfer. The previous three separate wheel-coordinate literals
  could drift apart as body kits change; tests now protect this contract.
  Because the chassis root mesh is nonuniformly scaled, cosmetic
  attachment transforms divide centimeter positions by root scale
  and preserve measured in-world positions.
- **Axle-local anti-roll support:** a front/rear pair whose wheels both
  carry positive measured spring load redistributes a small portion to
  the more-compressed (or descending) wheel. Left-to-right transfers
  conserve the total vertical force; each pair is independently limited
  to 8% of measured vehicle weight and cannot pull an airborne wheel
  toward the road. This is a **candidate** torsion-bar approximation,
  not independently modeled tire/suspension or verified Chaos behavior.
- **Axle-specific handling (rear-drive/front-steer prototype):**
  forward/reverse motor thrust, opposing-throttle engine braking, nitro
  and player slipstream boost require *positive measured load on a rear
  wheel*. The existing chassis torque controller requires loaded **front**
  wheels to steer. Front-only contact cannot power a fictional rear
  differential; rear-only contact cannot steer unloaded front tires.
  One-wheel axles receive at most 40% of the measured rear load or 45%
  of the front load, and both authorities remain below the same global
  contact-grip ceiling. At four equally loaded wheels the existing drive
  and yaw values are preserved. This is an honest first rear-wheel-drive
  approximation, not a real LSD, individual tire slip model or proven
  Chaos vehicle axle solver. Service braking/lateral traction retain the
  original **whole-car** limit for physical contact on any qualified tire.
- **Tire-grip load cap:** just seeing asphalt within spring reach is not
  enough. The existing 0/45/75/100% contact-count grip budget is capped
  by `sum(actual positive spring forces) / (mass × gravity)`. At a
  stationary flat ride height, existing handling stays unchanged; at
  full droop and during upward rebound, an unloaded spring provides no
  tyre force even with a nearby WorldStatic ray hit.
- The Unreal spring arm follows chassis **yaw** but explicitly ignores
  chassis pitch/roll, retaining the chosen chase-camera pitch and
  preventing body roll from tilting the player's horizon. This still
  requires visual-camera and clipping checks inside the actual engine.
- **3D wheel-geometry basis stays independent of the drivetrain:** real
  probe anchors rotate with the physical chassis pitch/roll, but all
  engine/brake/lateral/yaw-driving thrust uses a normalized **flat XY
  heading**. Tilting the car cannot inject unintended vertical engine
  acceleration. An almost vertical/invalid forward projection prevents
  motor and yaw corrections **after submitting genuine spring forces**,
  without zeroing Chaos linear/angular momentum.
- During the race's existing countdown, the actual body may settle
  vertically on real springs. Horizontal and yaw input are neutralized;
  no engine/nitro, tire wear, race clock or checkpoint grants are allowed
  until the existing authoritative green phase.
- This does NOT add UE5 Chaos Vehicles plugin, physical wheels, a proper
  tire friction curve, calibrated shock absorbers, aerodynamics, original
  skeletal car assets or professional animation. The original rigid
  box chassis still supplies the collision response.

## Safe rear-drive and front-steer reattachment (source candidate)

The initial four-corner spring controller already drops whole-car force
when actual measured support vanishes. The new
`Vehicles/SprungGripReattachment.h` prevents a single lucky road trace
on landing from immediately restoring the **full rear-drive motor/boost**
or **front-yaw steering** torque. It is wired into
`ALatebrakeCarPawn::Tick` after physical spring/axle observation and
before the first motor/steer force submission:

- **Lose grip immediately:** missing rear loads zero rear motor authority
  that same frame; missing front loads zero steering torque that same
  frame. Full airborne / unqualified frames cannot retain stale state.
- **Earn it back by real re-contact:** each axle's measured authority
  can increase by at most `6 × valid frame seconds` (0.10 at 60 FPS;
  full authority after at least about 0.17 seconds with continuous
  genuine support). Any *smaller* measured force ceiling applies
  immediately. A frame hitch greater than 0.10 seconds or invalid
  axle state resets the response.
- **Do not weaken emergency braking:** the filtered output preserves
  the raw, already-measured whole-chassis `ContactTractionScale`.
  Service braking, handbrake, and lateral grip keep their immediate
  physically limited support budget; ONLY rear-drive and front-yaw
  traction gain are rate limited.
- **Separate countdown clearing from a real interruption:** the pre-GO
  `Tick` calls `ClearDriverInputs(false)`, which still clears throttle,
  brakes, handbrake, nitro, transient drive policies and stale HUD
  readings but **does not reset earned axle adhesion every frame**.
  Regular `ClearDriverInputs()` after focus loss, controller loss, recovery
  and shutdown performs the full reset. This fixes a genuine integration
  problem where the first GO frame could otherwise have only 0.10
  steering/motor authority after three seconds of flat-road settling.
- **Reset on lifecycle boundaries:** race stats/reset, pause toggle,
  recovery, neutralized race/physics state and non-simulating physics
  frames discard saved authority. A fresh 3/2/1 grid countdown can
  settle both the physical springs and these motor/steer authority
  states without allowing pre-GO acceleration.
- **Validate physical metadata before reusing any axle memory:** the
  reattachment layer independently checks that four-corner contact counts
  agree with the front/rear breakdown, contact-count force ceilings,
  finite front/rear load ratios and each axle's actual load-adjusted
  2/1/0-contact traction cap. Malformed readings reset transient
  authority rather than granting motor or yaw torque.
- **One measured sample:** no extra wheel traces, actors, tire art,
  SaveGame writes, lap claims, network calls or telemetry side effects.

`Tests/Standalone/SprungGripReattachmentTests.cpp` reuses the genuine
production spring, ground-contact and axle load algorithms. It covers
30/60/120 FPS, snap loss of rear support, separate axle reattachment,
airborne no-grip, malformed/hitch frames, reset behavior, and **100,000
deterministic changing-contact frames** with source ceilings verified
each frame. Editor-source binding tests assert reset and force dispatch
order; these are not substitutes for real licensed Chaos/UHT validation.

**Manual acceptance still required:** on an authorized UE 5.8 runner
compile the exact PR commit, package an original map, test the same
vehicle on an installed Win64 candidate at 30/60/120 FPS, and observe
a real contact-loss/landing run with measured force, braking and
steering. Reject the feature if braking loses responsiveness or
re-contact feels delayed or unstable. Do not claim an excellent or
playable racing game from source tests alone.

## Canonical wheel layout and load-transfer verification

`Source/Latebrake/Public/Vehicles/ChassisWheelAnchors.h` is the
engine-independent single source for four clockwise axle-paired wheel
centers. `ChassisProbeOffset` returns the real chassis-frame ray origin
(with a fixed Z of zero); `ChassisVisualOffset` shares its exact X/Y
but adds measured, bounded visual-only travel to the -29 cm spindle.

The existing prototype's cylinder wheel radius is 29 cm: at target ride
distance 59 cm, its nominal bottom is 58 cm below the chassis origin,
leaving **1 cm intended visual clearance**. This is a geometric source
assertion, **not** a claim that the actual Unreal collision body's
wheel/body geometry passes hardware penetration QA.

`Tests/Standalone/ChassisWheelGeometryTests.cpp` verifies matched
4-corner order, nonuniform parent-scale compensation, visual travel
is strictly Z-only, actual yaw-rotated contact/visual XY alignment
at five headings, flat equal-force pitch/roll balance, and physically
asymmetric corner-bump torque from the **same production spring code**.
`Tests/Editor/UnrealWheelAnchorBindingTests.py` ensures the real pawn
uses the shared header for visible wheels, real road traces, force
anchors and wheel animation. The tests intentionally avoid any claim
of an installed playable game.

Before accepting this geometry in a playable build, test all four
corners on a flat map, offset/raised road edges, pitched and rolled
body, and every cosmetic garage car at 30/60/120 FPS; confirm force
anchors line up with rendered wheel centers and actual road height.
Reject the feature if wheel and probe movement visibly diverge.

## Numerical source tests

`Tests/Standalone/SprungChassisDynamicsTests.cpp` compiles the actual
header under strict C++20 warnings-as-errors and verifies:

1. Four-corner equilibrium at five masses (100–15,000 kg).
2. 490 cm/s² alternate real-gravity equilibrium.
3. One-corner bump, asymmetric lift and visual travel.
4. No pulling into road during rebound, bounded damping on impact.
5. Reject unsupported/wall/far samples and extreme frame delays.
6. Reject NaN, infinity, malformed trace distance, normal and gravity.
7. **120,000 independent frame fixtures**, verifying per-corner and total
   force budgets and numerical consistency.
8. Full-extension and upward-rebound observations never grant invented
   traction; unloaded springs cannot authorize nitro or steering.
9. Per-axle anti-roll transfer balances positive force on both loaded
   wheels, conserves vehicle lift, rejects airborne sides, remains
   bounded across all 120,000 varied frames, and never creates net
   downforce or breaks the other axle.
10. `Tests/Standalone/AxleTractionBudgetTests.cpp` checks rear-only,
    front-only, diagonal and single-wheel contact, loss of loaded
    suspension while ray-hit remains present, muted throttle/nitro and
    steering under axle support loss, partial weight transfer and
    **100,000** deterministic variable-mass/gravity/landing states.
    The portable test is not an installed Unreal driving benchmark.

`Tests/Standalone/SprungChassisRideHeightTests.cpp` adds a minimal
**one-dimensional vertical heave model** using the same production spring
function at fixed **30/60/120 FPS**, 800–1,800 kg, a gentle initial
height and the actual **180 cm grid drop**. It asserts equilibrium near
59 cm with no sustained bounce, spring impulse ceilings, and physical
box chassis bottom clearance in that *simplified* model. A separate
low-impact test uses three gravity magnitudes. This is a valuable
numerical regression, but does **not** simulate rigid-body pitch/roll,
Chaos physics substepping, WorldStatic collision, wheel normal loading
or road geometry. A genuinely packaged engine test remains mandatory.

`Tests/Standalone/SprungChassisAttitudeRecoveryTests.cpp` adds a
**separate approximate small-angle heave/pitch/roll reference** driven by
the same production four-corner spring and anti-roll output. It uses a
simplified 380 × 180 × 65 cm rectangular-solid inertia model rather than
the actual Chaos body tensor and semi-implicit 30/60/120 FPS integration.
Tests cover signed 4-degree pitch and 6-degree roll, 59/80 cm starting
heights, 800/1,100/1,800 kg chassis, and a temporary 10 cm front-left
road-height excursion. The checks require bounded forces, non-invented
net vertical lift, resisting initial moments, temporary dual-axis
response and eventual return near the reference ride height. This
**is not proof of tire contact, authentic real-world stability, engine
substepping, a real curb, collision clearance or playable driving**.

`Tests/Editor/UnrealSprungChassisBindingTests.py` inspects the actual
Unreal pawn for four WorldStatic traces, genuine corner velocity, world
gravity, validated spring output, calls to `AddForceAtLocation`, pre-GO
drive isolation and visual spindle travel. This is **source inspection,
not UHT, a physics trace execution or a real road-driving replay**.

## Observational evidence in a real non-shipping Unreal session

Once an **actual** Unreal Development/Editor build is available, the car
emits the original structured `LATEBRAKE_SUSPENSION v=1` log on the
**first valid physics frame**, on changes to the positively loaded
**FL/FR/RL/RR spring contact mask** or `grid`/`go` phase at most once
every **0.20 seconds per physical car**, and once every **2 seconds**
during stable contact as a heartbeat. A nearby road ray whose spring is
completely unloaded does not count as a bearing corner for this
event policy. The output fields stay identical, so the strict offline
v1 log parser needs no unverified schema migration.

This is deliberately rate-limited diagnostic sampling: a change that
reverses faster than 0.20 seconds may not be printed, and no log line
proves genuine physical tire/road contact or handling quality. It is
not a frame-by-frame replay, lap-time record or full production physics
telemetry system. The cadence is reset on pause/restart/recovery and
invalid physics frames, but a persistent vertical pose does not flood
the logs with 60 redundant first-frame observations.

These logs are compiled out of Shipping builds. They do not save player
data, award credits or run any extra physics traces.

Each real log line reports:

- `driver=player/autonomous`, `phase=grid/go`, and number of sprung
  contact corners;
- `weight_ratio` (measured summed spring lift / actual mass × gravity)
  and the load-limited `traction` fraction;
- `drive_rear` and `steer_front` — bounded physical force multipliers
  calculated from the rear/front qualified spring loads; `front_load`
  and `rear_load` are each normalized against half the actual vehicle
  weight. These are commanded force budgets, **not** measured tire slip
  or wheel torque;
- `arb_front_pct` and `arb_rear_pct` — signed, mass-normalized
  left/right anti-roll force transfers on front and rear axles (positive
  means additional support on the left wheel). These are *calculated
  commanded spring forces*, not measured physical tire loads;
- `body_z_cm` as **world height** (not ground clearance), real vertical
  chassis speed, physical pitch and roll;
- `fl_cm/fr_cm/rl_cm/rr_cm` — actual WorldStatic ray distances at each
  corner, or `-1` when a ray missed.

### Inspecting a real Development-session suspension log

After the authorized real Win64 test run has written its private Unreal
log **outside** the cooked game archive, the tester can run:

```powershell
python scripts/AnalyzeSuspensionTelemetry.py --log "C:\\LateBrakeQA\\my-session.log"
```

This read-only, offline parser accepts the explicit
`LATEBRAKE_SUSPENSION v=1` telemetry schema and prints a JSON summary.
It validates finite spring/load values, confirms rear-drive and front
steering never exceed the supported overall contact fraction, flags
unpowered rear-only / unsteerable front-only conditions, counts airborne
sampling, and highlights potentially unstable grid motion, large body
angles and unusually fast vertical contact events.

The output says `UNVERIFIED_OFFLINE_SUSPENSION_DIAGNOSTIC`,
`gameplay_verified=false`, `ready_for_release=false` **always**.
Diagnostics are sampled only once per two seconds, so this is not a
frame-by-frame physical replay and cannot measure FPS, verify real
collision, prove source authenticity or certify a complete race. Use
the existing hashed runtime receipt, candidate integrity checks and
observed video/hardware QA alongside it. The unit tests for this parser
use explicitly **synthetic** text lines and never claim actual gameplay.

The QA reviewer must compare multiple log lines to the real rendered
car, a captured video and collision debugger. A log line cannot prove
wheel penetration, tyre model quality, actual substep stability or
whether a player can complete a lap. Never report this as live evidence
unless it was emitted by an actual licensed Unreal Engine session.

## Nearly vertical chassis — physics recovery without fake drive

A fully pitched/standing-on-nose car can produce a near-zero world XY
forward projection while its **real Chaos body still has valid mass,
velocity, gravity and four contact probes**. The previous immediate
`Forward.IsNearlyZero()` return called
`NeutralizeMotionWhenDrivingBlocked()`, repeatedly zeroing BOTH linear and
angular velocity **before spring forces were sampled**. This could lock a
physical chassis in an unnatural suspended/vertical pose. The new Unreal
Tick path computes and, where valid, **submits the existing four spring
forces before checking the unusable planar heading**; if the heading is
still invalid it clears the motor/steer inputs, HUD road-contact authority
and temporary grip history but deliberately **does not zero Chaos velocity
or angular momentum**. Since these four spring forces were already accepted,
the visual-only wheel spindle travel remains bound to this latest valid
`SpringResponse` rather than snapping all four wheels to full droop.
The preserved cosmetic spring sample cannot authorize throttle, steering,
race scoring or invented contact.

This is not a magic righting or teleport mechanic: gravity and actual
blocking road contacts decide whether the chassis can fall naturally.
No synthetic ground hit, world-spawned righting torque, engine
boost or yaw force is authorized by this Tick branch. The separate
GameMode still owns the race clock and checkpoint gates; real hardware
QA must verify tipped vehicle motion cannot earn illegitimate gate or
finish results. In the earlier 3/2/1 grid branch, only the spring
dispatch executes, exactly once, as before.

A source-binding regression verifies actual Tick order and no motor
dispatch for invalid XY forward vectors. **On real installed Win64
hardware**, deliberately test a safely staged, near-vertical physical
chassis on valid road, observe that gravity restores ordinary
orientation without adding engine force, and capture camera,
wheel-pose, physics and checkpoint evidence. If the car remains stuck,
this is a failed gameplay quality gate; source tests alone cannot prove
real physical self-righting.

## Mandatory real engine acceptance gate

**Do not merge a candidate as production-ready solely because source CI
is green.** On an authorized Windows Unreal Engine 5.8 workstation:

- [ ] Generate project files; compile UHT, Editor and Win64 Game; investigate
      *all* errors from the new point-velocity and force API calls.
- [ ] Cook the project-owned circuit map and inspect four actual blocking
      WorldStatic road slabs. Capture the exact source revision and engine
      version in build logs.
- [ ] Drive at 30/60/120 FPS with physics substepping and inspect ride height,
      stability and oscillation over 10 minutes; measure actual frame-times.
      Capture `LATEBRAKE_SUSPENSION v=1` logs with all four measured
      ray distances, body pitch/roll, load ratio and contact count.
- [ ] Check stationary startup and 3/2/1/GO grid. The car should settle
      before the start with **no planar pre-start motion** or jump at GO.
- [ ] Test all four straight/corner approaches and raised/road-edge fixtures,
      recording distinct wheel travel, chassis pitch and roll, and true
      support loss. Confirm gravity/force scaling at default map gravity.
- [ ] Drive all six fictional garage cars, both handling presets, AI rival,
      pit stop, hard service braking, collision damage, reverse, spin,
      nitro, recovery, restart, pause and focus loss.
      Use a real road-edge/one-axle-support scene to check rear-drive
      throttle vs front-only contact, front yaw vs rear-only contact,
      and whether one-wheel recovery remains controllable.
- [ ] Look for wheel intrusion into asphalt or chassis, nonzero thrust
      while unsupported, oscillation at the rest-extension boundary,
      force spikes after hitches, flipped vehicles, steering-torque
      instability, camera clipping and ghost race-clock regressions.
- [ ] Compare before/after lap-time, brake-distance and lateral-slip
      telemetry using the exact same installed package and track fixture.
      Tune spring/damper/vehicle collision geometry **from measurements**.
- [ ] Record genuine screenshots/video, logs, package and source SHA256,
      exact machine GPU/driver/RAM, and known limitations.

## Performance and release scope

This candidate reuses four existing collision traces; no new high-volume
queries or frame allocations are introduced. True Chaos CPU cost, shader
cost, visual impact, platform compatibility and any driving improvement
**cannot** be claimed until the engine runs.

An eventual multi-GB release would contain original cooked 3D tracks,
licensed/generated car meshes, materials, textures, animation, sounds,
localization and optional replay data. **No padding or fake GB assets
should be added:** size is not a measure of gameplay quality.

This is source work on the official UE project; **the separate 200 KB
Win32/GDI 2D preview remains a distinct experiment**. The actual playable
release acceptance remains [GitHub issue #148](https://github.com/sanskarIN/latebrake/issues/148).
