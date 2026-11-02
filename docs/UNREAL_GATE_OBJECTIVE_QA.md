# Next checkpoint 3D objective beacon (Unreal source only)

**Status:** source implementation and standalone policy tests. This is
an improvement to the **official Unreal Engine C++ checkpoint actor**, not
the separate experimental Win32/GDI preview. No licensed Unreal Engine
5.8 build, rendered level, cooked executable, or physical controller test
is implied by this feature.

## Motivation

All four checkpoint markers were previously the same visual size. This
makes the next required gate harder to distinguish from the other
out-of-order markers, especially after a restart or recovery.

## Behavior

- The actual `ALatebrakeCheckpoint` actor asks the existing authoritative
  `RaceProgress` for its **next ordered checkpoint**.
- With a circuit passing the startup safety audit, all four markers are
  visible. The **single next gate** expands to 14×14×0.16 relative mesh
  scale and illuminates a small blue-green non-shadow-casting PointLight;
  the other gates are 8×8×0.08 with their lights turned off.
- During failed circuit startup, invalid checkpoint counts/indexes or
  finished races, the markers and objective beacons are hidden. On
  restart or event change they update automatically, without caching
  a previous session's gate authority.
- The plan is *purely cosmetic*: no new trigger, blocking primitive,
  checkpoint acceptance, reward, SaveGame, event, lap time, reset
  authorization or score mutation. Repeated frame updates are suppressed
  when the visual state is unchanged.

## Validation

`GateObjectiveVisualTests.cpp` covers four gate selections, invalid
progress/count/index values, failed startup, finished races, exactly one
highlight and safe passive markers. Editor-source Python checks confirm
that the **real UE Actor** consumes the plan and retains its existing
directional and teleport-safe gate validation.

**Still needed on a licensed Unreal 5.8 Windows workstation:** full
UHT/Editor build, actual starter map load/cook, visually verify that
the light and dimensions are legible but not distracting, inspect GPU
cost/dynamic-light rendering, restart, one-lap Time Attack, three-lap
Circuit and career mode, pause/focus loss, and check that every gate
remains collision-free. The genuine executable/installed-device gate
remains [issue #148](https://github.com/sanskarIN/latebrake/issues/148).
