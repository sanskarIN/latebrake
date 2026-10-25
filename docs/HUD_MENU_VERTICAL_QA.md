# Menu vertical overflow and save warnings — v0.5.68

> Unreleased SOURCE-ONLY development. v0.5.65 is still the latest
> published public GitHub source pre-release. This is not a verified
> UE 5.8 game build and includes no playable installer.

## Problem and scope

The normal race HUD has been vertically budgeted since v0.5.66, and
long text width is measured since v0.5.67. However the **separate menu
branch** in the same Unreal HUD used a different, unbounded callback.
Long Results, Career and Garage pages could place late rows below the
viewport or over the lower Android steering pad. In that state, a
late `WARNING: PROGRESS NOT SAVED` could also be off screen.

## Implemented source correction

- An independent production `HudMenuVerticalPlan.h` computes a
  deterministic maximum count of menu details with actual `HudLayout`
  font scale and the original `TouchControlLayout` steering start.
- The real `MenuLine` callback refuses surplus lines, rather than
  pushing `Y` beyond the window. It remains measured through the
  existing actual-font `FitHudText` for horizontal action-pad safety.
- If rows were omitted, the menu draws a width-checked
  `MORE MENU DETAILS HIDDEN` notice at a dedicated lower baseline.
  The UI does **not** pretend omitted text is available; a future
  scrolling UI must restore full page access.
- An independent SAVE FAILED message remains visible even after the
  menu detail limit, above the physical steering pad. This text is
  outside the bounded `MenuLine` callback.
- No menu choice, event selection, award, career progression, physics,
  credits, input hit-testing or SaveGame transaction is modified.

## Source checks

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R '^(HudVerticalPlan|HudTextBounds|HudMenuVerticalPlan)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'HudMenuVerticalBindingTests.py' -v
```

The new C++20 suite covers 240x240, portrait/landscape Android and
desktop viewports; verifies ordinary rows, the omitted-content notice
and the save banner never intersect vertically, with defensive
NaN/Infinity rejection. Unreal-source binding tests verify the actual
MenuLine callback obeys the plan and that save-failure text is drawn
separately. Full strict portable CI runs on Windows, macOS and Ubuntu.

## Needed before a playable release

- [ ] Author and cook a genuine original four-gate Unreal Engine 5.8 map.
- [ ] Compile UHT/Editor, cook game targets and package Win64/Android.
- [ ] Test every menu page on installed 360x640 and 640x360 mobile
      viewports, as well as small/high-DPI Windows game windows.
- [ ] Verify that menu overflow/ellipsis still leaves event choice
      understandable, and add real keyboard/touch scrolling or a
      paginated UMG replacement for full accessibility.
- [ ] Confirm the real font metrics and Android safe-area insets,
      rotation, screen-reader behavior and menu touch interactions.
- [ ] Inject save failure on a saturated Results screen and prove that
      the warning stays visible in genuine engine screenshots.
- [ ] Record screen video, physical device model, exact build
      source/map hashes, crash logs and installed package evidence.

**This source-only feature does not yet establish real Unreal render
correctness, phone compatibility or a playable game.**
