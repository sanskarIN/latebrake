# Portable C++20 test suite

The portable suite compiles `Source/Latebrake/Private/Racing/RaceProgress.cpp` from the game source rather than copying the implementation into test fixtures. This allows Linux, Windows and macOS CI to catch regressions in game-independent race rules without installing Unreal Engine.

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --parallel
ctest --test-dir Tests/Standalone/build --output-on-failure
```

These checks **do not** validate physics feel, Unreal Header Tool generation, runtime map, input events, save APIs, cooked binaries or Android devices. See `docs/TESTING.md` for the Unreal-specific acceptance plan.

## New in v0.2.1

The `RivalRoute` target verifies waypoint/lap tracking and eight-second stuck recovery; the `SteeringFilter` target validates frame-step smoothing, invalid inputs and overflow protection. Both test the exact same header-only C++20 logic used in the Unreal-source gameplay classes. These are not substitutes for a real Unreal Engine build or playable package.

## v0.3.1 tests

`HudLayout` exercises viewport-dependent UI spacing and corrupted geometry; `RecordValidation` guards saved race time and completion counters. Updated `GarageState` tests cover invalid masks, owned-car cycling, costs and affordability. All are pure C++20 checks and do not certify Unreal Engine runtime rendering, game builds or SaveGame IO.

## v0.3.2 tests

`DriverInputSafety` covers normalized axis/touch input, nonfinite sensor values, frame invalidity and throttle smoothing. `GarageTuningKey` covers when live vehicle tuning should be reapplied versus when a balance change is irrelevant. These suites test real portable header implementations only; game-mode, physics, Android touch, camera and saving still require a licensed Unreal Engine build and manual validation.
