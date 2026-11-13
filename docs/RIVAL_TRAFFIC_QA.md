# Rival player-traffic awareness — source-development QA

## What changes

The original one-car circuit rival uses an engine-independent route
planner and corner anticipator, but until now it could apply throttle
straight toward the player's occupied lane. The new conservative
`RivalTrafficAwareness` post-processes its existing motor command
using the **real possessed player's Unreal actor position and world
velocity**, compared with the rival's actual chassis heading and
observed speed.

- Only a player **ahead** of the rival who is physically inside the
  narrow 225cm lane corridor, or is measured converging into that corridor
  before a near-term rear catch, can trigger courtesy slowing.
- The rule only activates when the rival is moving and genuinely
  **closing** on the player, not merely when cars are parked nearby,
  in adjacent lanes or traveling the same direction at similar speeds.
- The base 600–1,450 cm throttle/reaction policy is retained, but
  **qualified measured braking capability** can extend anticipation
  smoothly to a maximum of 6,500 cm for rapidly closing cars.
- A nonzero estimated stopping shortfall now requests early normal
  braking even before the old fixed 1,450 cm cutoff. Under 600 cm,
  strong closing motion still requests braking. The rule does **not** deliberately
  swerve out of lane, reverse, accelerate to pass or choose a
  different waypoint.
- Existing corner braking, route arrival, checkpoint progress, racing
  time, rewards and career rules are never overwritten or promoted
  to official rival race placement. A missing/unready player or
  malformed physics sample blocks rival motor output.

## Anticipate measured sideways lane entry (source follow-up)

The previous 225cm corridor looked only at the physical player's
**current** lateral offset from the single real AI rival. A player
moving sideways into the lane could be farther than 225cm away **now**
but occupy the rival's path by the time a high-speed rear catch occurs.

The safety post-processor now projects the **relative lateral world
velocity of both chassis** onto the rival's measured heading. For a
forward-closing pair (at least 2.3 m/s of closing velocity), it examines
the approximate time until reaching the existing 3.8m rear-clearance
margin, bounded to **0.10–1.25 seconds**. A currently adjacent player
is treated as a possible lane conflict only if the projected lateral
gap comes **inside 225cm and closer to lane center**. Otherwise an
adjacent player moving **away** from the lane or too far in the future
still does not trigger the courtesy brake.

An anticipated lane conflict then goes through the **existing**
grip/brake-capability stopping envelope and emergency decision. It
cannot invent motor power, add steering corrections, override an
authoritative corner brake, edit a player command, or award a checkpoint.
Existing single-rival recovery, local actor readiness, physical
velocity validity and traffic-brake hold remain authoritative.

The test suite covers converging and diverging lanes, opposite-side
mirroring, northbound world-axis rotation, weak/healthy brakes,
sub-horizon and beyond-horizon conditions, and **12,000 adversarial
velocity/gap pairs**. This is a conservative **constant-velocity,
near-term source-only approximation**, *not* a 3D multi-car collision
prediction, safe overtaking proof, or certified collision avoidance.

**Installed Unreal acceptance still required:** test a real player
cutting inward and outward from both adjacent lanes at 30/60/120 FPS,
including diagonal motion, road barriers, recovery, low-grip driving,
corner turns and same-speed false-positive cases. Capture actual
contact traces, vehicle speed, projected lateral gaps and footage.
Do not claim no-collision performance from this portable policy.

## Live measured-brake stopping envelope (subsequent source fix)

The fixed 14.5-metre courtesy awareness distance did not leave enough
room when the physical rival was closing much faster than the player.
The rival now forwards the **already qualified** physical brake
deceleration from `RivalCornerBrakeResult` into its existing
`RivalTrafficAwareness` policy. That forecast derives from previously
measured tire grip, road surface, collision damage, brake heat and
arcade/simulation brake mode. Invalid, missing, negative or nonfinite
braking evidence produces neutral steering and active braking instead
of inventing dry-road grip.

The new conservative relative-speed stopping approximation uses
`closing_speed * 0.50 s + closing_speed² / (2 * measured_braking)`,
plus the existing **380 cm** clearance margin. Gentle throttle
awareness scales with that distance, capped at **6,500 cm**; the old
1,450 cm band remains its minimum. A genuine emergency stopping
shortfall **overrides the 6,500 cm awareness cap** rather than
permitting acceleration when faded brakes require greater runway. Within the observed same-lane
corridor, near bumper overlap can request braking even if both cars have
similar speed. Pre-existing emergency time-to-contact and brake-hold
hysteresis remain active. The driver, route, waypoint, steering, race
clock and savegame are unchanged.

Portable tests include low-brake-grip versus healthy braking on the
same actual speed/gap, 50-metre high closing cases, missing brake
telemetry, alternate lanes, matched speeds, and a deterministic
16,000+ sample parameter sweep. A low-grip car must never be permitted
to accelerate at an observed gap that already required braking when
the identical car had stronger brakes.

This is a **source-only safety estimate**, not a validated
collision-avoidance system. It assumes linear same-lane approach and
does not predict steering cuts, another car suddenly changing lanes,
current ABS slip or future Chaos contact. The Unreal 5.8 self-hosted
physics validation remains required before approving the richer
six-car integration, a playable release or safety claims.

## Data origin and ordering

`ALatebrakeRivalCar::Tick` first uses the existing authoritative
green-light, map, player and rival physics readiness gates. It then
plans its actual current waypoint through `PlanRivalTurn` and
`AnticipateRivalCorner`. After those decisions, it reads the real
`UGameplayStatics::GetPlayerPawn`, checks race-physics readiness and
samples actor location and `GetVelocity()`. Only then does it call
the new pure C++20 traffic-aware limiter and forward the resulting
`PilotCommand` to the base `ALatebrakeCarPawn` physics controller.

The limiter uses observed world XY positions in Unreal centimetres.
It projects player displacement and velocity into the rival's actual
forward/right axes. It is a simple lane-based courtesy rule, not a
complete racing traffic planner or a substitute for a working physics
collision system. Sharp bends, intersecting tracks, occlusion, and
unusual geometry may cause false yielding and require actual Unreal
track playtesting.

## Files and verification

- `Source/Latebrake/Public/AI/RivalTrafficAwareness.h`
- `Source/Latebrake/Private/AI/LatebrakeRivalCar.cpp`
- `Tests/Standalone/RivalTrafficAwarenessTests.cpp`
- `Tests/Editor/RivalTrafficBindingTests.py`
- `Tests/Standalone/CMakeLists.txt`

Portable regression suite:

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^RivalTrafficAwareness$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

Test same-lane emergency brake, gentle approach, equal-speed traffic,
player behind/sideways, slow rival, world-Y heading, existing
corner/arrival decisions, NaN/Inf/invalid velocities, player missing
and bounded behavior across many closing distances.

## Mandatory real gameplay qualification

Test a real Unreal Engine 5.8 built and cooked circuit in Win64,
including all four corners, player stopped on racing line, player
overtaking, closely matched pace, reversing, side-by-side race, a
teleporting player, collision, pause/focus and race restart.
Confirm rival recovery/route progression remains correct and no
false braking traps the rival permanently. Check actual logs and
HUD/debug data and frame timings. Re-run on representative Android
hardware if rival spawning is enabled.

**This is unverified source-only AI behavior**, not proof of safe
collision avoidance, fully competitive rival placement, installed
game packages or authoritative race results.

## Predictive time-to-contact and brief brake hold (next source candidate)

The rival's straight, same-lane player-following policy now computes a
bounded **time-to-contact estimate** from the observed relative forward
velocity and distance, subtracting a conservative 380 cm separation margin.
Both cars' true WORLD velocity vectors are projected onto rival heading:
the rival's planar speed magnitude is cross-checked but **not mistaken
for forward closing speed during a lateral drift or reverse movement**.
Malformed/inconsistent velocity samples fail closed to braking.
For a closing speed of at least 500 cm/s, an estimated contact time
**at or under 0.75 seconds** requests normal braking before the older
600 cm-only emergency threshold. The existing slower/near emergency,
lane width, player availability, heading, finite velocity and waypoint
guards still apply. The projection is intentionally narrow: it is not
a full collision predictor, autonomous overtake or racing classification.

A separate engine-neutral, fixed-memory brake hold retains an actual new
traffic-brake decision for at most **0.18 seconds** after the first safe
frame, to avoid rapid motor/brake oscillation at the threshold. The hold
never overrides rival waypoint arrival or steering, never adds throttle,
and resets on event restart/configure, race readiness loss, route finish
or attempted recovery. Invalid/oversized frame time and malformed input
fail closed to zero motor and active braking.

**Portable verification:** strict cross-platform C++20 tests cover
relative-speed/distance/lane cases, threshold prediction, preservation
of corner steering, brief hold and release, repeated emergencies, malformed
input/time, route completion and event interruption. Python source contracts
check real player Chaos position/velocity wiring and safe application
before the pawn's physics Tick.

**Real installed-game acceptance remains missing:** compile with licensed
UE 5.8; cook and launch the original circuit on Win64 and Android; capture
the rival approaching a slower player at multiple actual speeds, straight
lanes, corners and diagonally crossing trajectories. Verify no oscillatory
braking, unreasonable prolonged brake holds, rear impacts or unexpected
steering/route changes, while actual gameplay runs at 30/60/120 FPS and
during a long hitch. Exercise menu/focus pause, restart, recovery and
destroyed/replaced player pawn. Measure behavior with real logs and device
video. No source assertion can prove collision-free driving or a game build.
