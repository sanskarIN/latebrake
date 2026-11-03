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
- Spring anchors use the **same front/rear 122 cm and lateral 94 cm**
  offsets as the visual wheels. The earlier inboard 68 cm sensor was
  inappropriate for physical load transfer; contact forces now act at the
  actual spindle track width.
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
  acceleration. An almost vertical/invalid forward projection fails closed.
- During the race's existing countdown, the actual body may settle
  vertically on real springs. Horizontal and yaw input are neutralized;
  no engine/nitro, tire wear, race clock or checkpoint grants are allowed
  until the existing authoritative green phase.
- This does NOT add UE5 Chaos Vehicles plugin, physical wheels, a proper
  tire friction curve, calibrated shock absorbers, aerodynamics, original
  skeletal car assets or professional animation. The original rigid
  box chassis still supplies the collision response.

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

`Tests/Editor/UnrealSprungChassisBindingTests.py` inspects the actual
Unreal pawn for four WorldStatic traces, genuine corner velocity, world
gravity, validated spring output, calls to `AddForceAtLocation`, pre-GO
drive isolation and visual spindle travel. This is **source inspection,
not UHT, a physics trace execution or a real road-driving replay**.

## Observational evidence in a real non-shipping Unreal session

Once an **actual** Unreal Development/Editor build is available, the car
emits the structured `LATEBRAKE_SUSPENSION v=1` log at most once every
**two seconds per live physical car**. These logs are intentionally
compiled out of Shipping builds. They do not save player data, award
credits or run any extra physics traces.

Each real log line reports:

- `driver=player/autonomous`, `phase=grid/go`, and number of sprung
  contact corners;
- `weight_ratio` (measured summed spring lift / actual mass × gravity)
  and the load-limited `traction` fraction;
- `body_z_cm` as **world height** (not ground clearance), real vertical
  chassis speed, physical pitch and roll;
- `fl_cm/fr_cm/rl_cm/rr_cm` — actual WorldStatic ray distances at each
  corner, or `-1` when a ray missed.

The QA reviewer must compare multiple log lines to the real rendered
car, a captured video and collision debugger. A log line cannot prove
wheel penetration, tyre model quality, actual substep stability or
whether a player can complete a lap. Never report this as live evidence
unless it was emitted by an actual licensed Unreal Engine session.

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
