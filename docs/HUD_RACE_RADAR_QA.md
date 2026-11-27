# Responsive live race radar

The live desktop radar now has a protected right-hand lane and uses the
actual aspect ratio of the authored starter circuit. The map projects
the four original asphalt slabs and four barrier footprints, rather
than drawing only a square outline. Native car and gate markers still
come from verified physical observations. Off-map values are omitted;
glyphs and heading ticks are clipped at the edge. Single-rival and
experimental fleet contexts remain separate.

Actual font measurements are used to select non-overlapping label
positions. Labels that cannot fit are omitted while their physical
markers remain visible. Group masks cannot duplicate or invent a rival.
The live and replay north-up projections now agree, including yaw.

Native QA: resize from 1120x600 to ultrawide, verify the radar is clear
of text and lower instruments; check all four corners, pit symbol,
heading and clustered rivals, invalid evidence, pause and Results.
The original layout and touch controls remain independent. The map
does not determine official placement, physics or scoring.
