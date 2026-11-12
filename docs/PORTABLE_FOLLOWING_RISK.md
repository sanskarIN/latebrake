# Portable following-risk envelope

\`PortableFollowingGapEnvelope.h\` is an engine-independent, numerical
**diagnostic**, not a live AI braking algorithm. It combines measured bumper
clearance, speed difference, declared stopping capability and reaction distance.
It cannot infer real Chaos contact, friction, steering, a continuous track
wraparound or a lane-changing path from one scalar lane station.

The strict C++20 test checks stopped overlap, constant-speed following,
rapid closing, lane separation, exact failure of nonfinite and out-of-order
observations, finite stopping bounds and sensitivity to actual brake/reaction
parameters. All readings are metres, seconds and metres/second.

No Unreal player/rival runtime calls this module yet. **Merging it does not
change vehicle braking or establish tested six-car AI**. Native integration
requires direct real-world road projection, full swept occupancy geometry,
contact telemetry, deterministic recovery and real UE 5.8 hardware QA.
