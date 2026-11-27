# Shared footprint sweep

The short and longer fleet traffic forecasts now share a continuous
translation-only separating-axis interval. Recovery retains its stricter
single moving obstacle speed bound. Two moving cars can have twice that
relative speed without making a distant, nonintersecting lane a false
obstacle. Invalid inputs still fail closed.

The immediate guard evaluates all verified blockers and orders actual
padded overlap, earliest future contact, strongest following cap and
stable slot. It cannot grant additional propulsion, create steering
targets, alter race progress or report true Chaos contact.

Tests cover axis intervals, rotation, exact edge and horizon contact,
relative speed, recovery compatibility, all 120 five-blocker orderings,
and randomized exchange symmetry. Native playtests must assess actual
turning, collisions, queue discharge and false-positive braking.
