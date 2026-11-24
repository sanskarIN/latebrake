# Garage paused-hotkey and transaction acceptance (Unreal 5.8 / Windows)

**Scope:** source-side correction for paused E/Q/U/I action bindings and for
authoritative Garage-page-only equipment/purchase operations. This guide
does not certify an engine build, physical keyboard dispatch, saved profile
reliability, GPU visuals, or a Windows executable.

## Why this change exists

Previously the new Ctrl+1..6 bindings explicitly set
`FInputActionBinding::bExecuteWhenPaused`, but the old E/Q/U/I bindings did
not. Unreal's paused-game input filtering can drop those older actions exactly
when the native Garage menu is on screen. The old GameMode methods also
allowed cycling and two-step spending attempts while the menu was closed.
The fix makes the original four bindings pause-compatible, **but** only the
visible Garage page may authorize equipment changes or purchase staging.
All credit spending remains subject to its existing eight-second second-press
confirmation and its existing save rollback.

## Native owner QA matrix

On a licensed Unreal Engine 5.8 Editor build and separately on a cooked
Windows build, run every scenario with a new profile and a profile containing
multiple owned cars. Record actual input device, menu state, credits, equipped
vehicle, displayed feedback, save/reload result and screen capture.

| Context | Action | Expected |
| --- | --- | --- |
| Driving, no menu | E | No equipped-car change |
| Driving, no menu | Q twice | No purchase staging, no credit deduction |
| Driving, no menu | U twice, I twice | No upgrade or credit deduction |
| Paused Main/Career/Results page | E/Q/U/I | No garage change, no hidden approval |
| Paused Garage page | E | Cycles to the next owned vehicle, saves once |
| Paused Garage page | Ctrl+1..6 | Selects owned requested car; locked car remains locked |
| Paused Garage page | Q once | Shows next unlock price, does not deduct |
| Paused Garage page | Q twice within eight seconds | Unlocks next eligible car once if affordable |
| Paused Garage page | U/I once | Shows appropriate upgrade price only |
| Paused Garage page | U/I confirmed | Upgrades appropriate shared level once |
| Paused Garage page, insufficient credits | Q/U/I | Visible rejection, no mutation |
| Active locked career event | E/Q/U/I | No mutation even when Garage page visible |
| Pending Q approval, switch menu page and return | Q once | Fresh preview, not an old approval |
| Pending purchase, leave Garage / start race | Q/U/I | Pending approval discarded |
| Simulated failed profile write | E/Q/U/I confirmed | Exact previous equipment, credits and upgrades restored |
| Garage input while paused | Escape / Tab | Menu still closes/resumes normally |
| Garage key repeat / rapidly alternating keys | Q/U/I/E | No double charge or race-control bleed-through |

## Verification boundaries

- `Tests/Editor/GaragePausedInputBindingTests.py`: static source wiring and
  authorization contracts only.
- `Tests/Editor/GaragePurchaseConfirmationBindingTests.py`: retains
  purchase confirmation and rollback source contracts.
- Existing portable `GaragePurchaseConfirmation`, `GarageState`, and
  `GarageQuickSelect` tests must remain green.
- **Unverified:** Unreal Editor keyboard dispatch, modifier interactions,
  pause ownership, cooked Win64 session, save-file failure under OS-level
  denial and UI appearance. These require a real engine/device test.

## Integration

This change belongs to the combined six-car showroom/quick-select PR so
that E/Q/U/I and Ctrl+1..6 receive one consistent QA review. Do not
independently merge the old F1-F6 shortcut PR, which conflicts with the
3D showroom's function-key controls.
