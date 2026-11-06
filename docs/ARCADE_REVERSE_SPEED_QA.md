# Arcade reverse-speed limit — original reference QA

> **Current draft upgrade:** [Physically bounded reverse overspeed](REVERSE_OVERSPEED_FORCE_QA.md)
> replaces the original real-vehicle `SetPhysicsLinearVelocity` cap
> with a genuine rear-contact/grip-limited Chaos force. The legacy
> instant-clamp function below is retained as a *portable reference*,
> not the actual draft pawn's present drive path. A force-based reverse
> target is soft and may be exceeded transiently after impacts; see
> the new QA procedure. The released v0.5.93 remains source-only.


This milestone makes reversing behave more like a low-speed gear on
LateBrake's existing rigid-body car. **It is not a new gearbox, tire solver,
transmission, or verified playable Windows/Android feature.**

## Historical rigid-body integration (before force governor)

The earlier `ALatebrakeCarPawn::Tick` implementation first checked the authoritative
race clock, start gate, local player/controller state, valid rigid-body
physics, valid frame timing and live pavement sample. The forward speed cap (including the human-only nitro speed-cap scale)
was then applied normally. **Afterward**, the pawn read actual Chaos
linear velocity and current heading and invoked the original instant
`latebrake::LimitPlanarReverseSpeed` reference policy. **The current
draft pawn no longer invokes this function or sets measured XY velocity
during accepted reverse driving.** See the linked bounded-force QA instead.

Both the locally driven prototype and its physically simulated AI rival
previously shared that instantaneous rule. Its pure reference tests remain,
but the current draft instead validates a grip/contact-limited force and
rejects invalid optional driving without overwriting valid chassis momentum.

## Original hard-clamp reference values (not the draft pawn)

| Behavior | Arcade | Simulation Inspired |
| --- | ---: | ---: |
| **Reference hard cap** / current **soft speed target** | 1,350 cm/s (48.6 km/h) | 1,000 cm/s (36 km/h) |
| Existing forward speed cap (before garage tuning/nitro) | 6,500 cm/s | 5,400 cm/s |
| Sideways sliding velocity | Unchanged by reverse cap | Unchanged by reverse cap |
| Vertical Chaos velocity | Unchanged by reverse cap | Unchanged by reverse cap |
| Normal forward travel | Unchanged by reverse cap | Unchanged by reverse cap |

The original reference used signed *measured longitudinal velocity relative
to the actual horizontal car heading*, not whether reverse was pressed.
Under that historical rule a collision's backward excess was immediately
replaced while retaining lateral velocity. The new physical policy instead
reacts continuously to overspeed with force; short collision overshoot is
possible, but it never teleports XY velocity. The separate forward/nitro
planar governor still applies independently.

Garage top-speed upgrades do not affect the reverse-gear cap, and it cannot
increase nitro or drafting acceleration or modify money, career, lap times,
checkpoints, saved state or collision-damage accounting.

## Source checks

The original standalone C++20 reference regression includes **200,000
deterministic heading and velocity combinations** and checks the old pure
projection, perpendicular component, mode-specific targets and invalid
inputs. The NEW separate `ReverseSpeedGovernor` strict test covers 100,000
force policy cases and 30/60/120 FPS non-teleport settling.
It is registered on Windows, Ubuntu and macOS in the strict C++ CI suite
with warnings as errors. The current Editor-source bindings verify the new force uses the true
velocity and contact data, participates in lateral grip, never rewrites
accepted-frame measured XY/Z momentum and is submitted to Chaos only on
an otherwise qualified frame.

## Actual UE 5.8 acceptance tests still required

On an authorized Unreal Engine 5.8 Windows workstation:

1. Compile the genuine Editor/UHT and game targets, author/cook the original
   LatebrakeCircuit map, then package a real Development Win64 candidate.
2. Drive forwards at full throttle and nitro: no new forward acceleration
   cap or hard stop should appear; steering/slide control should remain
   unchanged under the original overall speed cap.
3. Reverse on a flat straight in both handling modes. Observe and record
   the **soft** target near 48.6 / 36 km/h while checking for absence of
   velocity jumps; instantaneous hard compliance is not guaranteed.
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
