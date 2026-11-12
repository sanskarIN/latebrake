# Experimental six-car terminal start-fault QA

**Draft developer-only source work. No Unreal Engine 5.8/UHT, original cooked
track, real six-car handling, or Win64 executable is claimed.**

This document covers a safety fix stacked on the six-car experiment
([PR #331](https://github.com/sanskarIN/latebrake/pull/331)) and the
real-fleet launch/player-recovery safeguards
([PR #332](https://github.com/sanskarIN/latebrake/pull/332)).

## Failure that this work eliminates

An older opt-in start policy applied the six-car countdown requirement
to **every event** when `-LatebrakeExperimentalSixCarGrid` was present,
including solo Time Attack and career races where extra rivals are not
allowed. It could indefinitely hold the legitimate solo/career green light.

The same policy handled a physically failed extra actor spawn by changing
`bExperimentalSixCarGrid` to `false`, which silently changed the
requested six-car experiment into an ordinary two-car start. That could
mislead a tester into believing a full-grid launch succeeded.

## New deterministic contract

- The developer opt-in flag is retained for the session; it is not
  disabled just because a specific spawn fails.
- The six-car countdown requirement applies **only** to the non-career
  Circuit event. Time Attack and career are independent of the flag.
- A failed `DontSpawnIfColliding`, failed real post-spawn `TryRecoverTo`,
  missing actor registration or invalid source grid geometry triggers
  **`bExperimentalFleetFaulted`** for that accepted attempt.
- A faulted experimental Circuit attempt is denied GO and **never**
  auto-respawns all four actors in an unbounded loop. It logs
  `EXPERIMENTAL_GRID_FAULT` for an operator.
- An explicitly accepted `RestartRace` clears the fault **after**
  successful career-replay preparation, destroys any prior extra actor
  identities and resets the start countdown. This is the only path that
  clears the fault. Restart cannot bypass an actual failed player placement.
- The GameMode uses `DecideExperimentalFleetStart`, a pure C++20
  decision helper with no time, movement, saving or score authority.
  The policy distinguishes an existing roster from physics-ready bodies
  and checks the physical-ready set again immediately after an
  authorized pre-GO spawn.

This code is for conservative source correctness; it cannot guarantee
real Unreal gameplay performance or prove a genuine chassis is stable.

## Source test commands

    cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
    cmake --build Tests/Standalone/build --parallel 4
    ctest --test-dir Tests/Standalone/build -R '^ExperimentalFleetStartPolicy$' --output-on-failure
    python3 -m unittest discover -s Tests/Editor -p 'ExperimentalFleetStartFaultBindingTests.py' -v

The standalone test exhausts **4,096 permutations** of request/event,
fault, roster, actor readiness, map, menu/pause, finish and start-state.
It also explicitly covers the default one-rival game, opt-in Circuit
waiting, full six-car GO, terminal error, accepted restart, solo/career
event transitions and denied late actor spawning.

## Real UE5.8 test before any merge/publication

1. Build UE5.8 Editor/UHT and Game from the exact pull-request SHA on
   the owner's isolated trusted Windows runner.
2. With the dev flag absent, test ordinary Circuit, career and Time
   Attack starts. Compare real countdown and timing to the prior build.
3. With the dev flag present, navigate directly into career and Time
   Attack. Confirm their countdowns and race clocks advance normally.
4. With the flag present in non-career Circuit, deliberately reject
   a physical extra-grid spawn or corrupt a post-spawn placement.
   Observe exactly one terminal error, no endless actor retries,
   **no green light** and no fabricated six-car result.
5. Issue a deliberate approved restart, restore valid physics,
   and confirm that a brand-new full roster may be assembled with
   distinct registered actors and real chassis readiness.
6. Repeat an *unsuccessful* career replay preparation and verify it
   does not clear a fault from a current race. Check focus loss,
   pause/resume, event switch, restart after finish, and actuation
   latency at 30, 60 and 120 FPS.
7. Verify the original published source-only v0.5.97 and pinned v0.5.98
   candidate do not include this still-unmerged experiment.

Keep [playable release gate #148](https://github.com/sanskarIN/latebrake/issues/148)
open. A successful CTest build is **not** a cooked, installed game, and
this stacked PR is **not** an authorization to release v0.5.98.
