# Two offline championships (v0.5.0 source)

All rounds are timed challenges on the **existing single starter circuit**.
The experimental rival may drive alongside the player in select rounds,
but its approximate route-lap value is NOT a competitive finish ranking.

| Championship | Round IDs | First completion bonus | Max medal points |
| --- | --- | ---: | ---: |
| Rookie Cup | 0, 1, 2 | 350 in-game credits | 30 |
| Prologue Cup | 3, 4, 5 | 550 in-game credits | 30 |

- Bronze = 3 points, silver = 6, gold = 10 for a round.
- Points always derive from the best persisted medal, not the count of
  races or an AI opponent's approximate waypoint progress.
- Round 1 is available when its prerequisite is satisfied; consecutive
  rounds unlock only after a bronze-or-better finish. Prologue requires
  completion of Rookie Finale.
- The three-round championship bonus is awarded exactly on the
  transition from an incomplete series to a complete one. Subsequent
  rounds/replays do not award it again.
- All medals and points survive SaveGame v2 migration; old profiles retain
  their garage/timing records without retroactively fabricated career wins.

This is source-implemented logic covered by C++20 portable CI. The Unreal
adapter and device-side scoring UI are **not engine-validated**. See
[career](CAREER.md), [save migration](SAVE_MIGRATION.md), and
[issue #2](https://github.com/sanskarIN/latebrake/issues/2).
