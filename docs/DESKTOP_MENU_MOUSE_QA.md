# Desktop mouse navigation for the experimental UE menu

The engine-side, asset-light **Canvas menu** now offers six mouse targets
when the menu is open in a sufficiently wide **desktop** viewport:

| Button | Existing action |
| --- | --- |
| PREV PAGE | Navigate to the previous menu category |
| NEXT PAGE | Navigate to the next menu category |
| DETAIL UP | Scroll back through this page's details |
| DETAIL DOWN | Scroll forward through this page's details |
| SELECT | Invoke the *existing* GameMode menu action |
| CLOSE | Close the menu and explicitly return to driving |

The mouse dock is never presented on Android or when a menu is closed.
Compact desktop windows continue using the keyboard/gamepad actions already
supported. The desktop menu is **not** a completed UMG/Slate system.

## Authority and input safety

The existing GameMode remains solely responsible for event starts, garage
purchases, credit awards, restart, save and race pause transitions.
The new pawn input handler only accepts mouse clicks from a locally
controlled pawn while the menu is open and refuses out-of-bounds,
invalid coordinates and any click outside the six buttons. It clears held
driving commands before routing an accepted click to a pre-existing menu
method. Mouse UI does not introduce a second gameplay progression path.

The GameMode toggles cursor visibility and game/UI input mode when the
menu opens/closes. The same shared, portable pixel geometry drives both
Canvas button drawing and mouse hit-testing, with real Canvas dimensions.
Text-fitting reserves the right-side menu dock so long menu descriptions
cannot overwrite buttons.

## Source validation

```sh
python -m unittest discover -s Tests/Editor -p '*Tests.py'
cmake -S Tests/Standalone -B build/test
cmake --build build/test --parallel
ctest --test-dir build/test --output-on-failure
```

Relevant tests: `HudMenuMouseControls` and
`DesktopMenuMouseBindingTests`. They cover 720p, 1080p and ultrawide
layouts, resize sweeps, invalid coordinates and disabled states.

## Required genuine Unreal/Windows checks

- Compile UHT and the actual UE 5.8 LatebrakeEditor/Game targets.
- Open TAB menu, test all six buttons with real desktop mouse, trackpad
  and DPI/scaling settings at 1280x720, 1920x1080 and ultrawide.
- Test open-close after starting/restarting an event; confirm the cursor
  is hidden again and no throttle/steering input remains latched.
- Verify keyboard, gamepad, mouse and Android input still function,
  including pause and alt-tab/focus-loss recovery.
- Check save errors, results pagination and menu detail clipping against
  every mouse button and Android action region.
- Profile Canvas text measurement and input latency on a real installed
  build. Source-only tests cannot prove touch or device behavior.

See [production UI milestone](https://github.com/sanskarIN/latebrake/issues/236)
and [genuine playable release gate](https://github.com/sanskarIN/latebrake/issues/148).
