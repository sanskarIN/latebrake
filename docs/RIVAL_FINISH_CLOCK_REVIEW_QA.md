# Unofficial quick-race rival finish-clock comparison — source QA

**Status: Unreal source integrated; NOT engine/runtime/hardware-qualified.**
The existing Unreal Engine 5.8 single-player Quick Race Circuit
contains one autonomous rival with its own physically verified ordered
checkpoint trace. Previously, Results could only observe whether the
rival's route was already complete at the player's verified finish.

This source update additionally captures the rival's accepted final
checkpoint time on the same local **active post-GO clock**. After a
**recordable** player finish, it can show an *approximate observation*:

- `UNOFFICIAL: RIVAL FINAL GATE OBSERVED X.XXs EARLIER` when the
  stored rival final gate was over 0.25 seconds earlier on the sampled
  active clock.
- `UNOFFICIAL: FINAL GATES TOO CLOSE TO TIME` for a nonnegative
  difference of 0.25s or less. This avoids interpreting the order of
  two engine tick callbacks as a certified side-by-side finish.
- No margin when the rival has not finished, the player finish timing
  is invalid, the clock is malformed, or the comparison is ineligible.

**No declared winner/rank/placement.** Unreal actor tick order, discrete
physics samples and local clock granularity make the observation
insufficient to declare an official winner. The current career race,
Time Attack, credits, championship medals, best laps, official clock,
player gate acceptance and SaveGame do not depend on this value.
The separate rival-route-completion message remains available.

## Integrity and lifecycle

- `RivalFinishClockReview.h` is a pure allocation-free C++20 policy,
  configured using the actual race checkpoint count and required laps.
- The rival observer runs only after its **own forward directional
  gate trace** and matching split policy have accepted the gate;
  recording additionally requires its **route-complete state**.
- Rival and player timestamps remove *player recovery penalty fees*
  from the race elapsed value to compare a common active physical
  session clock, **not** a penalty-adjusted official race outcome.
- Player completion is checked with `ValidateFinishTiming()` before
  capturing Results. The comparison requires Quick Race Circuit,
  a separately verified physically ready rival, exact matching gate
  count and legitimate final player ordinal.
- Race initialization/restarts reset the private rival timestamp and
  Results snapshot. Duplicates, backdated clocks, wrong event sizes,
  nonfinite values, out-of-range time and stale callbacks cannot
  overwrite a prior accepted final gate.

## Source-level verification

The standalone `RivalFinishClockReviewTests.cpp` exercises valid
observed margins, close-call uncertainty, invalid/before-final player
states, repeated rival callbacks, corrupted clocks, wrong gate counts,
seven-day upper bound and reset isolation on strict portable C++20.

`UnrealRivalFinishClockBindingTests.py` asserts that GameMode and
Results actually consume this policy in the intended order and cannot
change award or save authority.

Both are **source-only** tests, not physical race tests.

## Genuine engine acceptance still required

1. Compile with real licensed Unreal Engine 5.8/UHT and package Win64.
2. Drive Quick Race Circuit against a physically moving single rival.
   Observe the rival cross its final directional gate first and compare
   the displayed margin with captured session and race audit logs.
3. Test a close final-gate crossing: the UI must present uncertainty.
4. Confirm solo Time Attack has no rival comparison and careers retain
   their previous independent scoring.
5. Restart, pause/reconnect, recover, and change race/event: old finish
   observations must not persist.
6. Confirm results layout, pagination and localization on installed
   Windows/Android builds before shipping.

See [playable-release gate #148](https://github.com/sanskarIN/latebrake/issues/148).
