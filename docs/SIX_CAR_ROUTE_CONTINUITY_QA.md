# Six-car source-only route-continuity and relative-pass audit

**Development research, not integrated Unreal gameplay.** This portable
C++20 subsystem audits six independent measured racing chassis on the
proposed 1.70 km world-scale original circuit. It does **not** spawn
cars, modify physics, controls, gear shifts, racing line, collision
queries, lap counters, prizes or career/persistent state. No native
Unreal Engine 5.8 Editor/UHT, Win64 cook, installed driving evidence or
circuit asset was produced by this work.

## Why this matters

A future six-car racing game needs trustworthy independent vehicle
observations before any advisory gap or overtake reporting is useful.
A source-only algorithm should **not** treat a list of six names,
six static positions, a swapped actor identity or forged lap count as
proof of a genuine physical race.

`Source/Latebrake/Public/AI/SixCarRouteContinuityAudit.h` receives a
whole source-authenticated observation frame with six actual distinct
actor tokens, stable grid slots, trusted race session/ordinal,
world-X/Y positions and *independently measured* world velocities,
confirmed authored-road support and a wrapped lap-progress report.
The future real Unreal adapter must obtain those values from current
physical actors; this header does not provide such an adapter.

For every received frame, the advisory:

- Rejects a missing/duplicated sixth car, repeated slot, empty actor
  identity, nonfinite value, invalid timing, invalid road support,
  unreasonable speed, off-circuit position or route-distance mismatch.
- Projects each chassis onto the proposed world-scale centerline and
  compares the reported route progress with **independent** geometry.
  This candidate circuit projection is not an authoritative checkpoint.
- Requires identical actor identity, strictly consecutive frame
  ordinal, and physically compatible trapezoidal observed XY motion
  before accumulating the next frame's evidence. **Per-frame kinematic
  residuals are limited independently to 5 cm or 12% of expected travel,
  whichever is larger.** The looser 220 cm track-projection tolerance
  is NEVER reused as permission for a frozen body with a falsely moving
  speedometer to earn measured-source continuity. The independently
  projected circuit tangent also gives **signed forward velocity**:
  only physical longitudinal route motion can justify increasing
  reported lap distance. Reverse driving or a pure sideways slide must
  not be misreported as a completed forward overtake.
- Validates *unwrapped* route distance, including genuine lap transitions
  where `CompletedLaps` increments and wrapped progress returns to 0.
  Spoofed lap increments, unexplained jumps, backwards progress and
  teleports invalidate the full frame, not just one car's rank.
- Re-baselines after a pause, race epoch change, actor replacement or
  frame gap. It cannot award time that was not actually observed.
- Sorts the advisory positions from **independently projected physical
  chassis locations**, not the separately reported lap stations. Repeated
  small source-station overstatements cannot accumulate into invented
  passing events even when every individual frame remains within the
  conservative geometric and per-frame tolerances.
- Sorts the six advisory distances independently of input-array order
  and reports pairwise relative passes only after 35 cm of hysteresis,
  preventing small rank-tie jitter from producing repeated pass events.
  A relative pass is reported only when the newly leading chassis also
  has at least 0.5 m/s of measured velocity along the circuit's actual
  forward tangent; ordinary physical sideways drift remains permitted
  when real forward progress agrees with projected motion.
- Never outputs a "winner", official finish, pass bonus, race score,
  checkpoint award, safety intervention or throttle command.

## Deterministic test coverage

The new `Tests/Standalone/SixCarRouteContinuityAuditTests.cpp` target
runs with the existing strict `/W4 /WX` or
`-Wall -Wextra -Wpedantic -Werror` C++20 toolchain and requires:

- Six distinct actor IDs, independently measured velocities and
  physically projected road locations in every accepted frame.
- 30, 60 and 120 FPS continuity across 90 source-only frames,
  with the complete six-member input array deliberately permuted.
- Stable ordering and a single pairwise overtake even when the
  speed-sensitive crossing occurs across several observations.
- Independent frozen-world, forged-progress and moving-world/stale-route
  attacks at 30/60/120 FPS; neither a believable projected location nor
  the previous 180 cm fixed error budget may fabricate motion.
- Cumulative +7cm/frame fake route progress cannot award a relative
  pass while the true world-projected car remains behind; this check
  deliberately stays within existing per-frame drift tolerances.
- Reverse-motion and 90-degree sideways-sliding chassis with false
  forward progress reports must be rejected at 30/60/120 FPS, while
  a legitimate forward drift with moderate lateral slip stays valid.
- A true 1.70 km wrapped-lap crossing that remains continuous.
- Rejection of NaN/Inf, impossible speeds, lost road support,
  duplicated/missing identity, repeated slot, invalid lap boundaries,
  forged track progress and off-road source positions.
- Complete invalidation on single-car teleports and fake +1 laps.
- A new baseline after lost frames, actor replacement, new race
  session, pause and focus interruption.

The world-scale nearest-segment lookup visits approximately 512
segments **for each of six cars**. Its cost and projection ambiguity
must be measured and addressed before an actual per-tick integration.
No result may claim the roster is *physically drivable* merely because
these standalone tests pass. Real Unreal collision and overlap
checks must be added at the actual adapter boundary.

## Native UE5.8 / Win64 acceptance gate (NOT performed)

1. Obtain an authorized engine installation and build the entire
   original project, including UHT, Editor target, packaged game and
   an actually cooked original level.
2. Instrument real six-Car `UPrimitiveComponent` chassis positions,
   velocities, road support and stable actor identities. Reject stale
   or synthetic values before calling the source-only audit.
3. Prove distinct physical actors for player + five opponents with
   vehicle-sized footprints, obstacle collision and valid six-car
   start-grid/multi-car runtime spawn.
4. Verify 30/60/120 FPS in a **real** installed Win64 driving session:
   side-by-side passes, opposite-direction motion, restart, recovery,
   lap seam, overtakes, traffic interaction, pause and OS focus loss.
5. Confirm invalid observations cannot modify official checkpoint,
   lap, reward, penalty, results, race clock, career or SaveGame data.
6. Profile centerline projection and measure false alarms at actual
   bends, track intersections, drift slides and colliding cars.

**The currently published source-only pre-releases do not contain
this proposed feature until a separately approved merge/tag.**
The `v0.5.100-test.2` source candidate is frozen on the earlier
protected-main commit and must not silently include this new branch.
