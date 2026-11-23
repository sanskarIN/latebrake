# Unreal Engine garage appearance — source-candidate QA

**Status:** draft engineering integration. The Windows 2D trial in
`Experiments/Win32Preview` already has **12** original cars and a separate
appearance save. This change is for the **six** existing production Unreal
starter vehicle IDs (Arrow, Kestrel, Phantom, Tempest, Glider, Nova).
It does **not** prove that a UE5.8 Editor has compiled or displayed these
meshes or that a cooked 3D package exists.

## Scope and data ownership

- `GarageAppearanceState` is a pure C++20 allocation-free style system:
  eight paint choices, clean/center/twin roof stripes, and
  street/sport/track wheel-spoke trims, per one of the six immutable IDs.
- It cannot alter unlock bits, earned credits, engine upgrades, tire grip,
  Chaos collision, brake forces, race clock, career medals or replay state.
  It never selects an unowned car for the local human player.
- `ULatebrakePlayerSave` has three additive `SaveGame` fields:
  `GarageAppearanceSchemaVersion`, `GarageAppearanceBytes` and
  `GarageAppearanceChecksum`. Schema 1 writes 18 deterministic bytes
  (6 cars × paint/stripe/rims) plus a noncryptographic integrity checksum.
  Absent/legacy fields default to the existing factory paint identity;
  invalid/truncated records reset **only cosmetics**. Unknown higher
  appearance schemas make the entire profile read-only to prevent
  accidental loss of future style and career fields.
- The authoritative GameMode gates changes to the *open Garage menu*,
  verifies the viewed car is owned, cancels pending purchase
  confirmations, applies the visual change and calls the existing
  SaveGame write. A failed write restores the original appearance in
  memory and shows explicit not-saved feedback.
- The live Pawn reuses existing dynamic material instances. Two small
  non-colliding roof stripe meshes are visible only for non-clean
  styles; four decorative attached spokes change rim width/color.
  No per-frame mesh creation, no gameplay physics changes.

## UI controls

Open the real Unreal garage menu, **not** the Win32 2D preview.
Press **F5** to cycle paint, **F6** for roof stripes or **F7** for
decorative rims. On gamepad, click **R3** for paint, click **L3** for
roof stripes, or use **LB** for rim trims while the Garage menu is open
and paused. Those same controller buttons remain assigned to their old
driving actions outside the garage, and stale driving input is cleared.
Use **F8** or the separate **FACTORY** mouse button to reset only
the currently viewed owned car's paint, stripes and rims. This reset has
the same persisted-save confirmation and failure rollback, and never
resets another car's styling or purchases.
On desktop Canvas HUD the lower-right garage-only
dock also has separate mouse-hit buttons for each. The dock disables
its appearance targets when a career race or newer SaveGame schema
prevents editing; the same condition is enforced by GameMode. Appearance controls
are inactive when another menu page is open, after garage closure,
and while editing is not permitted by active career rules. Each
successful change shows its saved label. All cosmetics are free.

The two projects intentionally differ: the 2D experiment uses its
own `garage-appearance-v1.dat` and V/B/N keys. Production Unreal
uses `ULatebrakePlayerSave` and F5/F6/F7; those files are **not**
interchangeable, and a 12-car 2D save cannot grant Unreal ownership.

## Required validation before merging or publishing

1. Run `cmake -S Tests/Standalone -B Tests/Standalone/build`,
   build with strict C++20 warnings, run `ctest --output-on-failure`
   and run `python -m unittest discover -s Tests/Editor -p '*Tests.py'`.
   This is portable source testing, not UHT.
2. In an authorized UE5.8 Windows Editor workspace, compile the full
   Latebrake C++ game (including Unreal Header Tool). Verify that all
   new UPROPERTY fields serialize/load and neither GameMode nor Pawn
   introduces a UHT reflection failure.
3. Launch the actual 3D starter circuit, open Garage, cycle F5/F6/F7
   and click the three new dock buttons at 900x600, 1280x720,
   1920x1080, window resizing and high DPI. Confirm actual material
   parameters exist on built-in primitive meshes and that both stripe
   bars and wheel trims change visibly. Capture comparative screenshots.
4. Verify a legacy save preserves its six ownership IDs, credits,
   times, career medals and engine/grip levels with original factory
   colors. Set two different per-car paint/stripe/wheel combinations,
   save, quit, restart, reselect and verify independence/persistence.
5. Test corrupt data using a backup disposable save slot: bad length,
   bad checksum, invalid paint/rim index, and future schema. No unsafe
   default unlock, invalid car selection, or silent progress overwrite
   is permitted. Test a denied disk write; the visual change must be
   rolled back and the HUD must show NOT SAVED.
6. Start a career race, attempt customization and ensure it is rejected
   until permitted; verify wheel physics, acceleration, steering,
   pit service, scoring, engine boosts and race persistence are
   unchanged. Check focus loss, paused menu input, mouse hitbox
   boundaries, non-Garage pages and controller navigation.
7. Only after genuine UE Editor/UHT, cook, packaged Win64 and
   installed-machine playability QA should an Unreal playable
   trial/pre-release be considered. CI cannot certify 3D visibility.

**Known limitations:** This is primitive-mesh visual customization,
not imported production car meshes, licensed car brands, a complete
UMG showroom, physically modeled wheels, advanced liveries, wheels
or body-kit marketplace, or multiplayer replication. Material parameter
support and final art fidelity remain unverified until engine QA.

## Read-only multi-vehicle showroom

Open the Garage and press **F9/F10** (gamepad **RB/B**) to browse all
six original Unreal prototypes, including vehicles not yet owned.
The Canvas HUD displays the viewed car name, ownership/locked status,
cost in the existing catalogue and estimated effective acceleration,
grip and speed. A VIEWING marker distinguishes this inspection from
the actually equipped car; browsing cannot equip, purchase, spawn,
modify Chaos physics or serialize a locked car. **E** cycles to the
next owned car during an eligible quick race or from the visible Garage;
**Enter/SELECT** specifically equips the owned car currently being
viewed. **Q** still buys only the first locked ID after two explicit
confirmation presses while the Garage is visibly open; viewing a later
locked car does not bypass that purchase order. Cosmetics apply to the
currently viewed car only if it is owned, without needing to equip it.
Locked previews remain read-only; browsing arrows remain in the dock.

The two extra buttons below the garage paint/stripe/rim/reset controls
let the mouse browse the previous or next showroom car. Unlike the four
style actions, these two browsing controls stay usable even when the
player lacks permission to edit appearance (e.g. future SaveGame read-only
mode). The hit tester and visual disabled states use separate owner
permissions. At 900×600 and larger the three-row dock remains inside the
viewport; compact/mobile views retain F9/F10 or gamepad navigation.

## Equipping the viewed car (no direct locked-car purchase)

Press **Enter/SELECT**, or click the existing desktop menu **SELECT**
button, while a garage showroom card is being viewed. This calls the
GameMode's new authoritative `GarageSelectViewedVehicle` path. An owned
car is equipped and saved; an already equipped car is a no-op; a locked
car is explicitly rejected without charges or unlock. The old **E** key
still cycles to the next owned car. Q continues its existing **first
locked ID / double-confirmation** purchase policy; viewing a later car
never permits bypassing that order. Every newly equipped car is applied
to the true player pawn. An unsuccessful SaveGame write restores the
previous equipped vehicle, its physics tuning and original cosmetics.
The read-only preview cursor stays separate from saved gameplay data.

## Style the viewed owned car without changing the equipped car

The browsed showroom ID and the equipped car ID remain intentionally
separate. **F5/F6/F7/F8 and matching controller/mouse buttons target
exactly the currently VIEWED car, provided it is owned.** No equip step
is needed to customize another owned vehicle. A locked preview is still
read-only, and a denied style action reports why it cannot be applied.
The Canvas details now show **VIEWED LOOK** alongside **EQUIPPED LOOK**,
so browsing cannot silently repaint a hidden car.

The authoritative GameMode checks an explicit pure ownership helper
before editing. It persists all six per-car looks in the existing 18-byte
schema and uses SaveRecords rollback on failure. It calls
`ApplyGarageToPlayer` only when the newly styled car is actually the
equipped car, preventing the race pawn's paint and physical setup from
changing while parked-car styling. The preview schematic uses the
viewed car's look without touching collision, physics, credits,
career state or SaveGame selected vehicle. Browsing still writes nothing.

## Canvas showroom vehicle schematic (view-only, not 3D capture)

The production Unreal Canvas Garage renders a **car top-view 2D
schematic** beneath the six existing desktop actions in the right dock.
The drawing is derived from the SAME source definitions as the prototype
pawn: `VehicleVisualRecipe`, `VehicleBodyKitRecipe`,
`PaintForGarageLook`, `GarageAppearanceState`, and immutable six-car
`GarageState`. It reacts to browsed locked/owned car IDs and to every
valid paint/stripe/rim selection, including clean/center/twin roof stripe
count and three distinguishable wheel trims.

This is a screen-space, noninteractive display list: it never spawns
a locked vehicle into the course, creates a physics body, changes the
camera, charges credits or updates saved ownership. The diagram
deliberately uses Canvas `DrawRect` tinting so the palette can always
be visually inspected, even if the installed Unreal built-in primitive
material does NOT expose a parameter named `Color`. This is **NOT a
photorealistic vehicle mesh, native 3D rotating car viewer, texture
render target, or screenshot of UE's actual 3D scene**.

The allocation-free C++20 layout policy checks the actual pixel bounds
of the style/browse dock and reserves only spare space underneath.
It fails closed on compact/touch windows, insufficient panel height,
nonfinite coordinates and invalid/future vehicle/look IDs. Strict C++20
`GarageShowroomSchematic` tests enumerate all 6 × 8 × 3 × 3
car/look combinations on seven desktop viewports; an additional resize
sweep and corruption probes guard the visual pane from overlapping the
mouse buttons. Manual UE Editor verification remains necessary for
Canvas font scale, high-DPI legibility, material parameter and input QA.

## Garage gamepad driving-input isolation

**P0 regression addressed in source:** The same controller inputs are
intentionally mapped to garage operations and driving controls: R3 /
handbrake, L3 / nitro, LB / look-back, RB / camera, and B / recover.
Previously the new `CycleGarage*`, reset and gallery browse pawn
callbacks called `ClearDriverInputs()` **unconditionally** even when
racing with the menu closed. A shared `CanConsumeGarageShortcut`
source-only predicate now checks **both** `Mode->IsMenuOpen()` and
`Mode->GetMenuPage()==Garage` **before** any live driving-input clear
or GameMode mutation. Invalid GameMode and hidden page also fail closed.
Each of six callbacks retains its existing authoritative GameMode
economy/ownership/saved-look gates when it is legitimately focused.

Standalone strict C++20 tests cover all 11 menu pages, contradictory
closed/open states and 300,000 menu transitions; Python source-contract
tests verify every real Pawn callback's guard order and the five
overlapping gamepad button mappings. These prove source behavior and
policy correctness but do not replace actual gamepad testing: in UE
Editor, drive with R3/L3/LB/RB/B held and verify throttle, steering,
nitro, handbrake, camera and recovery behave as before; open the
Garage, confirm UI actions work; close it and verify racing inputs
resume without stale values. The menu remains authoritative over
cosmetic edits; this fix cannot grant cars or credits.

## Paused garage E / Q / U / I controls

The actual Unreal garage menu calls `SetGamePaused(true)`. Existing
E/Q/U/I bindings were missing `bExecuteWhenPaused = true`, despite
the visible Garage HUD promising E to select the next owned car, Q to
unlock the next car, U to upgrade its engine and I to upgrade grip.
All four Pawn action bindings now execute while the menu is paused.
Q/U/I preserve their prior paid double-confirm authority checks.

The E hotkey still cycles owned vehicles in eligible quick races with
NO menu open. When a menu IS open, E now accepts only the Garage page,
so Settings/Career/Results/Main cannot silently equip a car.
Future-schema SaveGame is denied before any transient selection.
Purchase authorization, career race protection, SaveGame rollback and
the Enter/SELECT showroom target remain independently enforced.

QA in licensed UE5.8: pause and open Garage; test E/Q/U/I (including
the existing two-click spend confirmation), verify credits and selected
vehicle after restart. Change to non-Garage pages and verify E cannot
equip cars behind the menu. Close the menu in Quick Race and verify E
retains original ownership-only cycling. Run the real controller focus
QA separately; standalone tests verify only source contracts.

## Separate E selection from Q/U/I spending and standalone PAUSE

The ordinary Garage menu calls `SetGamePaused(true)`, so these
bindings must work while the Garage is open. However, **E** is the
only allowed menu-closed shortcut: it cycles already-owned cars during
an eligible active quick race and never charges credits. A shared
`CanUseGarageVehicleHotkey` helper rejects E when another menu
owns focus or when standalone pause/focus-loss is latched.

**Q/U/I can stage or confirm a transaction ONLY in the visibly open
Garage page.** The stricter `CanUseGaragePurchaseHotkey` rejects a
closed menu (even in an otherwise active quick race), any non-Garage
menu page, and independently latched manual pause or OS focus loss.
Unreal's ordinary Garage menu world pause remains allowed; there is
still no way to spend outside the two-step, unchanged-snapshot,
eight-second confirmation gate. An application deactivation also
cancels any previously staged purchase offer immediately, even when
the open Garage was already world-paused.

Standalone C++ tests cover 11 pages and 300,000 menu transitions.
In real UE5.8 QA: verify E alone still changes eligible owned cars
during active quick races; confirm Q/U/I while driving never stage
an offer or alter credits; check all four keys during standalone
race pause with **no menu**, then in non-Garage menus and the actual
Garage. A focus-loss between the first and second purchase press must
require two fresh keypresses after return, with no hidden purchase.

## Future SaveGame profile: do not stage transactions

A saved profile written by a newer schema must remain read-only. In
addition to E selection and appearance editing, all **Q/U/I**
vehicle-unlock/engine/grip purchase handlers now reject
`bFutureSaveSchema` BEFORE even staging the two-press purchase
confirmation or mutating any credits/tuning. The Garage HUD displays
a visible "NEWER SAVE PROFILE: READ-ONLY, NO PURCHASES" notice.
The read-only car gallery still supports browsing, but no write may
silently downgrade a newer SaveGame or create a false success.

Manual UE QA: load a newer cosmetic/career schema, open Garage;
F9/F10 browse should still work, but Q, U, I, E, F5–F8 must not
change in-memory profile state, activate purchase staging, change
credits or overwrite the newer slot. Legacy and same-version saves
retain confirmed purchased upgrades with rollback on write failure.

## Explicit pause and focus-loss suspension (P0)

The ordinary Unreal world pause created by opening the Garage must not
be confused with the independent manual race pause and OS focus-loss
latches. E/Q/U/I are allowed during a visible, normally paused Garage,
but **cannot equip, unlock or stage upgrades while either external
suspension latch is active**, even if a Garage page is subsequently
shown. Enter/SELECT on a browsed owned vehicle and all F5–F8 appearance
writes are likewise denied. The desktop customization buttons show
this disabled state. The Garage details column explicitly says
**GARAGE SUSPENDED: RESUME TO EQUIP, BUY OR STYLE** rather than
incorrectly claiming an already-equipped car must be equipped again. Read-only showroom inspection still has no economy
authority. The six shared gamepad garage callbacks reject external
suspension before touching held race inputs.

Verify in genuine UE5.8 on a real desktop/controller: open Garage while
normally menu-paused and confirm approved E/Q/U/I, F5–F8 and owned-car
selection still work; then suspend the race with the standalone pause
or an OS focus interruption, open/navigate to Garage, and confirm **no
selection, spending, cosmetic save, or invisible driver-input reset**.
Resume explicitly and verify the controls become available again.
Also repeat for another menu page and for a newer read-only SaveGame.
Portable C++/Python tests cover policy and source order, not engine
focus-event delivery or controller hardware.

## Paid shortcut routing: visible Garage is required

Q, U and I may **only** stage or confirm a purchase while the Garage
page is visibly open and not externally suspended. Normal Unreal
`SetGamePaused(true)` by the Garage does not disable these routes.
Pressing Q/U/I while actually driving, while viewing another menu
page, or while under a standalone pause/focus-loss latch now rejects
before `ConfirmOrStage` and resets any pending purchase. This is a
player-facing correction: racing should never secretly stage a paid
transaction with no visible price or confirmation message.

E deliberately **retains** its existing eligible quick-race ability to
cycle owned cars with the menu closed and the world unpaused. This
exception is not extended to paid commands; an owned switch never
unlocks or charges the driver. Enter/SELECT still equips the *viewed*
owned garage car through the separately guarded showroom route.

Manual UE5.8 acceptance: in a live quick race, press Q/U/I twice and
verify no credits, upgrades or ownership change. Open a non-Garage
menu and repeat. Open the actual paused Garage, confirm that Q/U/I
display the price and two-press confirmation and can purchase only
when allowed by profile schema/credits. Exit Garage between the first
and second press, return, and verify the former staged offer was
cancelled. Confirm E still cycles owned cars in eligible live quick
races. The new portable 300,000-transition test and editor source
binding checks supplement, but do not replace, real controller QA.

## Focus interruption cancels pending purchases

If Q/U/I has displayed the first of two purchase-confirmation prompts,
an application deactivation immediately invalidates that transient offer.
This cancellation runs *before* the existing focus guard decides whether
the game was in its paused Garage menu; the focus guard correctly skips
race-time suspension when the menu was already open, but must **not**
preserve previous spending consent. On return, the first new Q/U/I press
may only stage a fresh offer, and a second within the normal consent
window is still required. Browsing and saved ownership remain unchanged.

Test with real UE5.8 on Windows: open Garage, press Q once, Alt-Tab out
and return within eight seconds, then press Q once. The first post-focus
press must **not** debit credits or unlock; it must show a new offer.
Repeat with U and I, across all six cars and with an external controller.
Portable C++ tests exercise reset/re-consent semantics; source-binding
tests verify the real GameMode focus event calls Reset before its early
return. OS callback delivery must still be verified in the native Editor.

## Acceptance matrix: owned parked-car style

In real UE5.8, own Arrow (0) and Glider (4), but keep Arrow equipped.
Open Garage and browse to Glider. F5/F6/F7 and the desktop style dock
must be **enabled**, show Glider's new paint/stripes/rims on the 2D
schematic and persist those choices without changing Arrow's active
3D Pawn, selected vehicle, engine/grip upgrades or credits. On return
to Glider, the look must remain. After quit/relaunch, opening the
same SaveGame must restore BOTH cars' independent cosmetic choices
under the unchanged appearance schema version 1.

Browse locked Phantom (2). F5/F6/F7/F8, the corresponding controller
bindings and desktop style buttons must be denied/disabled; previewing
must not grant ownership or write to its saved look. Repeat when a
career race denies editing, on manual suspension, focus loss and a
future SaveGame schema. Simulate failed save writes and confirm
the previous parked-car look is restored without applying a different
look to the active Pawn. Verify no hidden, accidental editing when
showroom pages change. These are engine/hardware acceptance tasks;
portable C++ and source-binding tests are not proof of actual visuals.
