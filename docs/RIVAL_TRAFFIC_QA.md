# Rival player-traffic awareness — source-development QA

## What changes

The original one-car circuit rival uses an engine-independent route
planner and corner anticipator, but until now it could apply throttle
straight toward the player's occupied lane. The new conservative
`RivalTrafficAwareness` post-processes its existing motor command
using the **real possessed player's Unreal actor position and world
velocity**, compared with the rival's actual chassis heading and
observed speed.

- Only a player **ahead** of the rival in a narrow lane corridor
  (within 225 cm laterally) can trigger slowing.
- The rule only activates when the rival is moving and genuinely
  **closing** on the player, not merely when cars are parked nearby,
  in adjacent lanes or traveling the same direction at similar speeds.
- Between approximately 600 and 1,450 cm ahead the rival reduces
  its existing positive throttle, proportionately to distance.
- Under 600 cm of ahead distance, sufficiently strong closing motion
  requests ordinary braking. The rule does **not** deliberately
  swerve out of lane, reverse, accelerate to pass or choose a
  different waypoint.
- Existing corner braking, route arrival, checkpoint progress, racing
  time, rewards and career rules are never overwritten or promoted
  to official rival race placement. A missing/unready player or
  malformed physics sample blocks rival motor output.

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
