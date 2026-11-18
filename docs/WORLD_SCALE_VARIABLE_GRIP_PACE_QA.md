# Verified-input variable-grip pace (future world-scale circuit)

**Research status:** the proposed 1.7 km circuit has an existing
source-only nominal pace planner. The new
`Track/WorldScaleVariableGripPace.h` adds a *separate optional*
closed-ring pace calculation whose speed limits and braking distances
can only become more conservative when independently sourced track
grip is lower than baseline.

## Explicit sensor contract

This module **does not manufacture or measure** friction. A future
trusted Unreal adapter must independently supply all 512 station
observations from the real circuit, with an actual road component,
fresh measured grip, source confidence, station identity and physical
contact provenance. One unknown, stale, physically unverified or
out-of-range station rejects the **whole** advisory plan. Historical
source-only synthetic values in CTest are geometry fixtures, not
evidence of authentic road measurements.

The source envelope restricts accepted grip to 0.50–1.00 of the
existing nominal source pace reference. It cannot raise vehicle
speed, grip, permitted tire force or braking authority above the
reviewed baseline, and cannot authorize a lap, overtake, pit strategy,
race record, penalty, or engine physics change. Grip below 0.50 is
not interpreted as safe: the model is outside its supported range
and fails closed. The model makes no dynamic hydroplaning claims.

## Numerical procedure

A valid 512-station frame produces a local curvature speed ceiling
using `sqrt(lateral_acceleration * grip / curvature)`, also bounded
above by the existing dry source target. Each directed station edge
uses the *minimum of its two measured endpoint grip fractions* to
scale the available braking and propulsion acceleration.

A monotone squared-speed relaxation over the **entire circular track**
enforces both backward braking and forward drive acceleration limits,
including the 511→0 start/finish edge. Iteration stops only at an
unchanged closed-loop solution (or fails closed if it cannot settle
within 512 complete passes). Every resulting speed, edge and
closed-lap geometric time estimate is independently rechecked.

Reference sampling interpolates **squared velocity**, not velocity
step-functions, so target speed is continuous across station
boundaries and the signed start/finish seam.

## Verification

```sh
cmake -S Tests/Standalone -B build/grip-pace -DCMAKE_BUILD_TYPE=Release
cmake --build build/grip-pace --config Release --target latebrake_world_scale_variable_grip_pace
ctest --test-dir build/grip-pace -C Release -R '^WorldScaleVariableGripPace$' --output-on-failure
```

The portable strict C++20 regression checks an unchanged uniform-dry
reference, slower uniformly reduced-grip case, wet patch spanning
start/finish, edge-by-edge braking and acceleration including the
seam, invalid single-station measurements, confidence, source age,
identities, unsupported low grip and 30/60/120 Hz continuous
sampling through negative and positive wrapped laps.

**Important:** this does not create 512 real sensors, change Chaos
friction materials, implement wet tires, improve actual AI racing
or qualify Win64 playability. The physical car's existing brake and
traction supervisors must remain authoritative. A separate trusted
UE5.8 native simulation and input provenance test is required before
any integration.

This is a draft branch only; protected main remains frozen at
`a92a4799bfd2e43de30089f1cb9c069012529d5c` for the
owner-approved manual `v0.5.100-test.2` source-only prerelease.
