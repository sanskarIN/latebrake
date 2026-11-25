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
- Inject inaccessible test storage and verify failed save restores ownership,
  selected car, tuning and appearance. Test old profile defaults, malformed
  appearance bytes/checksum and unknown future schema. Save prewrite guard
  reloads disk metadata and rejects unreadable/newer profiles; it is NOT an
  interprocess atomic lock. Avoid simultaneous instances.

### Pointer arbitration and input

- Confirm normal navigation, styling, six quick-view chips and four orbit
  targets each highlight and dispatch one matching action. Disabled targets
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
