# v0.5.74 — human controller deadzones (source milestone)

## Scope and rationale

The native UE player pawn shares keyboard A/D and gamepad left-X
steering, and W/S with signed right/left trigger throttle/reverse. Real
gamepads often report small fluctuations near their rest position.
`DriveAxisDeadzone.h` adds stateless, bounded **human-only input**
gating with smooth remapping of remaining analog travel.

- Steering axis uses a symmetric 12% center deadzone.
- Throttle/reverse axis uses a symmetric 5.5% center deadzone.
- `+1.0` and `-1.0` (full input including keyboard) remain exactly
  full input; no steering or throttle power is permanently lost.
- An input at or inside its deadzone becomes exactly zero, both for
  forward and reverse.
- Outside the deadzone the remaining travel maps continuously back
  to the full 0…1 range without sudden jump at the threshold.
- NaN, infinity and malformed threshold settings fail closed to zero.
- The new filters appear only inside actual player
  `ThrottleInput` and `SteerInput` callbacks; AI
  `SetAutonomousControls` and touch's independently validated screen
  geometry do not pass through this added deadzone.

This milestone makes no changes to car forces, suspension/tire
simulation, recovery, opponent route policy, credits, checkpoint
scoring, save schema or persistent user options.

## Build/test

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release \
  -R '^DriveAxisDeadzone$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

The strict C++20 tests cover signed endpoints, both center deadzones,
intermediate steering/throttle remapping, invalid numbers/thresholds,
symmetric dense input sweep, monotonic controls, independent touch/AI
sanitizer and deterministic outcomes. Python source contracts verify
the exact runtime callbacks and `DefaultInput.ini` mappings.

## Real-world Unreal/device verification still mandatory

- Build with licensed UE 5.8 Editor/UHT and game targets and cook the
  authentic original map.
- Use a keyboard, Xbox-style and PlayStation-style gamepads on
  an installed Windows build. Inspect resting analog readings and
  ensure unattended car motion is neutral across USB/BT reconnects.
- Verify full-throttle/reverse travel and low-speed left/right turning,
  split triggers, combined keyboard+gamepad interactions, focus loss
  and pause/restore across unusual frame times. Check default
  thresholds are acceptable and make them configurable only when
  real playtest evidence supports this.
- On Android verify the existing steering and GAS/BRAKE touch pads
  do not change sensitivity or finger ownership and menu inputs
  still suppress vehicle drive.
- Record device models, controller firmware/driver, source SHA,
  screenshots/logs and actual build/package receipt for
  [playable qualification #148](https://github.com/sanskarIN/latebrake/issues/148).

**v0.5.74 is source development, not a published or playable release.
No real UE/device validation or executable package is claimed.**
