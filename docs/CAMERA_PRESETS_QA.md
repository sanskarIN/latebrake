# v0.5.72 Four-view camera + Android CAM QA

## Source-delivered features

The original single C-key near/far toggle has been replaced by a
**four-preset** native Unreal spring-arm camera cycle:

| C press sequence | Arm | Pitch | Intent |
| --- | ---: | ---: | --- |
| 0 / 4: CHASE | 560 cm | -12° | Original rear follow camera |
| 1: CLOSE CHASE | 270 cm | -9° | Tighter racing perspective |
| 2: HOOD | 0 cm | -3° | Low forward-mounted camera (verify vehicle clipping in engine) |
| 3: HIGH CHASE | 720 cm | -40° | Steep elevated view of nearby track |

Each preset explicitly controls spring arm/socket offsets, lag and
collision probe and bounded static lens field of view. Camera state is ephemeral to the pawn; it does not
change driving/AI forces, checkpoint scoring, career progression,
reward accounting, multiplayer, game saves or lap record eligibility.
Unsupported enum values fall back to safe CHASE. The non-compact HUD
shows the view name **after** official race lap/time lines.

On Android, the **already reserved upper-middle MenuBrowse rectangle**
displays **CAM 1/2/3/4** during driving, indicating the active preset, and keeps its existing **BROWSE** menu
function while the menu is open. The new policy only accepts a genuine
freshly pressed finger on this specific control when a real mode,
qualified circuit and local driver exist, with no game/menu pause or
focus-loss latch. Dragging a steering/pedal finger into CAM never
switches the view. Input safety and viewport ownership remain unchanged.

## Independent strict source tests

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release \
  -R '^(CameraPresets|TouchCameraCyclePolicy)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

The C++20 tests verify distinct deterministic framing, 4→1 cycling,
reset, invalid configurations, correct names, exact shared
`TouchControlLayout` disjoint hit tests, fresh press gating, paused
and menu/focus/restart conditions, and all unrelated action regions.
`CameraPresetBindingTests.py` checks real UE pawn spring-arm updates,
keyboard action binding, shared Android menu-vs-race rectangle,
live race HUD ordering and source/test registration.

These are source tests, **not Unreal Editor/UHT compilation**.

## Required engine/device acceptance before playable claims

- Compile real **licensed UE 5.8** Editor and game targets with UHT,
  record build SHA, UHT logs and toolchain.
- Create and cook the original owned circuit map, start a car with
  authenticated collision/physics and ensure the view does not pass
  through the chassis, cabin, barriers or scenery.
- On Windows, cycle all four views while accelerating, braking,
  reversing, resetting the car, crossing gates, pausing/resuming and
  using menus. Verify default Chase is the original camera.
- On Android test real multitouch: steering + throttle + CAM, drag over
  CAM while a pedal finger is held, press duplication, rotation while
  a finger is down, background/resume, menu BROWSE, pause and recovery.
  CAM must not steal or latch accelerator/steering fingers or permit a
  camera cycle on an invalid or unfinished race world.
- Check several device aspect ratios and DPI: low right-hand buttons
  remain clear; upper-middle numbered CAM must not overlap the speed/time HUD,
  SAVE FAILED warning or menu controls. Verify natural camera lag, lens field of view and
  spring arm collision behavior at high speeds and narrow track walls.
- Attach actual package provenance, screenshots, device model/OS,
  failures and benchmark metrics to
  [Android issue #13](https://github.com/sanskarIN/latebrake/issues/13).

**The v0.5.72 implementation is source-only development, not proof of
a working UE camera, cooked original track or playable game build.
Source archives contain zero verified game executables.**
