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
