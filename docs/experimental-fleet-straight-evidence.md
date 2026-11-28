# Experimental fleet: same-straight evidence

The experimental HUD now supplements accepted directional gate counts with a
limited road-distance comparison for cars that have exactly the same accepted
gate ordinal on the original four-gate circuit.

The calculation reuses the engine-neutral two-car straight projection.
It does not estimate progress around a turn. A car near a gate, outside the
central lane, off pavement, or missing independently verified route evidence
is excluded. Finished vehicles do not receive a live straight comparison.

The displayed counts describe comparisons with the human, not a six-car
classification. "Close" uses the existing three-metre tolerance. The distance
is along the straight centreline rather than a Euclidean gap, time delta,
or estimated overtake. The existing gate count, timing, and physical radar
remain separately available. No scoring, AI decisions, replay, rewards,
collision handling, or player controls are changed.

Standalone verification covers all four circuit segments, opposite directions,
near-corner and off-lane rejection, missing gate evidence, and finished races.
Native verification still requires the actual Unreal game and six registered
cars in motion; source tests do not prove HUD fit or runtime driving behavior.
