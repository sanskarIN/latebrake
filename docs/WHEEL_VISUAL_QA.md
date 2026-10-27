# Procedural wheel visuals — source-development QA

## Gameplay change

The starter car already has four visible Unreal-built cylinder wheels.
They previously remained static because they were collision-free ornaments
attached to a separate, physics-simulated chassis. This feature makes
their motion track the actual player car's signed forward chassis speed,
rather than an invented wheel RPM.

- The four non-colliding wheels roll both forward and backward.
- The two front wheels turn (up to ±30 degrees) with the *same*
  existing smoothed and speed-limited steering demand used for the
  chassis torque. Rear wheels do not steer.
- A slender collision-free primitive cube on each outward-facing wheel
  cap makes rotation visible even though a bare cylinder looks identical
  at every rolling angle.
- The cylinder's local spindle is rotated into the vehicle's Y axis,
  not incorrectly aimed along the vehicle's forward X axis.
- This is a lightweight, deterministic visual treatment: there is
  **no separate wheel rigid body, suspension, traction, tire friction,
  gearbox simulation, physical angular velocity, reward or race-time
  influence**. It is not a claimed Chaos Vehicles conversion.

## Safe numerical policy

`Source/Latebrake/Public/Vehicles/WheelVisualMotion.h` contains
the same portable C++20 implementation used by Unreal. A valid
frame converts measured forward speed (cm/s) and wheel radius (cm)
to degrees of rotation: `angle += speed × delta / radius × 180/pi`.
It wraps the visual phase to ±180° so arbitrarily long sessions cannot
overflow. Reverse speed subtracts rotation. The prototype radius
is 29 cm, derived from the default UE cylinder geometry/scaling.

The existing actual player track/start, physics, pause, focus, and
driver-readiness checks execute *before* any wheel motion. Zero/negative
or greater-than-250ms frames, NaNs, speeds beyond 15,000 cm/s,
out-of-range steering and nonsensical wheel dimensions are rejected.
Blocked driving suspends visual updates and neutralizes the front steering
angle without inventing additional wheel rotation. Player recovery and
race restart reset wheel animation, so a long spatial teleport never
appears as free wheel travel.

The rolling effect is intentionally an approximation of rigid-body
motion. Because the existing car is an arcade force-controlled **cube**
rather than a physical wheeled vehicle, wheels may still visibly
roll during side slip, airborne motion or skids without true road
contact. That behavior must not be described as physically accurate
tire rotation. Cosmetic axle spin does not establish a cookable map or
a running, correctly rendered Unreal game.

## Changed project code

- `Source/Latebrake/Public/Vehicles/WheelVisualMotion.h`
- `Source/Latebrake/Public/Vehicles/LatebrakeCarPawn.h`
- `Source/Latebrake/Private/Vehicles/LatebrakeCarPawn.cpp`
- `Tests/Standalone/WheelVisualMotionTests.cpp`
- `Tests/Editor/WheelVisualBindingTests.py`

Run portable regressions:

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^WheelVisualMotion$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

Tests cover correct signed roll/reverse, real visual steering, pause,
teleport, invalid input, speed/radius bounds and 100,000 consecutive
short frames. The source-binding suite checks wheel mesh orientation,
spoke collisions, safe actor lifetime and actual physics integration.

## Mandatory Unreal and installed-device validation

1. Compile the project in Unreal Engine 5.8 with UHT and the real
   `Latebrake` Game and Editor targets, not only portable CMake.
2. Cook the actual project-owned circuit and launch a genuine Win64
   candidate. Verify basic vehicle collision, checkpoints and the
   authoritative race timer remain untouched.
3. Inspect the wheel axes and outward-face spokes using front, rear,
   left, right, hood and chase cameras. Check no odd wobble or wheel
   clipping across all three garage body silhouettes.
4. Drive straight, steer at low and high speeds, brake, reverse, stop,
   drift, collide, pause/focus, restart and recover. Confirm wheel
   movement remains legible and never adds steering or force.
5. Check mobile GPU/frame-time regressions caused by four added cube
   meshes on representative real Android devices. Original track,
   physics and touch QA remain outstanding.
6. Preserve honest build receipts, game logs and screenshots tied to
   the exact tested Win64 and Android candidate SHA256.

**Status: source-only implementation.** No real Unreal/UHT compilation
or installed graphics and hardware QA has been performed for this
feature in hosted GitHub Actions. Source artifacts are not installers.
