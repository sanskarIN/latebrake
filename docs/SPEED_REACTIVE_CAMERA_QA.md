# Speed-reactive race camera — source-only implementation

A subtle visual-only camera effect uses genuine positive *longitudinal*
chassis velocity to widen the existing selected camera preset's lens
by at most **six degrees**. The baseline Chase/Close Chase/Hood/High
presets remain unchanged and the final field of view stays at or below
105 degrees. It does not change spring-arm collision, steering, motor
forces, grip, progression, lap timing, credits or records.

The portable C++20 policy smoothly eases over approximately 0.3 seconds,
caps vehicle speed inputs at 300 km/h and rejects invalid frame times
or hitches above 250 ms. On reverse, pause, menu, invalid physics, race
finish, focus interruption, recovery, restart, camera cycling or held
rear-view, the effect resets to the ordinary camera FOV. Autonomous rival
pawns never own human speed-reactive framing.

## Source validation

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^SpeedReactiveCameraFov$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'SpeedReactiveCameraBindingTests.py' -v
```

Source regressions check signed velocity, bounded visual-only state,
continuity across different frame rates, high-speed saturation, reverse
or malformed velocity, interruptions, and the real UE pawn FOV callback.
They are **not Unreal Header Tool compilation or installed gameplay**.

## Real UE 5.8 and device acceptance required

1. Compile actual Editor/Game targets with UE 5.8 and cook the authentic
   circuit map. Verify that UCameraComponent and Chaos integration build.
2. Observe all four camera presets from idle to top speed and braking.
   Check framing, oscillation, motion sensitivity, visibility near walls,
   and smooth response across 30/60/120 FPS and artificial frame spikes.
3. Hold and release rear view while at speed; confirm it has the
   unmodified preset FOV and restores correctly on release.
4. Verify menu, pause, background/foreground, focus loss, recovery,
   restart and invalid physics never preserve old widened FOV.
5. Test a real Android phone/tablet and Win64/gamepad with screen
   recordings, performance, renderer compatibility and build hashes.

This is not a verified playable executable or APK, and is **not part
of the already planned v0.5.80 source release**.
