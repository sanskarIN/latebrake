# Production 3D chase-camera corner look-ahead (source qualification)

This feature is implemented in the **Unreal Engine 5.8 car pawn**, not the
separate experimental Win32/GDI Windows preview.

- A human-controlled car's **real signed forward Chaos speed** and **filtered
  steering axis** guide the chase camera into a corner by up to **6 degrees**.
  High chase has a tighter **4-degree** limit. The hood camera receives
  **zero** synthetic yaw; held rear view remains an absolute 180-degree view.
- The response uses a frame-rate-independent 0.24-second exponential easing
  and automatically recenters after steering returns to neutral.
- AI opponents cannot get this player-only camera effect. A stopped/reversing
  car, malformed or nonfinite physics values, invalid frame time, menu pause,
  lost input focus, recovery and camera switching restore the preset yaw.
- The camera only changes the spring-arm's relative view rotation; steering
  forces, AI, lap timing, checkpoints, race results, rewards and saved progress
  are **unchanged**.
- An existing conservative speed-driven FOV effect remains independent.
  This is an asset-light intermediate presentation improvement, **not** a
  replacement for high-quality 3D cars, suspension, track visuals or audio.

## Offline source tests (no UE 5.8 required)

```bash
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --parallel
ctest --test-dir Tests/Standalone/build -R CornerLookAheadCamera --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py'
```

The strict portable tests cover 20,000 continuous frames, camera/driver
ownership, input limits, frame hitches, reverse motion, view switching,
nonfinite inputs, recentering and 30/60/120 Hz equivalent elapsed time.
Source-binding tests check that the actual Unreal pawn reads its qualified
chassis state and restores camera orientation during blocked input.

## Real-engine qualification still required

On an **authorized UE 5.8 workstation**, compile the actual Editor/UHT
target first (see `scripts/Compile-UnrealEditor.ps1`). With a genuinely
cooked original circuit and installed Win64 build, check:

1. Chase and close chase: gentle turn anticipation at driving speed; no
   persistent sideways view after releasing the wheel.
2. High chase: road stays readable through sharp turns; hood remains fixed.
3. Hold/release rear view, toggle pause, switch views, restart, recover and
   trigger OS focus loss while steering. Verify immediate neutral reset and
   no stuck input.
4. Check camera boom obstacle collision, vertical slopes, frame pacing,
   input on keyboard/gamepad/Android touch and any motion discomfort.
5. Confirm race clock, ghost, checkpoint attribution, controls and saved
   records are unchanged.

Portable CI and source files **do not verify** UHT compilation, camera
rendering, authored map ownership, installed package or gameplay. Keep
playable qualification [#148](https://github.com/sanskarIN/latebrake/issues/148)
and full production UI/UX work [#236](https://github.com/sanskarIN/latebrake/issues/236)
open until actual device QA is complete.
