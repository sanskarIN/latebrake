# Touch controls (v0.4.1 source candidate)

> **Experimental Android-only on-screen touch interface.** This source has not
> been compiled with Unreal Engine 5.8, run on a phone, or packaged as an APK.
> Rectangles in the engine HUD are not native accessibility widgets.

## Layout and gestures

The HUD and the vehicle input dispatcher use the same engine-independent
`UI/TouchControlLayout.h` rectangles, scaled to the live viewport:

| Screen area | Action | Interaction |
| --- | --- | --- |
| Lower left | Steering | Hold and move sideways; center means straight |
| Lower right (upper pad) | GAS | Hold to accelerate |
| Lower right (lower pad) | BRAKE | Hold to brake |
| Upper right, first pad | PAUSE | Tap to pause; tap again to resume |
| Upper right, second pad | RESTART | Tap to restart the current event, also unpauses |
| Upper right, third pad | RECOVER | Tap to place the car near its next gate; +5s penalty |

Steering and pedals support two independent simultaneous fingers. Touches
that **begin outside** a control cannot activate it merely by moving over it.
The gap between gas and brake is neutral; a captured pedal finger crossing
into the gap releases both. Invalid touch coordinates neutralize held values.
The pad tint changes while steering/gas/brake is held. Touching race telemetry
does not activate the vehicle.

The input binding for initial touches is configured to execute while paused;
the PAUSE button should therefore offer an Android resume path. Actual
input routing during pause still requires device verification.

## Engine/device acceptance checklist (NOT YET COMPLETED)

1. Compile `LatebrakeEditor` and the game target with a licensed Unreal 5.8
   installation and inspect compiler/UHT logs (including touch bindings).
2. On at least one landscape and one portrait Android viewport, check that
   all six pads appear in the expected positions without covering essential
   race information.
3. Hold steering plus GAS with separate fingers; move steering to each side.
   Release GAS and confirm the car stops accelerating without residual input.
4. Slide the GAS finger through the neutral gap and into BRAKE; confirm
   immediate release, braking and no unexpected reverse acceleration.
5. Start a touch outside the pads, slide into GAS and confirm the car does not
   accelerate until a **new press** begins inside GAS.
6. Tap PAUSE twice. Confirm physics stops, held inputs are cleared and play
   resumes without stuck steering or throttle.
7. Tap RESTART while paused and while driving. Confirm progress, penalties and
   rival resets and that the new race runs unpaused.
8. Tap RECOVER while racing; verify a single 5-second penalty and that repeated
   physical input works. Verify RECOVER cannot act after race completion.
9. Verify multi-touch release/cancel and focus-loss behavior on real devices.
   If OS interruption creates stuck fingers, implement explicit cancel/focus
   cleanup before any playable release.
10. Measure HUD legibility, touch-target size, thermal performance, frame time
    and input latency on representative low- and high-end Android hardware.

## Portable-only tests

Run:

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
```

`TouchControlLayout` checks bounds, hit-testing, steering clamping,
portrait layouts, safe button separation and malformed numeric input across
portable C++20 builds. These tests do **not** compile Unreal's touch dispatcher,
exercise the actual HUD render path, or replace physical device QA.

## Future accessibility work

Add configurable left/right-handed pad placement, larger minimum target
sizes, explicit mobile garage/event selection, touch joystick affordances,
screen reader labels, remappable controls, and production UMG widgets.
No such settings or features are claimed in this milestone.

## v0.5.0 source additions (unverified on Android)

- Two career pads (NEXT/CAREER) appear in the racing HUD.
- In the paused development menu, NEXT advances panels, SELECT activates,
  BROWSE cycles eligible career/challenge IDs, and CLOSE returns to driving.
- The menu suppresses driving touch dispatch entirely; touches on GAS or
  BRAKE must not affect the vehicle while menu focus is active.
- Lifecycle delegate hooks clear captured touch/analog controls on app
  deactivate, background and foreground transitions; unpossession/endplay
  also clear controls and remove listener registrations.

Physical-device testing of orientation changes, Android back/home,
notifications, fingers released while suspended, repeated foreground
activation, and touch overlap/legibility remains **required**. Do not close
issue #13 on the strength of portable geometry tests.

## Development menu pause reconciliation

When the development menu is open, keyboard Escape/gamepad Pause now
requests menu closure rather than directly resuming the game behind the
menu. On Android, the menu's CLOSE pad continues to use the menu route.
This is a source-level fix and **requires Unreal and real phone tests**
for actual pause delivery while the game is paused.
