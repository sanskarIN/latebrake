# Hold-to-look-back camera — source-only gameplay candidate

The existing four camera presets (Chase, Close Chase, Hood, High Chase)
remain available. On keyboard hold **V**, or on a gamepad hold the **left
shoulder button**, to look rearward for traffic awareness during eligible
active racing. Release the button to restore the selected forward preset.

This is a temporary camera orientation, not a fifth saved preset or an
automatic reversing camera. It does not turn the vehicle, change physical
steering, brake, pause, checkpoint scoring, rival AI, credits or persistence.

## Eligibility and interruption

- Only a locally controlled pawn with a started, unpaused, focused,
  unfinished race and validated physics may acquire the held view.
- Input release must restore the forward view even if a world pause
  prevents ordinary game ticks; the explicit release binding runs paused.
- Pause/menu interruptions, OS focus loss, driver input clear,
  unpossession, restart and paid recovery clear the held state.
- Cycling among the four existing presets while held retains the held
  direction of the newly selected view. Releasing returns forward.
- Android does not receive an invented or untested additional touch pad.
  The existing camera-cycle touch action remains unchanged.

## Portable validation

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^HeldLookBackCamera$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'HeldLookBackBindingTests.py' -v
```

Source tests exercise denial on ineligible press, repeated press, physical
release, explicit reset and repeated interaction. Python integration checks
input mappings, real pawn ownership, pause/focus guard and Unreal spring arm
application. **These tests cannot verify UE 5.8/UHT compilation or installed
camera behavior.**

## Real build / gameplay qualification (outstanding)

1. Compile the actual UE 5.8 project, author and cook the original circuit.
2. Drive a genuine Win64 packaged game at both high/low speeds: hold and
   release V and the left gamepad shoulder in all four camera presets.
   Verify intentional reversed orientation, hood framing, collision
   probe near track walls and prompt restoration without visual clipping.
3. Hold the button during pause, menu, restart, recovery, focus loss,
   controller removal and app deactivation; confirm no stale rear view or
   change in motor/steering output on resume.
4. Verify gamepad hardware maps the right shoulder to preset cycle and left
   shoulder to look-back without interfering with ghost, brake or recovery.
5. Record real device information, game logs, build SHA and exact package
   hashes under [release qualification issue #148](https://github.com/sanskarIN/latebrake/issues/148).

There is **no verified installed executable or Android APK** for this feature.
