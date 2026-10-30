# Prototype arcade nitro boost — source feature and manual QA

**SOURCE ONLY. Not a playable Win64 game or Android package.** The policy
compiles in isolated C++20 tests; **Unreal Engine 5.8**, UnrealHeaderTool,
a cookable project-owned circuit and installed-device driving tests have
not been completed for this feature.

## Player controls and gameplay scope

- Keyboard: hold **B**. Gamepad: hold **left thumbstick click**.
- Applies only to an eligible **local human driver** after GO, on a
  physics-ready circuit with a valid race and placement. It cannot
  activate for the CPU rival, in menus, during pause/focus loss or
  after finish. Android touch has no new nitro button in this source
  iteration; mobile UI space remains reserved for essential controls.
- Requires at least **7 m/s measured forward chassis speed**, positive
  throttle of at least 0.5, and neither service brake nor handbrake.
- While active, the existing rigid-body motor force may increase by at
  most **38%** and the speed cap target by at most **12%**. The speed cap
  is interpolated to avoid an instant snap when boost is released.
  Vehicle steering, gate/race authority, save data and rewards are not
  modified by the nitro policy.
- Meter starts at 100% on race reset, consumes 25% per second while
  active, regenerates 8.5% per second during eligible nonboost driving,
  warms while boosting and cools otherwise. Overheating/depletion
  forces a 2.5s cooldown and a fresh key press before boost can fire
  again. It is a **fictional arcade power-up**, not realistic nitrous
  chemistry, actual engine tuning or professional racing physics.
- During input interruption, an invalid frame, a paused race or
  recovery, the held command is cleared immediately. An interrupted
  frame must never silently restore a held boost; meter state remains
  run-local until an explicit race reset.
- Nitro and a valid draft may overlap, but never grant an unbounded
  combined motor bonus: the global additive cap is **40%** of baseline
  drive force. At full 38% nitro, a further draft may supply at most 2%.
  The actual granted drafting share appears in the expanded desktop
  HUD. This is an arcade balancing rule, not physically measured engine
  performance.
- Expanded desktop HUD includes charge and active/cooling status in
  the existing speed row. Compact/touch HUD priority rows are unchanged.

## Local portable source QA

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R ArcadeNitroBoost --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'NitroBoostBindingTests.py' -v
```

The portable tests exercise invalid/nonfinite inputs, bad frame intervals,
reverse, low throttle, brake priority, player-only press admission,
thermal shutdown, cooldown, regeneration, stale press release and
5,000 repeated simulation cycles. These are **not** UE runtime tests.

## Required manual real-game test (still open)

1. Build the actual UE 5.8 editor/game targets and cook the circuit.
   Verify clean UHT/Unreal C++ compilation and save exact source/engine
   provenance plus Win64 executable hash.
2. In a genuine *installed* player race, accelerate over 7 m/s,
   hold B/gamepad thumb click and measure acceleration vs same-car
   nonboost runs on identical gear/handling/road conditions. Test
   real FPS consistency at 30/60/120 Hz.
3. Verify no nitro before the GO signal, in countdown, after finish,
   during Time Attack pause, menu, focus loss or failed engine pause;
   verify a fresh key press is required after reactivation.
4. Hold nitro through heat cutoff, brake, reverse, handbrake,
   recovery and restart; inspect real speed-cap transitions for
   abrupt motion or physics glitches.
5. Verify AI car never receives bonus acceleration, the HUD fits
   720p/1080p screens and existing Android multi-touch/steering UI
   is unchanged. Test garage tuning extremes and engine packaging.
6. Log any visible, audible, controller, frame-time or physics defect.
   Do not call any build verified playable until actual on-device
   checks and project-wide [playable gate #148](https://github.com/sanskarIN/latebrake/issues/148)
   pass.

This is a source candidate, **not a playable release**. Track hardware
testing and genuine package provenance separately.
