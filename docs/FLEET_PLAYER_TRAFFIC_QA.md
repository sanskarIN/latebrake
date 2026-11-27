# Experimental fleet player traffic observations

The six-car experimental source candidate now provides a read-only
player-relative proximity advisory from the five real rival bodies.
It does not apply a brake, steering correction, extra throttle, gate,
time penalty, score, reward, collision count or save.

## Live feedback

- A fresh complete physical roster is required; partial, replaced, inactive
  or malformed observations do not imply clear traffic.
- Relative direction uses the actual player yaw and eight local octants.
  Centre distance and radial closing speed use real sampled XY positions
  and velocities, not the car's next checkpoint or an invented trajectory.
- The advisory distinguishes nearby, approaching, projected padded path
  overlap and current padded footprint overlap. The latter two are
  conservative planar estimates; they do not prove a collision.
- All five rivals are evaluated, including those behind the player.
  Priority is current padded overlap, projected overlap, approaching,
  nearby; projected-path ties favor the earliest sampled forecast.
- The live HUD shows at most one strongest alert from the fresh roster.

## Results history

The journal observes once after the actual accepted race-clock update.
It records only contiguous intervals whose endpoints agree on the same
physical rival's condition. Attempt, roster, engine frame, local frame,
session epoch, all actor teleport revisions and fee-free clock continuity
must agree. Pause, a hitch, invalid physics or recovery breaks continuity.
The first fresh sample establishes an endpoint rather than claiming the
preceding interval. Any-rival totals count the union only once.

The final snapshot reports sampled time, time with advisory warnings,
projected padded path overlap time, closest observed centre distance and
maximum simultaneous warnings. These are not actual collision durations
or measured minimum bumper clearance. Unsampled time is deliberately
not extrapolated.

## Required native acceptance

1. Build the exact experimental branch using the licensed engine and UHT.
2. Run the installed game in the opt-in six-car mode.
3. Approach rivals ahead, beside and behind the player; rotate the car
   and verify local direction labels. Check that off-road but real rivals
   remain represented and that no alert claims an actual collision.
4. Compare two closing rivals and a third near the player. Verify alert
   priority and that the HUD does not interfere with control or steering.
5. Pause and resume, recover the player, and observe a rival recovery.
   Verify no stale advisory crosses invalid roster or inactive frames.
6. At finish confirm the Results sample coverage is bounded by active
   race time and restarts clear the previous attempt.
7. Check default single-rival, solo and career modes remain unchanged,
   and inspect compact/expanded HUD and Results text clipping.
8. Savegame, credits and race classification must be unaffected.

Portable C++ and source-binding checks are not native gameplay evidence.
