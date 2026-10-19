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
