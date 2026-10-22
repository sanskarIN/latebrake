# Career mode — v0.5.0 source foundation

**Status:** Portable C++20 rules are implemented and compiled in GitHub Actions.
Unreal Engine 5.8 runtime classes, HUD rendering, pause/menu routing,
and packaged gameplay have **not** been built or playtested in Unreal.

## Scope

Latebrake still contains only **one experimental rectangular starter circuit**.
The career reuses its four ordered gates with 1–6 lap objectives. No additional
playable maps, finish rankings, competitive AI scoring, or traffic exist.

There are 12 fixed-ID events:
- Rookie Cup: 0 Rookie Qualifier (1 lap), 1 Rookie Sprint (2),
  2 Rookie Finale (3).
- Prologue Cup: 3 Prologue Clean (2), 4 Prologue Pace (2),
  5 Prologue Finale (4).
- Standalone: 6 Clock Hunter, 7 Clean Circuit, 8 Gate Guardian,
  9 Lap Consistency, 10 Kestrel Mastery, 11 Long Haul (6 laps).

Event IDs are persisted at fixed indices; do **not reorder** this catalog.
Append new IDs and perform an explicit migration for schema revisions.

## Unlocking and completion

- Event 0 and standalone Clock Hunter (6) are initially available.
- Each numbered championship round requires a bronze-or-better medal
  in the previous round; Prologue Cup begins after Rookie Finale.
- Other challenges use prerequisites declared in `CareerCatalog.h`.
- Results are accepted only from the real player's completed
  `RaceProgress` gate sequence, with finite positive time, valid lap spread,
  valid recovery count and approved vehicle.
- Bronze (3 points), silver (6), gold (10); best medal per event persists.
- `Locked`, `Available`, `Completed` and `Mastered` are derived
  from saved medals, not vulnerable UI name comparisons.
- A medal upgrade updates the best result/points but **never repeats**
  a first-completion credit grant.

## How to start a race (development UI)

- Press **N** / gamepad D-pad right to browse unlocked events.
- Press **M** / gamepad D-pad left to start the highlighted career event.
- Press **Tab** for the prototype paused menu, use PageDown/PageUp
  and Enter to navigate or select. Gamepad mappings are in DefaultInput.ini.
- Android source draws NEXT, CAREER and MENU BROWSE controls; it has
  **not** been verified on an actual handset.
- Press **3** or **4** to leave career for a normal quick race.
- Press **T** to restart. The selected career event is saved before
  the race; medals/credits are written only after an accepted finish.

## Result/reward semantics

The active event's start vehicle is recorded. Vehicle-required challenges
equip an already owned profile at race start, and results are refused if the
car changes before finishing. Wrong-order gates do not complete laps.

**Unreleased v0.5.2 source guard:** While a career event is running,
garage shortcuts **E** (select another owned car), **Q** (unlock car),
**U** (upgrade engine), and **I** (upgrade grip) refuse to alter the
vehicle or spend credits. This prevents mid-event tuning changes and
avoids invalidated vehicle-required results. The garage may be edited
again **after** a career finish or after switching to a quick race with
3/4. Restarting a completed career event re-engages the lock.

The restriction is checked with the same engine-independent RaceProgress
finish state used for awards. Portable tests exercise all four actions,
multiple laps, finish, restart, and quick-race behavior. Actual Unreal
input routing, HUD feedback, menu and SaveGame behavior still need
in-engine verification.
Recovery costs 5 seconds and is counted against clean objectives.
Career earnings are one-time first-medal credits plus a once-only
championship completion bonus. Quick race keeps its original bounded
race-completion earnings policy.

See [championships](CHAMPIONSHIPS.md),
[challenges](CHALLENGES.md), [save migration](SAVE_MIGRATION.md), and
[Android qualifications](TOUCH_CONTROLS.md).

## Release limitations

There is no verified Unreal build, authored cooked map, Win64/Android game
executable, or physical-device playtest. Portable CTest is not a substitute.
See [issue #2](https://github.com/sanskarIN/latebrake/issues/2) and
[issue #13](https://github.com/sanskarIN/latebrake/issues/13).
