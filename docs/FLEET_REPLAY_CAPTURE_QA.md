# Experimental six-car recent replay capture

The authoritative GameMode opens an assembly only after active-clock
advance. All six actual pawn ticks must submit the same-frame body,
measured velocity, heading, control demand and original asphalt evidence.
No missing sample is filled from the prior frame.

The complete assembly enters the existing bounded timeline and event log.
Incomplete or discontinuous evidence cannot be interpolated through.
A fresh complete segment follows pause, recovery, skipped frames or
impossible motion. The latest 480 samples of that segment are retained.
Finish freezes the last complete window before race permission is revoked.
The Results menu shows actual sample coverage and highlight candidates;
it does not install a native camera or move physical race actors.

Native acceptance requires the installed Unreal game. Verify original
road hits, six real actor samples, clock continuity, pause and recovery,
finish with an incomplete frame and unchanged normal race modes. Control
values are sampled demands, not measured forces. Observed traffic swaps
are not official race verdicts.
