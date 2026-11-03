# Advisory same-segment rival progress — source candidate

The previous **credited-gate gap** HUD reports only accepted forward gate
crossings. When both drivers have exactly the same accepted gate total,
it cannot determine who is farther along the circuit. The new expanded
desktop HUD can make a *limited spatial comparison* on one verified straight:

- Both the actual player and rival physics actors must be live, the original
  circuit must be valid and racing, and the existing local race must be
  unpaused, past GO, with a valid player restart placement.
- The existing `TryGetRivalCreditedGateGap` must return the **same gate
  ordinal**, corroborated by the current next-gate index.
- Both world positions must be finite, paved, in the **center 500 cm** of
  the same current straight, and between **3.5% and 96.5%** of that leg.
  Turns, off-track shortcuts, missing actors and mismatched gates show
  no derived lead.
- Differences under **3 meters** show only the original same-gate message.
  Clearer differences display `SAME GATES | YOU/RIVAL +Xm STRAIGHT
  (NOT RACE PLACE)` on roomy desktop HUD layouts.
- This is a *source-only, read-only estimate* of along-straight distance;
  it is **not race placement**, lap credit, a reward, a finish result,
  an overtaking detection system, online ranking, or saved profile data.
  It does not write physics, course progress, records, results or credits.

## Source-level verification

`Tests/Standalone/RivalSameSegmentOrderTests.cpp` uses the production
portable C++20 policy and tests both lead directions, sub-3m ambiguity,
all four signed straight directions, lap rollover, invalid gates, off-road
positions, near-corner samples, nonfinite coordinates, race interruption
and 20,000 repeated frames. The CMake target builds with warnings-as-errors.

`Tests/Editor/RivalSameSegmentOrderBindingTests.py` checks that the real HUD
queries the physically sourced coordinates alongside the authorized
checkpoint counters, only in the expanded layout. This is source
inspection—not a compiled Unreal component test.

## Required real-world acceptance

On an authorized Unreal Engine 5.8 workstation: compile the actual Editor
and game targets, author/cook the original map and drive two cars around all
four straights. Confirm the advisory appears *only* after GO with real active
chassis contact, does not flicker into corner/off-track false leads, clears
on pause/menu/restart/recovery/focus loss and never changes race awards.
Capture installed Win64 video/log evidence before calling this qualified.
The public v0.5.90 is a source-only pre-release with **no verified Windows
game executable**. This change by itself is not a playable release.
