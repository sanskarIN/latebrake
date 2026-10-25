# Racing events and autonomous rival — v0.2.0

## Event definitions

The pure C++20 `Racing/RaceEvents.h` catalog defines:

| Event | Laps | Gates per lap | Autonomous rival |
| --- | ---: | ---: | --- |
| Circuit Race | 3 | 4 | Experimental one-car AI |
| Time Attack | 1 | 4 | Disabled |

Player keys `3` and `4` switch events, reset the player's race and separately track best completion time for each event. Credits are earned offline on a completed event, with an additional clean-driving bonus. A recovery during a run adds a five-second time penalty and forfeits the clean bonus.

## Pilot implementation

`AI/RivalPilot.h` contains a deterministic, testable steering planner. It consumes position, heading, speed and a target waypoint; outputs steering, throttle, braking and arrival status. It rejects nonfinite sensor inputs and bounds steering commands. The Unreal `ALatebrakeRivalCar` actor supplies its world-state and applies computed commands to the same starter rigid-body pawn system as the player. The initial loop uses four fixed waypoints, intentionally matching the starter circuit.

The rival is **not yet** a properly qualified competitive opponent. Its cornering, braking distance, obstacle handling, AI race progression, recovery, avoidance and overtaking are untested; the player completion result is independent of the AI's progress. Do not advertise full racing-AI competition or multiplayer.

## Test and gameplay QA

Portable CTest suites validate basic planner controls, angle wrapping, invalid numbers, event settings, reward calculations and previous race invariants. Unreal integration still needs manual checks for spawn, despawn, physics response, collision, laps, UI and saves. Only the **local player's pawn** can trigger their own checkpoint scoring.

The roadmap anticipates more sophisticated path following, original vehicle rigs, traffic and race-aware controllers in later pre-releases once hardware performance and engine behavior have been measured.

## v0.2.1 stabilization

A portable `RivalRouteProgress` object tracks ordered waypoint approaches, approximate completed loops and low-speed stalls. Eight seconds of low-speed telemetry requests a recovery, with a bounded recovery counter. The Unreal rival uses that request to teleport to a lane-aligned point near the next target and preserve its approximate route history. The pilot has anticipation braking for close waypoints; the car base class uses a shared deterministic steering filter.

**Important:** waypoint loops remain approximations of AI route completion. Rival rank, valid gate crossings, finish time and head-to-head competition are not implemented. The latest portable CTest and the Unreal in-editor gameplay must be evaluated independently.

## Unreleased v0.5.2 — event-aligned rival route duration

The previous experimental rival route always stopped after three approximate
waypoint laps, even when a selected championship round required one, two or
four laps. `ALatebrakeRaceGameMode::RefreshRival()` now configures the
rival with the **actual selected `RaceProgress` checkpoint count and lap
target** after it spawns or reuses the rival actor. `RestartRace()` clears
route position, approximate lap count, stuck timer and recovery history
without reverting to the old three-lap default.

The expanded desktop HUD displays `RIVAL ROUTE completed/required LAPS`
so the value is not mistaken for the number of laps in a quick race.
The `Route` value is **approximate waypoint telemetry**, not a
head-to-head finishing order, valid gate crossing or automatic race reward.
The player's authoritative `RaceProgress`, medals and save records are
unchanged.

Portable C++20 `RivalRouteTests` now checks the route length and final
waypoint for **every career event which actually spawns a rival**, plus
one-event replay resets, stuck-recovery reset and out-of-range configuration.
Real UE physics, steering/cornering, opponent competitiveness and
installed-game display behavior remain unverified.


## v0.5.49 source development — checked rival recovery outcomes

The route counter is explicitly **attempts**, not successes. Each stalled
recovery request now becomes pending until the actual Unreal
`TryRecoverTo` verifier reports placement. `RivalRouteProgress`
separately counts verified and rejected engine outcomes; duplicate or
unsolicited acknowledgements cannot alter counters, and a pending
attempt cannot advance approximate waypoint or lap telemetry. Both
failure and success consume one of the same finite recovery budget slots.

The non-compact HUD distinguishes attempted from verified and rejected
rival teleports. These values do not modify the player-authoritative
race result or qualify the experimental AI as competitive. See
[Rival recovery QA](RIVAL_RECOVERY_QA.md) for portable tests and required
real Unreal/device validation. This change is **unreleased source work**.


## v0.5.60 — original-circuit corner anticipation

The prototype rival still uses `PlanRivalTurn` for steering/arrival.
`AnticipateRivalCorner` now examines the current gate and the original
four-gate predecessor/successor geometry. Inside a close approach to a
known 90-degree turn it can request brake and zero positive throttle
when actual planar speed exceeds a continuously distance-dependent
target. The policy cannot move waypoints, mark laps, revise the route's
finite verified/rejected recovery audit or modify player credits.
Invalid sensor readings fail closed.

The decision is compiled and tested against the real C++20 policy in
`RivalCornerAnticipationTests.cpp` (all four bends), with Unreal-source
integration checks. The change is not a claim that the AI will physically
take the bend correctly in Chaos. See
[RIVAL_CORNER_QA.md](RIVAL_CORNER_QA.md) for real engine and device tests.
