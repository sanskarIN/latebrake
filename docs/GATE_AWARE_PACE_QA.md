# Gate-aware live finish estimate

This is an engine-neutral read-only advisory bound to the existing Unreal
HUD source, not a native build or a guarantee of a driver's finishing time.

After a complete fee-free, actively timed reference lap exists, use its
actual remaining sector timings rather than subtracting elapsed time from
an average whole lap. Accepted checkpoint progress and current fee
attribution must reconcile with the authoritative race clock. Charged fees
are already in that clock; they do not satisfy a reference driving duration.
If an unfinished sector has consumed its reference time, withhold the
finish estimate until the next accepted gate. Do not fall back to a
whole-lap estimate when a measured prefix is invalid.

## Portable and native acceptance
- Registered LiveRacePaceProjection C++20 tests cover 1-256 gate counts,
  uneven sector timing, charged fees, early and late gates, finish/reset,
  fee-only and zero-clock prefixes and the bounded race clock.
- Source contracts check the actual HUD and absence of race mutations.
- In a licensed Unreal Win64 build, run multiple laps, compare the gate
  labels and recovery fees, wait past a reference sector, then finish that
  sector and confirm the estimate resumes. Test narrow and expanded HUD.
- Confirm no change to awards, official lap clocks, race state or profile.
- Native compile, map cook and device driving are separately required.
