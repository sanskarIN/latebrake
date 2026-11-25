# Native Garage reconciliation — source candidate

**Status: unverified native game; no cooked executable or release.** This
integration ports the previously authored six-car native showroom work from
draft #452, plus the layout work in #469 and the focus cancellation in #468,
onto the protected-main baseline 79a32648f11888725b1e7e827e222dbc9005aa68.
Do not count those overlapping branches again when measuring contributions.
The extra source tests are not native Unreal tests.

## Owner acceptance (issue #462)

Before any merge of the full native candidate or claim of playable rendering,
perform an actual UE5.8/UHT compile of Editor and Win64 game targets on a
licensed workstation. Cook the actual original circuit and verify real Windows
keyboard, mouse, controller and GPU rendering. The portable and Python source
contracts check behavior and bindings, not rendered pixels or packaged builds.

### Six-car rendering

- Inspect the original body and four independent animated wheel mesh
  components for all 18 car/rim combinations. Observe steer, roll, reverse,
  brake/reverse lamps, body/roof stripes and paint under real lighting.
- Verify all five generated components are visual-only, have no collision,
  overlap or physics simulation, and the real Chaos chassis is unchanged.
- Inspect Material vector parameter discovery; the portable tint selector
  tests 65,536 parameter masks. A missing component, invalid mesh, missing
  Material or missing tint parameter must keep visible legacy fallback.
- Validate camera fit and four F4 angles on 900x600, 1280x720, 1366x768,
  1920x1080, ultrawide and 4K. No clipping into text/menu dock, no hidden
  controls; restore race camera on closing Garage. Obstruction falls back
  to normal camera rather than changing the vehicle or race state.

### Ownership, consent and appearance

- Browse all six original cars using keyboard/controller and direct view chips.
  A locked car is preview-only; browsing never saves or changes race identity.
- E cycles owned cars only from the open Garage. Enter/SELECT equips the viewed
  owned car, Ctrl+1..6 selects the requested owned ID. They use one transaction
  and restore the actual equipped identity after rollback.
- F5/F6/F7 cycle paint, stripes and rims on the viewed owned car; F8 restores
  factory. Parked owned car styling cannot change live physics, credits or
  selection. Locked, future-profile, career-locked, closed-menu and explicit
  suspended contexts reject mutations. Verify no-op factory reset does not save.
- Q unlocks the next sequential locked car, U/I upgrade; first press previews
  only, second identical press within eight real seconds confirms. Verify
  pinned countdown across pages, focus loss/background cancellation before
  the race guard's ignored transition, and fresh preview after returning.
- On supported desktop sizes, click the next-car, engine or grip strip. Check
  actual credit shortfall, blocked/max state, and the confirmation countdown.
  The first click stages only; a second click requires a matching visibly drawn
  confirmation. Resize, expire or change the offer between display and click:
  the stale target must not charge credits. Both mouse and Q/U/I share the
  same authoritative transaction and save rollback. All credits are in-game.
- Inject inaccessible test storage and verify failed save restores ownership,
  selected car, tuning and appearance. Test old profile defaults, malformed
  appearance bytes/checksum and unknown future schema. Save prewrite guard
  reloads disk metadata and rejects unreadable/newer profiles; it is NOT an
  interprocess atomic lock. Avoid simultaneous instances.

### Original vehicle material finishes

- Settings diagnostics show 0–5 exposed global scalar parameters. A zero count
  means tint-only; a nonzero count is not shader/pixel verification. Inspect
  the actual parent material graph and check its shading model in Unreal.
- Compare satin graphite with pearl white, factory versus sport alloy,
  painted body versus tire rubber, and vinyl stripe versus body paint under
  the same lighting. Only supported scalar targets are applied. The starter
  material is not an authored automotive PBR shader; do not advertise actual
  clear coat, transparency or metallic flakes until a real material graph
  and cooked visual comparison demonstrate those effects.
- Switch cars, paint, rims and stripes; ensure missing scalar capabilities
  never prevent the tint-only car from rendering and old capabilities are
  cleared on source rebuild/fallback. Verify no geometry rebuild on paint-only
  or stripe-only edits, and no change to physics materials or tire contact.

### Direct cosmetic chooser

- Keyboard N or controller D-pad Right opens Paint and then switches through
  Stripe and Rims. Up/Down, PageUp/PageDown or D-pad Up/Down moves focus;
  Enter/Y chooses; M/D-pad Left cancels. Confirm only the visibly focused
  card. Rapid move+confirm before a fresh frame must not choose a hidden
  card or equip the race car. Focus outline and active cosmetic are distinct.
  When the chooser is closed, ordinary page, detail, career and equip inputs
  must retain their existing behavior. Compact/Android support remains
  limited to the existing controls, not this desktop chooser.

- Open Paint, Stripe and Rims with desktop mouse on each of six cars. Choose
  any of eight paints, three stripe patterns and three rim styles directly.
  Verify canonical names, actual paint swatches, active and factory tags.
- Stripe choices display zero/one/two bands using the viewed paint and shared
  stripe tint. Rim choices depict the original 7/5/12 spoke designs with shared
  native material colors. These are schematic outlines, not captured GPU
  renders. Verify visibility and contrast against selected/hovered cards;
  compare on the actual original and legacy fallback car under real lighting.
  No text or icon may obscure a neighboring card or authorize an unseen choice.
- Native stripes now use prepared three-band procedural overlays sampled from
  the actual body roof. Inspect all six cars from the four camera angles,
  particularly sloped portions of the roof. Clean must show no band, center
  exactly one, twin exactly two; no visible gaps, floating bars or z-fighting.
  Stripe-only and paint-only edits must not rebuild the body mesh. Missing
  stripe preparation must keep the legacy car and its fallback stripes
  visible rather than committing a partially prepared native vehicle.
- Confirm the chooser occupies only the central showroom aperture, closes
  after selection, and restores the unobstructed car view. Validate small,
  ultrawide and 4K windows. When the bounded panel cannot fit, mouse cycling
  remains available; F5/F6/F7 and gamepad cycling are unchanged.
- Choose the already active option: no save or mesh rebuild should happen.
  Try an old displayed option after changing car, changing appearance with a
  keyboard shortcut, resizing, leaving/re-entering Garage or losing focus.
  No stale pointer action may update another car. Locked/read-only views must
  not open an editable chooser.
- Inject failed saving and verify the prior look, not just the chooser label,
  is restored. Test each paint/stripe/rim field independently on a parked
  owned car and confirm credits, ownership, upgrades and equipped ID remain
  unchanged.

### Pointer arbitration and input

- Confirm normal navigation, styling, six quick-view chips and four orbit
  targets, plus three purchase offers, each highlight and dispatch one matching action. Disabled targets
  cannot leak clicks to another group if geometry overlaps.
- Confirm page/close navigation still works with locked or already-equipped
  viewed car. Compact/touch layouts retain supported controls and no hidden
  desktop hits. Ensure switching pages and Alt-Tab clear held controls.
- Verify the standalone preview remains a distinct 2D experiment; do not
  attach or advertise it as a native Unreal 3D executable.

Attach exact commit SHA, hardware/OS, engine version, build and cook logs,
screenshots, controller observations, save failure checks and relevant crash
logs to the owner acceptance issue. Keep the PR draft until source CI and
native QA are independently verified.
