# Live HUD vertical overflow and critical banners — v0.5.66

> **Unreleased source development; not UE 5.8 / installed-device qualified.**
> The C++20 geometry checks and Python Unreal-source contracts do not
> compile UHT, render Slate/Canvas pixels, create a Win64 EXE or Android APK.

## Original problem

The expanded racing HUD printed every available driving stat, timing
comparison, garage row and race insight in a single unbounded left-hand
column. With several laps, a career event, penalties and rival telemetry,
the growing `Y` could extend below the viewport; its old footer placement
`Max(Y+3, height-28*scale)` could move keyboard controls off screen.
Late save, finished-race and pause messages could be hidden behind controls
or never rendered in the viewport at all.

## Source-level correction

`UI/HudVerticalPlan.h` now computes a defensive bounded row count using
the **same** `HudLayout` scale/top/spacing used by the actual Unreal HUD.

- The race-clock TIME, official LAP/GATE, SPEED, event and startup warnings
  are rendered **before** optional radar, sector, garage and driving stats.
- The ordinary `Line` callback stops after `MaxTelemetryLines`.
  It does not keep incrementing a phantom Y after clipping.
- Two **reserved pixel baselines** remain for `WARNING: LOCAL PROGRESS
  COULD NOT BE SAVED` and the PAUSED / FINISHED / INVALID FINISH banner.
  They do not depend on regular telemetry remaining below budget.
- Desktop keyboard hints use a fixed footer baseline instead of following
  the length of the data column.
- On Android, the outcome baseline is above the real steering and
  accelerator pads from `CalculateTouchControlLayout`. The layout
  verifies supported viewports and rejects NaN/Infinity.
- No scoring, physics, input, recovery, credits, career progress or
  SaveGame logic is changed. The fix intentionally does NOT implement
  UMG scrolling, text wrapping or localization.

**Important remaining UI limitation:** On narrow Android screens, long
single-line telemetry text can still stretch sideways into the right-hand
action buttons. Pixel-accurate Android font/text-width checks and a
separate wrapped/ellipsized mobile HUD are still necessary. This source
change addresses **vertical** clipping and critical-status priority only.

## Source checks

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R '^(HudLayout|TouchControlLayout|HudVerticalPlan)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'HudVerticalPlanBindingTests.py' -v
```

`HudVerticalPlanTests.cpp` uses the real production `HudLayout` and
`TouchControlLayout` policies with 240x240, 640x360, 1280x720,
1920x1080, 2560x1440, portrait dimensions, invalid viewports and
corrupted plans. It asserts ordinary text stays above the save warning,
the alert stays above the fixed footer and mobile status remains above
the actual lower steering/pedal hit regions.

`Tests/Editor/HudVerticalPlanBindingTests.py` inspects actual HUD source
bindings. It requires the layout planner, row cap, authoritative
LAP/TIME placement before optional motion advice, fixed footer and
independent save/outcome status rendering.

## Actual Unreal/Android/Windows QA still required

- [ ] Compile UE 5.8 UHT, editor/game targets and the cooked starter circuit.
- [ ] Render at 640x360, 800x600, 1280x720, 1920x1080, 2560x1440,
      phone landscape/portrait and Windows DPI scales.
- [ ] Accumulate all optional telemetry: penalties, career series, sector
      bests, full garage, rival warnings and finished insights.
- [ ] Verify time, lap/gate, startup faults and SAVE FAILED banners remain
      visible, including paused+failed-save and finished+failed-save.
- [ ] Measure text glyph bounds and action pad rectangles; check horizontal
      overlap, focus, resizing, app rotation and safe-area insets.
- [ ] Ensure keyboard hints do not move beyond the viewport; check
      text contrast, accessibility scaling and dynamic font fallback.
- [ ] Retain video/screenshots, actual engine build logs, installed
      artifact hashes, device OS and reproducible failure details.

The public v0.5.65 release remains **SOURCE ONLY**, with no verified
playable Windows/Android packages. This v0.5.66 source milestone does
not constitute a release or license UE assets.
