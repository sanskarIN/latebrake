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
  verifies the selected car is owned, cancels pending purchase
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
the currently equipped car's paint, stripes and rims. This reset has
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
modify Chaos physics or serialize a locked car. **E/SELECT** still equips
only the next owned vehicle according to the existing GarageState rules;
**Q** still buys only the next locked car after explicit two-step
confirmation. Cosmetics apply to the real equipped car, not a locked
preview. Browser pointer arrows remain in the Garage-only dock.

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

## Prevent invisible customization of the wrong car

The browsed showroom ID and the equipped car ID are intentionally
separate. **Paint, stripe, rim and factory reset become unavailable
whenever those two IDs differ.** A preview of a locked or differently
owned car remains readable, but pressing F5/F6/F7/F8 cannot silently
repaint the equipped car. The input dispatcher and on-screen button
availability share this condition. Players should first use
**Enter/SELECT to equip an owned viewed car**, after which its own
stored look can be edited. The equipped car label is explicit in the
Canvas HUD, and a denied key press displays a clear explanation.

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
