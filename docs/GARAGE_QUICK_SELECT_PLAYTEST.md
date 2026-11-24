# Garage F1–F6 direct selection — owner playtest

This is a **native Unreal Garage input/UI change**, not a new vehicle,
mesh, or separate Win32 preview. The C++20 unit tests validate only the
pure read-only selection policy. They do **not** prove the actual UE5
keyboard bindings, visibility, handling, or cooked Windows build.

## Expected desktop behavior

1. Start a clean save, open the **Garage** page via Tab and Page Up/Down.
   Check that the six entries are labeled F1 through F6, with ownership.
2. Press F1: Arrow remains equipped; no credits or saved profile change.
   Press F2–F6: each locked slot reports it must be unlocked first.
   No action spends credits or starts an unlock confirmation.
3. Earn credits, purchase Kestrel through the existing **Q twice**
   confirmation, then press F1 and F2 in either order. The selected
   car, its visible appearance and its handling should update, persist
   across restart, and never change shared engine/grip upgrades.
4. Select Garage and press F3–F6 with an owned sparse/non-contiguous
   ownership profile. Only owned cars can equip; never rely on
   affordability alone. The current sequential Q unlock path remains.
5. Go to Quick Race, Career, Results, Settings or close the menu. F1–F6
   must **not** switch cars, spend credits, or unpause the game.
   Existing 1–4 racing shortcuts must still work as before.
6. In a live timed Career event, F1–F6 must be blocked if the Garage
   page is reachable. No score, race-start vehicle or career medals
   can change. Check the finished-race restriction separately.
7. Inject a SaveGame write failure while selecting an owned car:
   game state and live vehicle tuning must return to the previously
   equipped car; no phantom persisted selection is allowed.
8. Try mouse wheel and Arrow Up/Down on a compact window. All six
   slots should remain readable through Garage details pagination.

**Native release gate:** UE5.8 UHT/Editor build, cooked Win64,
actual keyboard focus, car visuals, save failure recovery, and
gamepad/touch regression must be checked by a tester. Source-only CI
does not satisfy that gate. No release/tag is authorized by this document.
