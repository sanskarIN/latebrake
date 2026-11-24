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

## Exact PR #461 source-to-Windows acceptance procedure

Only run the native build on a Windows test machine with an authorized
installed Unreal Engine 5.8, Visual Studio C++ toolchain, Git LFS and
PowerShell 7. Read and review the PR diff before running build scripts.
The commands below produce a **private unverified candidate**, not a
published release or proof of installed gameplay.

In a fresh clone, use the live PR head rather than a stale SHA in docs:

```powershell
git fetch --no-tags origin pull/461/head
if ($LASTEXITCODE -ne 0) { throw 'PR fetch failed' }
$ExpectedHead = (git rev-parse FETCH_HEAD).Trim()
if ($ExpectedHead -cnotmatch '^[0-9a-f]{40}$') { throw 'Invalid PR SHA' }
git switch --detach $ExpectedHead
if ($LASTEXITCODE -ne 0 -or (git rev-parse HEAD).Trim() -cne $ExpectedHead) {
    throw 'Checkout does not match reviewed PR head'
}
git lfs pull
if ($LASTEXITCODE -ne 0) { throw 'LFS checkout failed' }
if ((git status --porcelain).Count -gt 0) { throw 'Dirty source checkout' }
pwsh ./scripts/Compile-UnrealEditor.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8'
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -Launch
```

After the genuine Editor/UHT and cook pass, record the source commit,
engine version, UAT log location, private candidate SHA256 manifest and
actual gameplay observations. Do not publish screenshots as GPU proof
without opening the real UE5 game. Keep unknown native visual results
explicitly **unverified**.

### In-game input and Garage acceptance cases

| State | Ctrl+1–6 | Expected effect |
| --- | --- | --- |
| Garage open, fresh profile | Ctrl+1 | Arrow remains equipped, no credit write |
| Garage open, fresh profile | Ctrl+2–6 | Locked feedback, no purchase, no ownership change |
| Garage open, Kestrel purchased with Q confirmation | Ctrl+1 then Ctrl+2 | Equip owned vehicles and persist selection |
| Garage open, insufficient future-car route budget | Ctrl+6 | No shortcut unlock, show sequential funding shortfall |
| Garage open, active timed career | Any Ctrl+number | Refuse car change; retain start-car eligibility |
| Other page, racing, or menu closed | Any Ctrl+number | No garage selection or money mutation |
| Save write forced to fail | An owned Ctrl+number | Restore previous selection, tuning and appearance |
| Android/touch interface | Any mobile gesture | Numbered rows, no desktop Ctrl instruction |

Also verify ordinary **1–4** driving shortcuts, **Q-twice** vehicle
purchase, **E/SELECT** cycling, mouse/gamepad menu navigation, results
and source fallback. When draft PR #452 is later integrated, test
**F3–F10** lighting, orbit and customization independently: they must
not be shadowed by these Ctrl+digit chords. Reinspect all source
changes after that integration; this checklist does not authorize
merging draft #452 or publishing a v0.5.x build.

## Garage feedback visibility on every detail page

The native Canvas menu still uses exactly two pinned header rows.
On the Garage page the second row is the current authoritative
`GetGarageFeedback()` (including staged Q/U/I purchase consent and
failed-save rollback) instead of generic navigation help. This avoids
an off-screen confirmation message when the user has scrolled down to
another vehicle's detailed performance comparison. Other menu pages
retain their existing navigation row. The content area still includes
the transaction feedback for full-history review. This change does
not bypass the existing eight-second double-press confirmation or
change GarageState, credits, ownership or SaveGame authority.

On a narrow Android window, explicitly page down until the first
Garage details row is hidden, then press a purchase action and ensure
its **new** feedback remains in the pinned header. Test success,
insufficient credits, expired confirmation and injected save failure
with actual Unreal input; source-level tests cannot verify glyph
rendering or touch delivery.
