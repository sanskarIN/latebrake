# Frame-stable chassis resistance — source QA

## Scope

The experimental rigid-body car applies first-order force-based resistance
for lateral slide, brake/coast and opposing-throttle engine braking. Before
this change a long frame could request a damping impulse greater than
the signed velocity of one chassis axis, potentially overshooting that
axis through the damping term alone.

`Vehicles/FrameStableDamping.h` bounds each finite nonnegative per-second
rate to **min(requested rate, 1 / measured positive frame seconds)**. This
does not add grip, increase throttle, alter steering, override player inputs,
award records or change saved progress. It applies in the shared base pawn:
human keyboard, gamepad, touch and offline rival AI use the same integration.

- Normal 60 FPS behavior retains the existing 8.5/4.25 lateral grip,
  7.0 brake, 0.55 coast and 3.0 opposing-throttle rates.
- A 250 ms frame caps the arcade lateral grip rate to 4.0/s; a
  500 ms frame caps the braking rate to 2.0/s.
- Invalid or nonpositive frame/rate values return no damping from this policy.
  The real pawn already blocks nonpositive/nonfinite `DeltaSeconds` and
  unready Chaos state before applying drive forces.
- It is **not** a tire model or a promise of collision stability. Chaos may
  substep, vary execution order or have external collision forces; actual
  frame timings and force behavior require a licensed Unreal build.

## Portable source tests

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R FrameStableDamping --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'FrameStableDampingBindingTests.py' -v
```

The C++20 suite checks ordinary frames, lag spikes, reverse/forward signed
momentum, frame/rate envelopes, invalid values, huge and tiny finite frames.
Python verifies the real pawn uses bounded rates for all three call sites.
These **do not** run Unreal Header Tool, compile the game, cook the map,
produce executables or prove installed performance.

## Unreal and device acceptance (still required)

1. Compile the licensed UE 5.8 editor and packaged game with the original
   source hash recorded. Author and cook the actual circuit.
2. On Windows and Android measure real chassis longitudinal/lateral speed
   before and after braking at 60 FPS, 30 FPS and controlled 125/250/500 ms
   game-thread stalls. Compare direction changes and stopping distance.
3. Repeat in Arcade and Simulation Inspired, with normal braking, throttle
   reversal, tight turns, stationary vehicle, collisions and AI rival.
4. Test pause, focus loss, input release and paid recovery: no stale thrust,
   unintended lateral drift, phantom race time or score changes.
5. Preserve logs, build fingerprints, screenshots and tester observations
   under the existing playable-release QA issue. If unstable under real
   Chaos substepping, tune with measured device data before merging.

**Release status:** source candidate only; no verified Win64 EXE or Android APK.
