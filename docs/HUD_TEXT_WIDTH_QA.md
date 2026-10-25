# Horizontal HUD text safety — v0.5.67 source work

> **Source-only milestone, not an installed Unreal Engine game build.**
> The latest **published** GitHub pre-release remains v0.5.65.

## Why this matters

The v0.5.66 vertical HUD fix keeps lines inside the screen, but even a
visible line can be wider than the allowed left-side text area. On Android,
real PAUSE / RESTART / RECOVER and career-navigation pads begin partway
across the viewport. Long event names and race telemetry could otherwise
be drawn **over touch buttons**. Character-count clipping would be wrong
because Unreal proportional fonts have different glyph widths.

## New source-level work

- Engine-independent `HudTextBounds` finds the rightmost permitted
  horizontal text coordinate using the **same production**
  `TouchControlLayout` rectangles as the actual input handler.
  Conservative phone layout leaves a small gap before the earliest
  career/menu/touch control button. Non-touch desktop bounds remain
  inside the viewport.
- In `ALatebrakeRaceHUD::DrawHUD`, one shared `FitHudText` callback
  calls **Unreal AHUD::GetTextSize** for the exact font and DrawText scale.
  If measured text overflows, binary search determines the longest
  prefix that fits with a `...` suffix; impossible measurements fail
  closed instead of spilling over the screen.
- Race telemetry, career/garage/results menus, fixed save-failure
  message, paused/finished status, and desktop footer all use the
  same width-checked text function.
- Compact Android lap/gate display is abbreviated (`L1/3 G2/4`),
  keeping official progression labels ahead of optional data. Mobile
  reserved status uses the more actionable `SAVE FAILED`,
  `PAUSED - TAP PAUSE` and `FINISHED - RESTART` language.
- The new clipping policy has no authority over touch hit testing,
  game pause state, physics, checkpoint scoring, credits or saved data.
  It never modifies the actual touch pad rectangles.
- Truncation is code-unit based; a UTF-16 high/low surrogate boundary
  is backed up in the Unreal binder so an emoji pair is not split.
  Language-aware grapheme/word wrapping and screen-reader accessibility
  require further game UI work.

### Portable regression checks

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R '^(TouchControlLayout|HudVerticalPlan|HudTextBounds)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'HudTextBoundsBindingTests.py' -v
```

`HudTextBoundsTests.cpp` uses actual portable production
`HudLayout` and `TouchControlLayout` on 240x240, 360x640, 640x360,
800x480, 1280x720, 1920x1080 and 2560x1440 viewports. It checks
non-overlap against the leftmost action-pad edges, invalid/nonfinite
viewports, suffix that cannot fit, bounded logarithmic prefix probes
and inconsistent synthetic font measurements.

`HudTextBoundsBindingTests.py` checks the real Unreal source calls
`GetTextSize` (not a hard-coded per-character estimate), and binds
the font fitter to menu, telemetry, footer, save, pause, finish and
compact official LAP/GATE display.

## Genuine engine and device acceptance still needed

- [ ] Compile Unreal 5.8 with licensed Editor/UHT and game targets,
      cook the actual original course and package both OS candidates.
- [ ] Screenshot landscape and portrait Android HUDs using a real
      device in every menu, driving, pause and finish state. Test
      unusual and long generated event names.
- [ ] Measure rendered glyph bounds at different Android DPI / font
      settings, Windows high-DPI scales and viewport resizing.
- [ ] Verify truncation does not suppress essential save warnings or
      make lap/clock ambiguous. Consider a future UMG accessible
      HUD with tooltips, wrapping and adjustable detail levels.
- [ ] Assert that the rightmost drawn text pixel is strictly left
      of the next Android action control.
- [ ] Test Unicode/combining graphemes and non-Latin localization
      in a licensed Unreal runtime; portable source tests are not
      evidence of a rendered-font result.
- [ ] Attach build logs, game binary hashes, device screenshots and
      repeatable in-game evidence to relevant qualification issues.

This source change is **not** proof of playable Win64 EXE or
Android APK/AAB, physics, scene rendering or installed touch behavior.
