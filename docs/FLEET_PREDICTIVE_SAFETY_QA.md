# Experimental six-car predictive traffic safety — QA

**DRAFT, SOURCE-ONLY. No licensed Unreal Engine 5.8/UHT test, cooked map,
installed Win64 executable or measured collision-free driving is claimed.**

This separately stacked feature is based on the existing opt-in physical
six-car branch. It is **not** included in the frozen v0.5.98 source candidate
or the existing frozen #335 owner-controlled engine validation snapshot.
It does not replace or quietly merge #337's separate pre-GO ground checks.

## Objective

The original `RivalFleetTraffic` is a bounded, immediate short-term
physical-obstacle braking guard. At motorway-equivalent closing speeds,
a ~0.4-second swept collision window can be shorter than a conservative
reaction-plus-braking distance. Releasing maximum motor demand as soon
as one sample clears can also cause oscillation and unsafe acceleration.

This second layer observes exactly one physical AI body and five distinct
real blockers on *every qualified in-race frame*. It can only further
reduce already-authorized rival throttle or request braking. It has no
control over steering, physics impulses, car spawns, player motion, gate
progress, lap awards, penalty fees, credit balance or SaveGame files.

## Independent portable policies

- `RivalFleetCollisionForecast.h` validates six actor identity tokens,
  exact grid slots, all five physical blockers, and finite world-space
  body positions and velocities. Partial or duplicate rosters fail closed.
  **Actual footprint restriction:** each fully padded, rotated chassis
  must fit in one of the four original collision-bearing asphalt slabs
  without intersecting any authored wall. Paved *centre only* is no
  longer enough; a physical margin over road edges rejects propulsion.
  Because these slabs join as a union, the conservative one-slab proof
  can reject a legal diagonal junction pose. Verify real corner driving
  and adjust only on licensed engine evidence, never by assuming safety.
- Continuous separating-axis intersection checks all four body axes,
  constant relative measured velocity and conservative uncertainty
  padding for the next 1.4 seconds. It does not falsely assume vehicles
  stop, steer or rotate on command.
- A conservative 520 cm/s² *baseline* brake assumption, 0.24-second
  reaction allowance and 500-cm separation adds stopping-distance
  protection. The baseline is discounted by the **actual prior accepted**
  tire grip, service-brake heat, road traction and impact-damage policies.
  Missing/invalid observations reject predictive propulsion; no scale
  above 1.0 is admitted. This is **not** measured Chaos deceleration,
  physical ABS simulation or certified real-world braking.
- All five threats are inspected; physical overlap is higher priority
  than swept contact, which is higher priority than stopping-distance
  concerns. Among two lower-severity following risks, the smallest
  permitted throttle wins **regardless of actor observation order**;
  only otherwise-identical risks use stable actor-slot tie-breaking.
  No weaker cap may overwrite a stricter one when samples reorder.
- `RivalFleetRiskSupervisor.h` binds the decision to the current six
  actual actor tokens. Missing actors, malformed frame time or invalid
  driver output clear every old safe observation. Actor replacement
  requires a new conservative time-based reacquisition hold.
- Current danger brakes immediately. A 0.38-second emergency hold avoids
  flip-flopping on frame boundaries; a 0.95 throttle/s recovery cap
  prevents instant motor reapplication when actual traffic clears.
- These policies are gated behind the existing non-shipping
  `-LatebrakeExperimentalSixCarGrid` option. Single-rival gameplay uses
  its old logic and does not enter the new forecast.

## Source tests

```bash
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --parallel 4
ctest --test-dir Tests/Standalone/build -R '^RivalFleetRiskSupervisor$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'RivalFleetRiskSupervisorBindingTests.py' -v
```

Test scenarios include complete roster permutation, missing/duplicated
identity, anonymous actor, stale session, malformed velocities, the
unpaved centre, head-on and right-angle relative motion, false rear
braking, adjacent-lane separation, immediate chassis overlap, late
emergencies overriding earlier mild following caps, true stopping distance,
throttle invariants, frame-time rejection, actor replacement, restart
resets and 30/60/120 FPS hold consistency. A fixed-seed 30,000-frame
perturbation sweep validates no extra throttle or arbitrary steering.

## Real licensed Unreal acceptance before any merge

1. Freeze the **exact approved aggregated head** containing this new PR,
   with all six real actors. Existing frozen #335 lacks this feature.
   Review the complete diff before a trusted self-hosted UE5.8 build.
2. On original owner-authored cooked asphalt, compile Editor/UHT/Game,
   then drive a real six-car session at 30/60/120 FPS. Confirm honest
   tracker identity and no extra vehicles silently disappear.
3. Measure position, velocity, input and actual stopping behavior for
   frontal approaches, lateral crossings, high-closing-speed straights,
   slow side-by-side passes and near-wall traffic. Ensure route and
   checkpoint steering are not hijacked by the safety guard.
4. Destroy/replace a rival mid-session and verify the actual fleet
   identity sample rejects the prior roster and holds brake.
5. Compare the real CPU cost of up to five continuous swept OBB tests
   per rival on the actual target hardware. Verify bounded work and
   no hitch from the full physical six-car traffic collection.
6. Regress one-rival Quick Race, Time Attack, career, retry, pause,
   focus loss, recovery and full races. Verify default sessions do
   not enter the new experimental traffic path.
7. Keep issue #148 open until source, licensed engine, cooked map,
   real installed Win64 gameplay and platform-specific requirements
   have their own complete evidence.

The numerical v0.6.0 **200,000 nonblank/noncomment production-C++ line**
goal remains separate from gameplay correctness. Do not count tests,
documentation, workflow code, generated files or overlapping PRs.
