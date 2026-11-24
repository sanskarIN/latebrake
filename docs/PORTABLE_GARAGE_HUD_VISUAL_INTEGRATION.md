# Portable Garage, HUD and original vehicle rendering integration

This change imports previously authored, previously draft-only C++20 policy
modules and portable tests from LateBrake's experimental Unreal/Win32 branch
([PR #452](https://github.com/sanskarIN/latebrake/pull/452)), on top of
`main` commit `dd815b60118befd07017d1bbf6fa705eb510e7dc`. It deliberately does **not** overwrite the
active Unreal physics, SaveGame, menu purchase authority, user input or the
existing Win32 executable with the older draft's bulk integration.

## Actual Unreal path

`LatebrakeRaceHUD.cpp` now reads the existing ArcadeNitroBoost telemetry
through `NitroHudReadout.h` to display observed ACTIVE, COOLDOWN, OVERHEAT,
EMPTY, BUILD SPEED, STORED or UNAVAILABLE status next to speed. It treats a
paused/menu/finished race or unready track as ineligible rather than
advertising stored charge as usable. This is a **read-only display** with no
new nitro physics, reward, SaveGame or driving authority.

## Other source capabilities brought under portable CI

- Unicode-aware bounded HUD text-flow and measured gear/RPM badge layouts
- desktop menu panel sizing, safe Garage action/focus decision policies
- bounded indexed original car render geometry and three-entry CPU cache
- profile save pre-write guards for future native integration
- Win32-preview policy units: keyboard focus rearm, viewport coordinate
  transforms, car look/selection, two-press purchase consent, driver coaching,
  race-time text, settings persistence and bottom-button activation
- five cross-system and viewport regression tests

Standalone helper modules are reusable source contracts. **Their presence
does not mean they are all connected to the current Unreal Canvas or top-down
Windows preview yet.** No actual Unreal 5.8 Editor/UHT build or cooked Windows
graphics test has been performed by this source-only change.

## Checkpoint and testing

All imported portable targets use C++20 and warnings-as-errors on MSVC,
Clang and GCC via the existing CMake test helper. The Win32-preview pure
policies get an explicit include directory, so they compile without the
Windows API. Python source-binding tests verify the live nitro HUD wiring.
Run full PR CI before merging and track GPU/cooked game acceptance in
[#462](https://github.com/sanskarIN/latebrake/issues/462).
