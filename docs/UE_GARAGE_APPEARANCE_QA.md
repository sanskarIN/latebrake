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
