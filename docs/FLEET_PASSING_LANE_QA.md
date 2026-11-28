# Experimental fleet passing lanes

The opt-in six-car virtual race has a steering-only lane candidate for a
slower physical opponent. All five blockers must be observed. Default
modes retain the established policy.

The candidate uses authored straight coordinates, padded footprints and
a constant-velocity swept interval. It reserves the entire lateral
translation corridor, including rear and side traffic. It refuses short
straights, unqualified road evidence, malformed bodies and near corners.
Leader and lane remain fixed through confirmation and the bounded
manoeuvre. Retirement is latched until accepted gate progression.
Existing route braking, immediate traffic, predictive traffic, yaw and
spin guards still run afterward. Traffic brake restores original steer.
No score, position classification, extra throttle or gate is added.

Native qualification requires an Unreal build and installed game:
observe both long straights with actual slower rivals and nearby traffic,
abort during lane intrusion, pause and recovery. Verify brake priority and
that no lane change occurs near corners. The constant-velocity envelope
is not a proof of actual physics clearance.

Passing confirmation also requires distinct consecutive GameMode and engine
frames, the same race attempt, session epoch and actor teleport revision.
The swept envelope includes measured lateral drift in addition to the
planned translation, and all bounding corners must fit the authored slab.
