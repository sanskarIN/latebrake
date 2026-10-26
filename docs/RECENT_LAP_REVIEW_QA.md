# Recent lap review — source-only QA (v0.5.70)

## What is implemented

The real Unreal Results page now displays the **eight most recent** positively
timed lap records (newest first) from the same bounded
`RaceProgress::GetRecentTimedLap` ring already used for race insights.
Each visible row contains the true physical completed-lap number and time.
Results also identify an **equal run best** or the nonnegative
seconds behind the fastest observed lap **in this run**. On desktop,
a second line shows signed pace change compared with the preceding stored
timed lap: positive means slower; negative means faster.

The menu's existing up/down detail pagination exposes rows that will not fit
in the safe on-screen Android/desktop lane. It does not remove, scroll,
or overwrite existing checkpoint, lap, reward, control, or save state.

## Hard correctness constraints

- All results are derived from the **existing** completed race. No synthetic
  opponent times, sector extrapolations, or saved all-time ghost records.
- If `CanAwardFinishedRace` fails (incomplete, missing active lap timing,
  non-recordable clocks), the review is hidden in its entirety.
- A zero-time or penalty-only lap cannot appear as a legitimate result.
- Each row's lap number must be positive, ordered newest-to-oldest, and no
  larger than the completed-lap count; corrupt data fails closed.
- Every accepted duration must pass `IsValidFinishedRaceTime`; negative
  best-lap gaps fail closed, rather than implying a faster result than the
  source-of-truth best.
- The oldest surviving record never shows an invented "previous" lap delta
  when its predecessor has fallen off the eight-lap ring.
- The review uses a fixed eight-element `std::array`. It does not allocate
  dynamic game-state storage or touch save schema, achievements, career
  credits, race clock, penalties, car physics, or rival policy.

## Source validation

On a portable C++20 toolchain:

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^(RecentLapReview|MenuDetailPager|FinishedRaceInsights)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*LapReviewBindingTests.py' -v
```

The `RecentLapReview` test compiles the **real production**
`RaceProgress.cpp` and verifies a 10s/8s/12s three-lap finish,
best-lap gaps, signed change, a single-lap finish, incomplete results,
penalty-only finishes, invalid zero-clock laps, reset and a twelve-lap
event with eight retained history records. Existing paged-menu tests keep
scrolling and narrow-viewport text behavior separate from racing rules.

## Unreal and hardware qualification still required

Portable compilation and source binding tests do **not** prove that
Unreal Engine 5.8 compiles or renders the HUD. On an authorized UE 5.8
installation, compile LatebrakeEditor/UHT and game targets, then:

1. Cook the authentic original circuit and package actual Win64/Android
   candidates. Record the exact binary/map/source fingerprints.
2. Complete a three-lap race on Windows. Open Results; verify real
   lap numbers, ascending/descending signed deltas and "RUN BEST".
3. Check a one-lap event and a longer event; the history must remain bounded
   to eight and never claim unavailable earlier timings.
4. Resize windows and rotate physical Android devices. Verify UP/DOWN pads
   access all details; clipped strings, touch pads, SAVE FAILED warnings,
   title and status remain legible and operable.
5. Try interrupted app focus/menu transitions and restart. Confirm results
   never leak across runs and no race authority changes on page navigation.
6. Log hardware, Unreal revision, test date and actual failures in
   [Android qualification issue #13](https://github.com/sanskarIN/latebrake/issues/13).

Do not ship as a **playable** release or attach fabricated executables.
The latest verified public pre-release remains source-only v0.5.65 until
a separate tag qualification successfully publishes a newer source snapshot.
