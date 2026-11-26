# Fee-free lap pace Results and measured split evidence

Source-only development; no installed Unreal gameplay or renderer qualification.

The Results menu compares only actually recorded, fee-free, actively timed
laps in the retained eight-lap window. Median absolute deviation is the
median of absolute distances from the sample median, not standard deviation.
Tied extremes retain the newest physical lap. Adjacent comparison never
bridges penalized laps. A single clean lap has no variability; no clean
laps or incomplete attribution suppress the report. Official times, rewards,
record eligibility and saves are unchanged.

Public measured gate comparisons now require a complete valid reference
and a valid actual prefix, even if the selected gate pair looks plausible.
Already-validated internal race observations retain constant-time gate
comparison; full lap reports validate traces once.

## Native acceptance

- Finish a multi-lap race with two fee-free laps and one charged recovery.
- Verify actual lap identities and retained counts against the existing Results.
- Confirm no adjusted paid time is displayed as a clean personal best.
- Check compact, expanded and paginated Results at 900x600 and 1920x1080.
- Restart; verify previous results disappear and profile rewards are unchanged.
- Test actual UE5.8/UHT and cooked Win64 before claiming native qualification.

Existing registered C++ targets RecentLapPaceStatistics and
MeasuredLapSplitReference cover the pure policy and invalid trace cases.
Python ResultsInsightsBindingTests checks actual HUD source linkage.
