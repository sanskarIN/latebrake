# Grip-constrained world-scale pilot composition (source-only)

The existing future-circuit `WorldScaleCandidateRivalPilot` continues
to use the original dry source pace. The new
`WorldScaleConservativeGripPilot` wraps **that existing controller**,
and may only lower its speed ceiling, remove throttle or add a brake
when the separately qualified variable-grip plan requires it.

**No Unreal actor, Chaos tire dynamics, physical road measurements or
playable multiplayer simulation are created by this class.**

The adapter contract requires matching nonzero session identity,
current measured body contact, verified physical road identity,
finite race timestamp not earlier than the source friction snapshot,
and a fresh snapshot no more than **750 ms** old. Missing any input
produces the existing zero-power braked decision and resets steering
continuity rather than pretending to have dry grip.

The wetness constraint uses the measured source-only pace at both the
current projected station and the pilot's actual lookahead point.
Steering, heading errors, independently tracked lap position and the
original rival command's reverse/spin overspeed protections are never
modified. A wet limit cannot release a dry pilot's already commanded
brake.

Strict CTest `WorldScaleConservativeGripPilot` compares every one of
512 dry-station control decisions against the unchanged controller,
verifies wet grip cannot add power or change steering, and tests
physical contact failures, mismatched epochs, timestamps, stale
source evidence, reverse driving and 30/60/120 Hz start-finish travel.

Before binding this to any actual AI vehicle, native UE5.8 integration
must supply genuine friction, contact, plan lifetime, actual actor
kinematics and independently checked race-phase observation. Native
braking and tire-force authority must remain in Chaos, not this
portable advisory. Source CI does not establish playable performance.
