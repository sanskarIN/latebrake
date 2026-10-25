# Rival corner anticipation — v0.5.60 source development

LateBrake's source-level rival originally computed throttle/steering
from a single next waypoint. When approaching a 90° course corner
with little current heading error, it could retain high throttle until
the final late segment of that corner.

`RivalCornerAnticipation.h/.cpp` now consumes the existing
`PlanRivalTurn` output, the next waypoint index, actual rival position
and actual Chaos planar speed. It checks the original circuit gate
predecessor/next-successor geometry and requests **brake + zero throttle**
within approximately 22m of a known corner only if measured speed
exceeds a smoothly range-dependent target. It preserves steering,
existing main-planner braking, waypoint arrival and the previously
verified finite recovery budget and lap tracker. Bad gate index, NaN,
infinite speed or malformed commands fail closed to no throttle.

This policy is shared by both quick Circuit and eligible Career circuit
events that spawn the rival; Time Attack intentionally has no rival.

Tests compile the actual `RivalCornerAnticipation.cpp`, exercise all four
right-angle bends, far/slow approaches, preservation of existing braking,
and invalid input guards. The Unreal-source binding test requires the
real `ALatebrakeRivalCar::Tick` to feed output to
`SetAutonomousControls` in the same tick.

### Unverified physical behavior

A compiled policy **cannot** prove that actual Chaos rigid-body steering
will follow the intended racing line, that braking distance suffices,
or that rival traffic and player collisions are fair. Real UE 5.8
compilation, authored map, installed Win64/Android testing and time
series recordings are required before describing the opponent as
playable or competitively balanced.
