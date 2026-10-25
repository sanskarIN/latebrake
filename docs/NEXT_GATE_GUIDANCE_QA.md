# Next-gate bearing guidance — v0.5.60 source development

This is a **navigation hint only**, not evidence of passing a checkpoint.

`CircuitRouteAdvisor.h/.cpp` reads the actual player position and yaw,
the next checkpoint index from authoritative `RaceProgress`, and the
existing immutable four-gate circuit coordinates. It returns:

- Direct planar meters from the car to the selected checkpoint.
- Approximate left/right/straight/turn-around bearing to that checkpoint.
- An approach flag inside a direct 18m interval for compact HUD visibility.

The hint fails closed for a missing/raced-finished/paused player, invalid
gate, off-pavement position or nonfinite pose. It wraps yaw at ±180° and
has a near-target special case so a zero displacement is never treated
as a meaningful steering heading.

These are **direct-line distances**, not a physically measured along-road
distance, corner apex, GPS path, collision-safe instruction or projected
lap finish time. The compact Android HUD suppresses guidance unless
approaching a gate to keep controls readable. No new scoring decisions,
teleports, penalties or awards are made by this source.

Portable tests: `Tests/Standalone/CircuitRouteAdvisorTests.cpp`
links real `Private/Track/CircuitRouteAdvisor.cpp`, tests the original
start distance of 53.5m, all four gate positions, corner direction
signs, yaw wrapping, off-track rejection and NaN/inf suppression.

For licensed engine QA, drive the real circuit clockwise and
counterclockwise, verify the bearing reflects the actual next scored
gate and never advances early, confirm direct distance decreases on
appropriate approaches, and test compact UI after viewport changes.
Physical Win64/Android gameplay tests and cooked map still required.
