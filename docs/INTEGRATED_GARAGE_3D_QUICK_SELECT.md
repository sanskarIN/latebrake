# Integration checkpoint: source showroom, route budgets and Ctrl+number selection

This branch combines PR #456's source geometry, shading, reference timing,
race/garage HUD readouts and validated route-budget planner with PR #460's
actual Unreal Ctrl+1–6 Garage selection handlers and portable policy tests.

## Deliberate composition

- Preserve PR #456's sequential Q unlock pricing, cumulative route shortage
  and distinct projected car performance from GaragePurchasePlanner.
- Preserve PR #460's six per-key native input bindings and GameMode SaveGame
  rollback on failed owned-car equipment.
- Integrate both in the *same* HUD: desktop row labels Ctrl+1–6 with sequential
  budget details and the Q-next flag; touch row labels remain ordinal.
- Register the new GarageQuickSelect test once without removing any geometry,
  shading, reference-clock or recent-lap tests from PR #456's CMake.
- Do not copy the WIP native 3D Pawn/GameMode/SaveGame changes from #452.
  Those require a separate engine build and QA gate before final integration.

## Validation required before merge

1. Portable C++20 CMake + CTest (Ubuntu/Windows/macOS) and Python source
   binding tests on this **exact integration head**.
2. Confirm the source-only OBJ/MTL geometry preview and existing rendering
   policy tests still pass; these are **not** genuine Unreal frames.
3. Licensed UE5.8 UHT/Editor compile for the native new keyboard bindings.
4. Cooked Win64 gameplay and owner review of menus, six-car selection, all
   source wheel/material fallbacks, save rollback, career restriction,
   performance and 3D visual output.
5. Only after merge is approved, close the superseded overlapping source PRs.
   Do not independently merge #456 and #460 into the combined branch.

**No tag, release, installer, or actual UE cooked game is implied.**

## Additional combined regression evidence

- Compare every locked row's *sequential route budget* against the first
  unlocked vehicle's true Q purchase price. The direct Ctrl+number does not buy
  a car even when its budget is affordable.
- Exercise all 64 six-car ownership masks in portable tests with unusual
  equipped IDs and negative/maximum credit values. A real owned car
  selection may only change the selected ID, never credits, car ownership
  or the saved engine/grip tuning level.
- Fail a real Win64 profile write and check the old car is both displayed
  and handling the same way after rollback. Pure C++ transaction oracles
  alone cannot certify this native behavior.
- On Android/touch, show numeric car positions without suggesting desktop
  Ctrl+number shortcuts. Repeated finger taps must not invoke purchase actions.
- Retest source-clock and result HUD plus all six source mesh previews:
  adding keyboard input must never erase the separate source UI work.
