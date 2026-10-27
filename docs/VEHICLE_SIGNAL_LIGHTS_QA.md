# Decorative brake/reverse lights — source-only gameplay development

## Implemented prototype visuals

The physics-driven Unreal car now has three short-range point-light
components attached to its original cube body: two red rear lamps and
one white central reverse lamp. They are low-complexity **cosmetic
lights**, not a licensed lighting asset, emitted texture, advanced
headlamp rendering or physical simulation.

- The rear red light pair has a low constant tail intensity while
  authorized driving is active and increases brightness immediately
  with actual effective BRAKE input (keyboard/gamepad/touch or rival).
- The white reverse lamp illuminates only when **measured signed
  forward chassis velocity** is at or below -130 cm/s. Reverse
  throttle while stationary cannot fabricate reversing motion.
- Braking and reversing can occur at once; both signals may illuminate.
- Red/reverse intensity is clamped to deterministic small policy values
  to avoid nonfinite light setup. Three point lights have short
  170-cm attenuation radii. Dynamic shadow casting, volumetric
shadows, global illumination and reflection contributions are all
disabled for the prototype lights to limit renderer workload. This is an early stylized effect, not
  photometric driving illumination.
- None of the lights changes physics force, collision, lap timing,
  AI routing, career credits, records, driver input, saved profile or
  game quality classification.

## Source structure

`Source/Latebrake/Public/Vehicles/VehicleSignalLights.h` owns a
header-only, allocation-free C++20 policy with explicit speed bounds
and fail-closed authorization gates. Three Unreal
`UPointLightComponent` components are attached to the chassis in
`LatebrakeCarPawn.cpp`; positions compensate for its non-uniform
prototype scale. The actual physics tick samples existing, fully
validated forward velocity and the real effective brake state after
the car's authoritative game/physics readiness checks.

Blocked driving, bad rigid-body data, failed physics readiness,
recovery/teleport, OS app-focus callbacks, `UnPossessed`, and
`EndPlay` clear the lamp intensities immediately. OS callbacks
matter because Unreal may stop Tick altogether while paused.
Invalid samples must never leave an illuminated lamp from an earlier
driving frame.

## Regression tests

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^VehicleSignalLights$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'VehicleSignalBindingTests.py' -v
```

C++ covers forward braking, held stationary brake, actual reverse,
reverse+brake, exact reverse-speed thresholds, invalid motion, race
interruption and a bounded velocity sweep. Unreal-source contracts
check actual component wiring, real physics gates, actual brake
signal origin and non-authoritative rendering code.

## Genuine Unreal and installed-game QA still required

1. Compile the actual UE 5.8 Editor/Game targets through UHT,
   produce the original cookable racing map, and launch the actual
   packaged Win64 EXE (not a CMake policy-test executable).
2. Inspect the rear light positions relative to the body with the
   chase, close, hood and high camera presets. Confirm the lamps
   actually illuminate visible surfaces in the intended renderer.
   Non-emissive point lights are **not a substitute for visible
   bright lens materials**.
3. Drive, stop, brake, back up, coast downhill, change vehicle,
   switch arcade/simulation-inspired handling, recover, restart,
   pause, deactivate app and resume. Confirm no stale brake or
   white reverse lamp remains on after invalid motion.
4. Capture rendering performance on target Windows graphics
   hardware and multiple real Android devices. Mobile dynamic
   point lights may be expensive or unsupported by some renderer
   settings; gracefully degrade or disable them if necessary.
5. Attach exact executable SHA256, Unreal logs, screenshots,
   compiler/engine versions and device evidence. Source and
   portable C++ success alone cannot certify the game.

**Status: source-only visual development, not an installed playable
release or validated lighting system.**
