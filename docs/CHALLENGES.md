# v0.5.0 standalone challenges

All six challenge types are implemented in the portable
`CareerChallenges.h` evaluator. Each is based on the existing four
ordered checkpoint gates; no additional physical track exists.

| ID | Name | Condition | Bronze/Silver/Gold target |
| --- | --- | --- | --- |
| 6 | Clock Hunter | One-lap time (s) | 160 / 130 / 100 |
| 7 | Clean Circuit | Two laps; maximum recoveries | 2 / 1 / 0 |
| 8 | Gate Guardian | Two ordered laps, time (s) | 300 / 255 / 210 |
| 9 | Lap Consistency | Three laps; slowest minus fastest (s) | 24 / 15 / 8 |
| 10 | Kestrel Mastery | Two laps using owned Kestrel, time (s) | 320 / 275 / 230 |
| 11 | Long Haul | Six ordered laps, time (s) | 1100 / 950 / 800 |

Time targets are **initial experimental balance values**, not measured
gameplay achievements. A player can complete a race without earning a
medal if the objective is missed. Clean and precision results use the
player's actual ordered-checkpoint and recovery data, never rival telemetry.

Invalid / nonfinite time or lap values, interrupted race, malformed vehicle
ID, mismatched lap count, negative recovery count and wrong vehicle are
rejected. Kestrel must be unlocked through in-game earned credits first.

Replay can improve a stored medal/series points, but first-completion
challenge credits are paid **once** only. No money, external services,
pay-to-win or network connection is required.

The C++20 tests validate each challenge category. Unreal physics, handling,
timing and practical target difficulty still need engine/device QA.
