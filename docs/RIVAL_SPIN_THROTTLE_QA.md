# Physical rival spin-throttle guard (unreleased source development)

**Status: portable C++20 policy and Unreal C++ source binding only.**
No licensed Unreal Engine 5.8/UHT build, Chaos simulation,
cooked circuit, installed Windows game or Android device has qualified
this feature. This is not an ABS system, drift physics model or playable build.

## Goal and authority

A high-speed rival can keep accelerating while its real chassis is
rotating rapidly and moving sideways, making it harder for the waypoint
pilot to regain control. `RivalSpinThrottleGuard` reduces **only the final
AI motor demand** after route, corner, grip, surface/damage, player-traffic,
brake-hold and yaw-steering decisions have run.

The guard samples real world Chaos angular velocity Z, finite planar
chassis speed, measured lateral chassis speed and frame delta. Both
substantial yaw and substantial slip are required; this avoids treating
ordinary turns, low-speed maneuvers or sideways motion alone as a spin.

| Condition | Source-only motor response |
| --- | --- |
| Below 1,250 cm/s, or yaw below 1.05 rad/s, or slip below 22% | Existing throttle unchanged |
| Qualified speed, yaw at least 1.05 rad/s and lateral slip at least 22% | Cap existing throttle at 30% |
| Yaw at least 1.90 rad/s and lateral slip at least 38% | Cut throttle to zero |
| Above risk ends | Retain cap for at most 0.16 s of valid frames to suppress chatter |
| Invalid physics, nonfinite sample, bad frame time or no physics readiness | Reset guard and fail closed with neutral steering, zero throttle, brake |

These are **arcade policy thresholds**, not claimed real-vehicle
stability-control calibration. The guard never adds braking, rewrites
steering, uses a player input, performs a teleport, advances a checkpoint
or changes rewards/saves/results. Existing braking decisions pass through.

## Portable regression checks

Run on a source checkout:

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --parallel
ctest --test-dir Tests/Standalone/build -R RivalSpinThrottleGuard --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'RivalSpinThrottleBindingTests.py' -v
```

The C++20 suite includes 250,000 deterministic finite frames, yaw/slip
thresholds, a severe-spin cooldown, stable cruising, prior brakes, reset,
nonfinite inputs and malformed physics readiness. The source-binding suite
checks that the final stage is driven by genuine rival measurements and
that every interrupted race/recovery clears any old throttle hold.

## Required real Unreal/installed QA

1. Compile LatebrakeEditor and Latebrake Game with authorized UE 5.8/UHT
   on the **same source SHA** as the tests.
2. Cook a genuine owned circuit; confirm all four directional gates work.
3. Capture rival chassis velocity, lateral speed, yaw rate, final commanded
   throttle and steering while testing normal cornering, deliberate low
   speed maneuvers, collisions and high-speed spins.
4. Compare slide transitions at low/high frame rates; verify cap lifetime,
   no sticky throttle hold after pause/focus loss/recovery/restart, and
   that stopping motor power does not trap a rival indefinitely.
5. Retest following distance, damaged/off-pavement handling, brake heat,
   player completion and race results. Record actual package, engine,
   map, device, QA logs and observable failures.

Keep [playable qualification issue #148](https://github.com/sanskarIN/latebrake/issues/148)
open until this and the rest of the game are genuinely verified.
