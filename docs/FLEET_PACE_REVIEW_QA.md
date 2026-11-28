# Whole-run experimental fleet pace

Only already-admitted gate receipts on the shared active clock feed this
attempt-local history. Up to 100 completed laps per car are retained even
when the recent matched-gate ring wraps. Partial laps do not change the
completed-lap sector statistics. The first lap includes the standing start;
recovery time can remain in a duration. No clean lap or record is claimed.

The results snapshot compares the same completed lap ordinals for each
rival. Mean, median, best, worst, spread and signed differences are derived
from that shared population. A full-fleet mean comparison requires a
shared prefix with all five independently verified rivals. Sector means
are compared only when completed-lap coverage is equal. A sum of best
observed sectors is not a completed lap. Values within 0.25 seconds
remain in an uncertain comparison window. No classification or award is
produced.

This is a bounded C++ source feature. Native acceptance requires a real
Unreal build, running a multi-lap fleet race, comparing sampled lap clocks
and the frozen Results panel, verifying missing evidence is withheld,
and checking menu scrolling and compact layouts. The history is not
persisted or used by the driving controller.
