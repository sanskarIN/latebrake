# Desktop Garage factory schematic and showroom geometry

This candidate reconciles the independent layout policies from draft PR #452
against the protected main source at 79a32648f11888725b1e7e827e222dbc9005aa68.
It keeps main's finite rectangle and wheel navigation guards. The runtime
change is a read-only desktop Canvas illustration of the equipped fictional
car's factory appearance, not a 3D render or saved customization.

## What is intentionally not enabled

- The future paint, stripe, rim, browse and orbit pointer geometry has portable
  tests but is not drawn or dispatched in the live HUD or pawn.
- The schematic is not a new inventory cursor; current E and Ctrl+1–6
  ownership/selection behavior stays unchanged.
- No new SaveGame fields, prices, unlocks, physics or executable assets.

## Native acceptance

1. On a licensed Unreal workstation, compile Editor/UHT and cook Win64.
2. Open Garage at 900x600, 1280x720, 1920x1080 and ultrawide/4K.
   Confirm the schematic stays in the right column under the existing
   menu dock, away from details, mouse controls and the save-error warning.
3. Cycle all owned cars; each illustration uses its equipped factory recipe.
   Check readable label, four tires, appropriate accents and visible body kit.
4. Click inside the schematic and reserved blank area: no purchase, selection,
   camera change or save should result. Existing six navigation buttons work.
5. Resize to a compact viewport and switch to Results. The schematic must
   disappear without retaining a hidden click area. Android remains unchanged.
6. Verify menu and save behavior with controller and keyboard on a cooked
   build. Attach screenshots and exact source SHA to issue #462.

## Portable coverage

Three registered C++20 tests cover schematic geometry, quick-view target
mapping, desktop lane, orbit targets and cosmetic hit admission. Source
binding tests assert that the runtime path draws only factory appearance and
does not dispatch unimplemented actions. These tests do not establish
native compilation or actual display behavior. Keep this integration draft
until exact-head portable CI and licensed Unreal runtime verification.
