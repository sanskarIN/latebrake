# Observed in-game braking-distance estimate — source-only QA

This read-only HUD advisory projects how far the **prototype in-game car**
would travel while stopping if its just-observed rate of braking continued
unchanged. It is calculated from actual consecutive Chaos chassis velocity
samples; it is **not** a road-safety instrument, driver instruction, lap
ranking, vehicle physics modifier, or guaranteed stopping point.

## Behavior

- The policy accepts finite frame times from 1 to 250 ms and real planar
  chassis speeds from 9 to 100 m/s, with forward movement comprising at
  least 88% of planar speed. Invalid speed, reverse, major sideways motion,
  or an implausible speed projection hides the estimate.
- A single pressed BRAKE or speed reading is insufficient. Three
  consecutive plausible positive-deceleration pairs over at least 120 ms
  are required before displaying an estimated remaining distance.
- Observed deceleration must fall between 0.75 and 20 m/s^2, rejecting
  stationary, accelerating, stalled or collision-like velocity changes.
  A bounded running smoother dampens numerical frame-to-frame jitter.
- The estimate uses current planar speed squared divided by twice the
  filtered deceleration. It is hidden above 2 km to avoid misleading
  unbounded output. The HUD label explicitly says **CURRENT RATE**.
- Only the existing noncompact desktop telemetry shows the optional
  estimate, after authoritative lap/time readouts. Units follow the
  player's existing km/h vs mph display preference (meters vs feet).
- Release of both brake inputs, invalid physics, pause/menu, lost focus,
  teleport/recovery and restart discard the prior sample baseline.
  There is no saved history, prize, scoring impact or new mobile pad.
- The shared pawn also hosts a rival, but the display reads the current
  human player's pawn only. No rival AI logic or physics is changed.

## Portable source checks

    cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
    cmake --build Tests/Standalone/build --config Release
    ctest --test-dir Tests/Standalone/build -C Release -R "^ObservedBrakeDistance$" --output-on-failure
    python3 -m unittest discover -s Tests/Editor -p "ObservedBrakeDistanceBindingTests.py" -v

Source regressions cover sample confidence, plausible deceleration, correct
cm-to-meter conversion, reverse/slide rejection, interruptions, invalid
samples, thousands of repeated braking segments and integration guards.

## Real-engine qualification still needed

1. Compile licensed UE 5.8 Editor/Game targets, create the authentic
   cooked map and install the resulting Win64 game.
2. Measure the actual Chaos vehicle velocity and displayed values over
   30/60/120 FPS, brake/handbrake overlap, road turns, collision and
   changing grip. Reject misleading flicker or sudden large swings.
3. Pause, switch menus, restart, recover and background/foreground the
   installed game; no stale stop estimate should remain visible.
4. Inspect HUD legibility at 720p and 1080p and on compact Android
   touch viewports. Test a real installed Android package separately.
5. Capture genuine logs, package identity and screen recordings before
   making any claim about accuracy, playability or hardware performance.

Source CI does not establish Unreal Header Tool compilation, packaged
Windows executables, Android APKs or physical driving fidelity.
