# Human Arcade physical-spin traction control — source QA

**Development only. The new policy has not been compiled under licensed
Unreal Engine 5.8, tested with actual Chaos dynamics, cooked into an owned
track or driven on any installed Windows/Android game. It is not a playable
release or production traction-control system.**

## What changed

The shared physical car already reduces positive motor demand when the
chassis moves sideways through a corner. That existing corner assist
does not measure whether the car is *actually rapidly spinning*. During
a high-speed spin, positive throttle and active nitro could continue
adding forward engine force.

A second, final engine-independent policy now evaluates three measured
Chaos signals **after** the original corner-throttle calculation:

1. Signed forward body speed (cm/s).
2. Sideways chassis movement relative to real forward velocity (cm/s).
3. Actual chassis angular yaw velocity (radians/second).

Only the **locally controlled human in Arcade handling** receives this
second safeguard. A handbrake held for an intentional drift, all
reverse/neutral demand, Simulation Inspired handling, and AI control
retain their preexisting behavior. The rival already has its own
independent spin throttle policy.

No steering, brakes, physics velocities, yaw torque, checkpoint timing,
race finish, SaveGame, garage credit or career reward is modified by
the new policy. The existing positive engine-force paths for standard
throttle, human nitro and slipstream all use the final reduced demand
rather than independently bypassing it.

## Example policy values

These are source-policy values, **not measured gameplay outcomes**.

| Measured state | Second traction multiplier |
| --- | ---: |
| Straightline or yaw <= 1.10 rad/s | 1.00 |
| Forward speed <= 1400 cm/s | 1.00 |
| Lateral-to-planar slip ratio <= 0.25 | 1.00 |
| Forward >= 2400 cm/s, slip ratio >= 0.50 and yaw >= 2.60 rad/s | 0.30 |
| Human handbrake held | 1.00 |
| Simulation Inspired or autonomous rival | 1.00 |

Between the above boundaries, three continuous linear blends smoothly
limit **only positive throttle**, down to a 0.30 multiplier in the
most severe classified spin. The prior lateral-slip corner reduction
also remains in effect. Invalid/nonfinite measured speed, yaw or input
produces zero throttle rather than potentially authorizing arbitrary
motor force. The policy never produces a larger throttle than the
original sanitized command.

## Source regression

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R HumanSpinTractionControl --output-on-failure
python -m unittest discover -s Tests/Editor -p HumanSpinTractionBindingTests.py -v
```

Strict Windows/MSVC, Linux/GCC and macOS/Clang CI cover forward
straightline driving, low-speed cornering, actual spin and directional
symmetry, normal cornering, preservation of human handbrake drift,
Simulation Inspired and AI bypass, reverse/neutral demand, numeric
thresholds, malformed physics and 250,000 deterministic sample frames.
The source-binding suite checks that real Chaos angular velocity
feeds the final positive throttle **before nitro and drafting
allocation**, and after the existing chassis-readiness gates.

## Required actual Unreal Engine 5.8 QA

1. Build the exact source commit in an authorized UE 5.8 workstation,
   capturing full UnrealHeaderTool, MSVC/UAT outputs and Cook receipts.
2. Drive an authentic Win64 Development package around the actual owned
   circuit, recording yaw, forward/sideways chassis velocity, analog
   throttle, nitro charge and real frame time on ordinary and low-FPS
   systems. Verify the car recovers from a fast spin without oscillatory
   throttle or unrealistically sudden slowdown.
3. Compare identical input runs in Arcade and Simulation Inspired with
   and without handbrake. Measure straightline acceleration and ordinary
   high-speed turns. The new guard should not slow them when physical
   spin thresholds are not met.
4. Verify genuine player/nitro/drafting additive force limits and
   confirm AI opponent route completion, timer, gates, pause/resume and
   restart/recovery behavior are unaffected.
5. Repeat relevant device tests under Android touch and real gamepad,
   including simultaneous left/right steering, throttle/braking,
   handbrake, focus loss and unstable network-free hardware behavior.

Source tests cannot prove real steering quality, stable 60 FPS, actual
Chaos contact/tires, playable EXEs, device performance or UI quality.
Keep [genuine playable release gate #148](
https://github.com/sanskarIN/latebrake/issues/148) open until
installed engine and game evidence passes.
