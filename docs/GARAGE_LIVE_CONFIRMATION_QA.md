# Garage live confirmation countdown — owner playtest

This branch exposes the **existing eight-second, two-press purchase approval**
to the *paused Unreal Garage* UI. It does not relax the transaction gate,
create currency, modify the SaveGame schema, or authorize a purchase through
rendering. All observations below must be verified on the owner's installed
Unreal Engine 5.8 workstation and cooked Win64 game; portable tests alone
do not certify actual on-screen updates or keyboard focus.

## Intended UX

- Open the Garage using the normal in-game menu controls. Each Q, U, or I
  preview shows the action, full price, the matching confirmation key, and
  a decreasing number of seconds on the pinned Garage status line.
- Only a **second identical key press** with unchanged credits, ownership,
  selected vehicle, and upgrades inside the eight-second window can attempt
  the purchase. Reading/drawing the countdown must not change the deadline.
- A pending preview with elapsed time greater than eight seconds or a
  changed snapshot shows **offer expired or garage changed**, not an obsolete
  instruction that falsely implies the old approval remains active.
- E, Ctrl+1..6, Escape/Tab, changing pages, starting a race, or otherwise
  changing context must preserve their existing cancellation behavior.
- On a short screen, the pinned feedback is visible regardless of which
  paginated Garage detail window the player is currently reading.

## Hands-on playtest matrix

1. **World paused and menu open** — open Garage, press Q once. Observe the
   remaining seconds decline while the background car/race stays paused.
   Check that the first press changes no credit balance or ownership.
2. **Within window** — press Q again before expiry. It may spend credits
   only for the next sequentially unlockable car, and only once; compare
   credits and ownership to the route-budget list.
3. **Expired** — press U once, wait over eight real seconds, observe the
   expiration notice, then press U again. That first post-expiry key is a
   fresh preview: it **must not** charge credits or upgrade the engine.
4. **Switch action** — press U then I. The pending action must become I;
   a subsequent U must start a new U preview rather than confirm the older
   U. No key switching may accidentally buy either upgrade.
5. **Invalidation** — stage a purchase, navigate to another menu page,
   return and press the same purchase key. It must preview again.
   Repeat after selecting an owned vehicle; changing selected ID invalidates
   any earlier authorization.
6. **Fail-closed context** — while driving, on Results/Settings, or during
   a career race whose Garage editing is locked, Q/U/I must not spend or
   expose an active confirmation promise.
7. **SaveGame failure** — with deliberately restricted test storage, run
   a second-press confirmation and verify that the existing rollback still
   restores credits, car ownership, and tune levels. Retest after relaunch.
8. **Viewport and input** — verify 900x600, 1280x720, 1920x1080 and a
   high-DPI monitor; inspect menu feedback on first and subsequent pager
   screens, plus Ctrl+digit quick-select and gamepad navigation. Confirm
   paused input and page changes cannot hide an actionable confirmation.

## Coverage and release boundaries

The production policy is
`Source/Latebrake/Public/Garage/GaragePurchaseConfirmation.h`.
`Preview(snapshot, now)` is const and fails closed on changed state,
negative/nonfinite/backward time or elapsed >8 seconds. C++20 regression
cases live in `Tests/Standalone/GaragePurchaseConfirmationTests.cpp`;
Unreal *source wiring* checks live in
`Tests/Editor/GaragePurchaseConfirmationBindingTests.py` and
`Tests/Editor/GarageCatalogBindingTests.py`.

These tests can establish deterministic policy and binding assertions, but
**not** Unreal 5.8 compilation, real-time Canvas painting while paused, a
cooked executable, actual input focus, or robust persistence on real disks.
Track the separate platform acceptance gate in
[#462](https://github.com/sanskarIN/latebrake/issues/462).
