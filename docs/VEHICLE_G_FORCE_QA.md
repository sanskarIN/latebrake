# Vehicle G-force telemetry — source candidate, not a playable release

## Player feature

LateBrake can now estimate **signed longitudinal and lateral chassis G**
from consecutive real world-space Chaos velocity samples. This creates
useful racing feedback without adding driver assists or changing vehicle
handling. A positive longitudinal reading means forward acceleration,
negative means deceleration; positive/negative lateral readings indicate
acceleration toward the car's right/left axis.

On sufficiently spacious desktop HUDs, the optional `CHASSIS G (EST)`
line shows smoothed signed longitudinal and lateral G. After a valid
finish the paged Results menu can display the run's **peak braking G**
and **peak cornering G**. Both are advisory *session-only* estimates.
The game does not turn them into race rewards, records, rankings,
garage credits, saved progress or driving instructions. Very small
screens prioritize authentic speed, clock, gates and driving controls.

### Physics and continuity policy

- The player pawn must pass the **existing** real start, circuit,
  restart placement, physics readiness and focus/menu state gates
  before taking samples; a stopped red-light frame is not telemetry.
- Observations use the actual chassis **world XY linear velocity** in
  cm/s. Differentiated velocity is projected onto the vehicle's
  observed yaw for forward/right acceleration. Local speed differences
  alone would miss real constant-speed cornering acceleration.
- Unreal `Tick` samples preceding frame-to-frame velocities. A first
  valid sample after startup or any interruption **arms a baseline**
  but earns no acceleration result.
- Samples outside 1–250 ms, with nonfinite/malformed velocity or yaw,
  with speed above 360 km/h, or with resultant acceleration above 6g
  are rejected; continuity and live displays clear. A collision or
  teleport spike must not become the session's best cornering reading.
- Live G readings use bounded first-order smoothing (~120 ms). Peak
  statistics are from accepted raw derivatives rather than smoothed
  HUD motion; they are **not** independently calibrated to sensors.
- A peak braking value requires an observed negative forward-axis
  acceleration **while actual BRAKE input is held**, and while moving
  forward at least 2.8 m/s. Passive engine drag and reversing do not
  count. Cornering peaks require at least 5 m/s of planar speed.
- Pause, OS focus, blocked physics and paid recovery interrupt samples;
  an accepted race restart resets all readings and peaks. A recovery
  does not erase earlier legitimate peaks from that run.

## Source and tests

- `Source/Latebrake/Public/Racing/VehicleGForceMeter.h` — portable
  bounded C++20 differentiator, filtered readouts and session peaks.
- `Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp` and its
  header — sample the actual player chassis only on valid physics
  frames; reset and suspend around interruption/recovery.
- `Source/Latebrake/Private/UI/LatebrakeRaceHUD.cpp` — optional expanded
  HUD telemetry and honest finished-results page.
- `Tests/Standalone/VehicleGForceMeterTests.cpp` — yaw/world-axis
  correctness, braking conditions, low-speed suppression, NaN/Inf,
  dropped frames, teleport-style impulses, resets and 20k sample
  bounded-state stress.
- `Tests/Editor/VehicleGForceBindingTests.py` — source-only Unreal
  attachment and gameplay-authority boundary checks.

Run on a developer system with a C++20 compiler:

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^VehicleGForceMeter$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'VehicleGForceBindingTests.py' -v
```

**No source-only CI test compiles Unreal Engine 5.8/UHT, cooks the real
circuit, runs the Win64 game or certifies hardware G force.** Actual
vehicle suspension and tire physics are still prototype-level and need
careful validation against real Chaos world velocities. Measure HUD
layout at desktop sizes, all handling presets, pause/restart/focus,
rotation, brake/reverse, collisions, teleports and long frame hitches.
Keep raw and observed game logs and link the installed candidate's
SHA256 to [playable-release gate #148](https://github.com/sanskarIN/latebrake/issues/148).

Do not label this an automotive safety system, precise accelerometer,
human driver rating, or demonstrated gameplay until engine/device
testing has actually happened.
