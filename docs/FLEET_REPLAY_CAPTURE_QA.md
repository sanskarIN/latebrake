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

## Discontinuous finish fallback

The recorder retains at most one earlier playable segment on a segment boundary. The latest playable window is preferred. If recovery or a missing sample leaves only a single-frame fragment at finish, Results can select the previous contiguous segment, explicitly labelled earlier. No samples or highlights are joined across a boundary. An ineligible finish discards both windows, and reset clears the archive. Foreign or duplicate admissions and foreign finish callbacks cannot close the current pending frame. Test recovery on the last gate, two successive discontinuities, and restart after fallback in installed Unreal before qualification.
