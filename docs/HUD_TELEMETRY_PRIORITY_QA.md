# Live HUD telemetry priority

The old live HUD rendered each row immediately, so an early optional
diagnostic could exhaust space before a late warning. The new bounded
queue selects explicit semantic categories and draws through the existing
actual font fit. Unfittable text consumes no row. Essential rows take
precedence; save, pause and finish status retain their own baselines.

Desktop Settings F2 cycles Focused, Balanced and Detailed. Balanced is
the session default; Focused retains essential timing and advisories;
Detailed admits existing diagnostics within the viewport budget. This is
a presentation preference, not a saved profile setting. Existing touch
geometry, driving controls, physics, credits and official time are unchanged.

Native QA: test a narrow and wide desktop window, all three modes, actual
late wrong-way and traffic alerts, track-limit and recovery notices,
start countdown, pause, finish and save warning. Check Android controls
remain clear. The portable suite is not native font/DPI acceptance.
