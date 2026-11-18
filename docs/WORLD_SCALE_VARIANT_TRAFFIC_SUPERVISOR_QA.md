# Source-only six-car traffic supervisor

This pure C++20 arbitration layer consumes the independently qualified
**same-session/same-frame** readouts from the world-scale variant rival
pilot and the six-body continuity audit. It does not spawn Unreal actors,
drive cars, change vehicle collision, grant overtake/lap/penalty credit,
or implement network traffic replication.

Production file: `Source/Latebrake/Public/AI/WorldScaleVariantTrafficSupervisor.h`.
Test file: `Tests/Standalone/WorldScaleVariantTrafficSupervisorTests.cpp`.

The source checks a complete six-slot, uniquely tokenized roster with
consistent finite speeds, original road support, signed unwrapped source
positions, matching pilot actor, source circuit, race session and frame.
It compares forward projected track gaps and measured lateral separation.
A rival in a different lane or a revolution behind is not incorrectly
considered the current lane's next collision threat.

For a relevant forward rival, the planner calculates conservative
clearance including two longitudinal car half-lengths, body clearance,
reaction travel and relative-speed squared braking distance. It will
either pass the pilot's original bounded decision unchanged, cap
following throttle, or demand an advisory brake and zero throttle.
It **never increases** an existing throttle demand, changes steering,
releases an existing pilot brake, or creates live control authority.

Invalid inputs, missing car, duplicate actor, stale or mismatched
frame/session, nonfinite measurements, unsupported road contact,
invalid settings and illegal command requests fail closed to a
zero-throttle braked receipt with no valid source authority.

## Reproducible test

```sh
cmake -S Tests/Standalone -B build/traffic -DCMAKE_BUILD_TYPE=Release
cmake --build build/traffic --config Release --target latebrake_world_scale_variant_traffic_supervisor
ctest --test-dir build/traffic -C Release -R '^WorldScaleVariantTrafficSupervisor$' --output-on-failure
```

The regression covers independent six-circuit signed lap geometry,
clear road vs same-lane following, 500cm emergency gap, separated
lanes, rear traffic, reverse-oncoming traffic, two conflicting
vehicles, retained original brake and rejected invalid source
provenance. The tests use synthetic, explicitly source-only receipts;
they do **not** prove live Chaos contact or six actual UE bodies.

Native integration still requires Unreal 5.8 Editor and UHT proof,
real six-body physics snapshots, verified tire-road contact,
contact-patch width and stopping distance, actual collision/safety
supervisor precedence, driver assistance, real graphics and Win64
driving qualification.

This PR remains a draft stacked on PRs #405–#407 while protected
main retains its source-only v0.5.100-test.2 manual release gate.

## Atomic six-car arbitration

`Source/Latebrake/Public/AI/WorldScaleVariantFleetTrafficArbitration.h`
provides a single **all-or-nothing** admission boundary for all six
rival pilot receipts. The six candidate pilots must independently
match their exact real actor, session, source circuit and frame against
the same qualified fleet snapshot. Only then are the six independent
conservative traffic demands evaluated. If any one pilot is invalid,
the complete frame returns six invalid, zero-throttle, braked receipts;
no partial advisory can leak to a controller.

This keeps the physical-identity trust contract separate from corner
braking, authentic 3D collisions and online race arbitration. It does
not make an Unreal actor or live brake control available. The strict
`WorldScaleVariantTrafficSupervisor` regression also exercises
six-circuit atomic all-or-nothing rejection for one bad actor token,
one bad track/session/frame, one malformed pilot command, a missing
physical contact or a discontinuous fleet source.
