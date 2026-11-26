# Near-footprint game AI merge response

The existing rival courtesy brake now intersects short-horizon measured
forward and lateral motion intervals for a nearby player entering the
rival's forward footprint, including near-equal longitudinal speed.
It is deliberately a local game-AI heuristic, not a physical collision
guarantee. It does not change steering, route, player controls, rewards
or saves. Existing brake hold and passing cancellation remain downstream.

Portable tests cover equal-speed inward merging, mirrored and rotated
coordinates, no simultaneous overlap, outward/parallel/rearward
motion, malformed velocity and bounded interval sweeps. Native acceptance
requires a licensed UE build and an actual single-rival race with a
player entering and leaving the rival lane, verifying no stuck braking
or unexpected steering and preserving ordinary corner and passing logic.
