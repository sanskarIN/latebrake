# Desktop racing instrument cluster — source QA

The Unreal race HUD now has a modest, asset-free instrument panel at the
**lower right of wide desktop viewports**. It is a UI iteration, not production
UMG/Slate work or proof of an installed racing game.

## Presented values

- **Speed**: actual player pawn reported speed, respecting the profile's
  km/h versus mph preference. The display caps nonsensical values to
  999 km/h rather than letting text overflow.
- **Nitro**: existing current-session boost charge, shown as a 0–100% bar.
- **Engine heat (arcade)**: measured game-model heat fraction, not a real
  vehicle sensor.
- **Chassis condition**: 100% minus current race collision-damage fraction,
  not a vehicle safety rating or persistent save/garage attribute.

Every bar has a written label and percentage. The fill is never used to make
racing or purchasing decisions.

## Layout and safety contract

The panel is absent on Android, compact windows, short desktops, menus and
finished races. All values and dimensions must be finite. The standalone
layout policy computes panel geometry without engine headers and reserves a
right-hand text lane, so long existing telemetry is truncated *before*
encroaching on the instrument area. The widget sits above the desktop footer,
and no new touch hit region or control ownership is introduced.

The rendering code may change **presentation only**. It must not mutate
physics, drive controls, lap clocks, checkpoint ordering, credits or save data.

## Verification

Source checks:

```sh
python -m unittest discover -s Tests/Editor -p '*Tests.py'
cmake -S Tests/Standalone -B build/test
cmake --build build/test --parallel
ctest --test-dir build/test --output-on-failure
```

Relevant portable test: `HudInstrumentCluster`, which sweeps desktop
viewports and checks invalid/compact/touch/menu/results cases and bounds.
Related source contract: `HudInstrumentClusterBindingTests.py`.

A qualified Unreal Engine 5.8 Editor/UHT build and real Windows/Android
installs are still required before claiming gameplay, font metrics,
high-DPI screenshots, contrast testing or frame-time performance. See
[playable-release gate](https://github.com/sanskarIN/latebrake/issues/148)
and [production UI issue](https://github.com/sanskarIN/latebrake/issues/236).
