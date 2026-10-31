# Arcade reverse-speed limit — source QA

This milestone makes reversing behave more like a low-speed gear on
LateBrake's existing rigid-body car. **It is not a new gearbox, tire solver,
transmission, or verified playable Windows/Android feature.**

## Real in-game integration

The shared `ALatebrakeCarPawn::Tick` routine first checks the authoritative
race clock, start gate, local player/controller state, valid rigid-body
physics, valid frame timing and live pavement sample. The current forward
speed cap (including the human-only nitro speed-cap scale) is still applied
normally. **Afterward**, the pawn reads the actual Chaos linear velocity
and its current actor heading, then calls the engine-neutral
`latebrake::LimitPlanarReverseSpeed` policy.

Both the locally driven prototype and the physically simulated AI rival use
this same pawn and cap. Any invalid direction or non-finite velocity fails
closed through the existing motion-neutralization policy.

## Expected handling

| Behavior | Arcade | Simulation Inspired |
| --- | ---: | ---: |
| Maximum **backward** component | 1,350 cm/s (48.6 km/h) | 1,000 cm/s (36 km/h) |
| Existing forward speed cap (before garage tuning/nitro) | 6,500 cm/s | 5,400 cm/s |
| Sideways sliding velocity | Unchanged by reverse cap | Unchanged by reverse cap |
| Vertical Chaos velocity | Unchanged by reverse cap | Unchanged by reverse cap |
| Normal forward travel | Unchanged by reverse cap | Unchanged by reverse cap |

The reverse limit acts on signed *measured longitudinal velocity relative to
the actual horizontal car heading*, not whether the reverse key is pressed.
If a collision pushes the car backward too fast, that backwards component
is also limited. Lateral velocity is retained so slides are not silently
erased. The normal overall forward/lateral speed cap is still authoritative
and may affect slides independently; this reverse-only rule adds no speed.

Garage top-speed upgrades do not affect the reverse-gear cap, and it cannot
increase nitro or drafting acceleration or modify money, career, lap times,
checkpoints, saved state or collision-damage accounting.

## Source checks

The standalone C++20 regression includes **200,000 deterministic heading
and velocity combinations** and asserts bounded reverse speed, preservation
of the perpendicular velocity component, unchanged ordinary forward motion,
the stricter simulation mode, and fail-closed nonfinite/degenerate inputs.
It is registered on Windows, Ubuntu and macOS in the strict C++ CI suite
with warnings as errors. Editor-source bindings verify the call occurs
**after** the normal speed cap, uses the actual chassis velocity, and
preserves the original Z component.

## Actual UE 5.8 acceptance tests still required

On an authorized Unreal Engine 5.8 Windows workstation:

1. Compile the genuine Editor/UHT and game targets, author/cook the original
   LatebrakeCircuit map, then package a real Development Win64 candidate.
2. Drive forwards at full throttle and nitro: no new forward acceleration
   cap or hard stop should appear; steering/slide control should remain
   unchanged under the original overall speed cap.
3. Reverse on a flat straight in both handling modes. Observe and record
   actual chassis speed against **48.6 / 36 km/h**, allowing appropriate
   numerical tolerance.
4. Reverse with steering and handbrake, on road and off pavement. Confirm
   the car remains steerable and sideways chassis velocity is not replaced
   by this reverse policy; verify no 3D vertical jolts.
5. Have the AI rival reverse during its own valid drive session if
   appropriate; confirm it cannot bypass the same cap.
6. Test interruption, restart, teleport, focus loss, crash and abnormal
   physics. Capture the true runtime log and check the private candidate
   receipt. Repeat actual installed Windows/Android hardware QA.

**No licensed Unreal compile, actual game driving, cooked map, or installed
device QA is claimed by this source change.** The project remains
source-only until [playable-release gate #148](
https://github.com/sanskarIN/latebrake/issues/148) is satisfied.
