# Rival accepted-checkpoint gap — source QA

**Status: source-development feature only.** This is not a playable
Unreal Engine 5.8 game, real measured race placement, official finishing
classification, wheel-to-wheel physical order, player reward or lap
timing comparison. Real Unreal/UHT compilation, cooked circuit and
installed driving QA remain outstanding.

## Why the HUD needs a gate-based comparison

A player and a rival can be metres apart but on **different sections
or laps** of the same circuit. Direct radar separation is not an
overtake, race ranking or timing gap. The existing independent rival
tracks accepted, *forward directional gate-plane crossings* through its
`CheckpointMotionTrace`, followed by
`RivalRouteProgress::ObserveVerifiedGate`. The player has separate
authoritative `RaceProgress` accepted gate counts.

The new read-only `RivalGateGap.h` compares only their credited gates:

```text
player gates = completed player laps * event gate count + next player gate
rival gates  = completed rival laps  * event gate count + next rival gate
gap          = player gates - rival gates
```

A positive gap means **the player has been credited with more forward
gates during this event**, not that the player is physically ahead on
the road. A negative gap means more credited rival gates. Equal gate
counts mean only that the drivers have accepted the same number of
gates. They could be anywhere between two gate planes.

Examples on the prototype four-gate, three-lap circuit:

| Player accepted | Rival accepted | Optional HUD interpretation |
| ---: | ---: | --- |
| 6 | 3 | `GATE GAP: YOU +3 VERIFIED (NOT RACE PLACE)` |
| 3 | 6 | `GATE GAP: RIVAL +3 VERIFIED (NOT RACE PLACE)` |
| 6 | 6 | `GATE GAP: SAME VERIFIED COUNT (NOT RACE PLACE)` |
| 0 | 0 | Hidden until at least one gate has been credited |

## Live eligibility

The GameMode returns no comparison if there is no spawned circuit
rival (including Time Attack), unready map/grid/player/rival physics,
unstarted race, paused/menu/focus-lost race, finished player race, or
a mismatch between player/rival configured gate count or required laps.
Invalid, negative, oversized or contradictory finished lap/gate
counters are rejected by the portable policy.

The optional readout is **desktop expanded HUD only**, after real
radar samples with a valid rival position, with sufficient vertical
telemetry space. It does not consume compact Android touch controls
or displace the authoritative lap, gate and race clock rows.

No location interpolation, apparent metre difference, extrapolated
speed, synthetic finish time, credits, saved results or AI route command
ever enters this comparison. Rival recoveries retain their previously
accepted gate count rather than awarding free gates.

## Tests (source only)

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R RivalGateGap --output-on-failure
python -m unittest discover -s Tests/Editor -p RivalGateGapBindingTests.py -v
```

Portable strict Windows/MSVC, Ubuntu/GCC and macOS/Clang checks cover
lap rollover, reversal, finished races, same-count ambiguity, 256-gate
and 100-lap bounds, invalid physics/event flags, invalid gate
counters, and 200,000 deterministic comparisons. Source binding
checks confirm that the rival progress comes from accepted directional
gate samples, the GameMode uses its existing player gate authority,
and the HUD shows no fabricated race ranking.

## Required real Unreal Engine 5.8 QA

1. On a licensed UE 5.8 Windows workstation compile Editor/Game with
   actual UHT and C++ API checks. Save/cook the original owned circuit;
   retain exact commit, engine, map and package fingerprints.
2. Install a genuine Win64 executable, drive both player and physical
   rival through every forward gate, and compare visible gate gap with
   independently recorded accepted gate events.
3. Verify reverse gate crossings, near-miss gates, opponent recovery
   attempts (success/failure), player paid recovery, start lights,
   pause/focus loss, race restart, missing rival, Time Attack, and
   player/rival completed laps. None may invent extra credited gates.
4. Verify Android compact layouts and low-height Windows viewports
   retain timing and touch controls with no optional gate gap row.
5. Confirm that career rewards, race completion, best laps and official
   save data remain unchanged by this observation-only feature.

Keep [playable release gate #148](
https://github.com/sanskarIN/latebrake/issues/148) and
[Android device gate #13](
https://github.com/sanskarIN/latebrake/issues/13) open until actual
installed gameplay evidence is reviewed.
