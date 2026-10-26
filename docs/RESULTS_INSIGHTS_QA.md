# v0.5.71 Results insights — source and installed-game QA

## Player-facing source work

The genuine Unreal Results menu is extended with **three independent,
presentation-only systems**:

1. **Recent timed-lap statistics** analyze the last 2–8 retained,
   actively timed, valid completed laps. Average, median, within-window
   minimum/maximum, population standard deviation and signed newest
   versus window mean are computed. A strict decreasing-time streak
   counts only consecutive adjacent physical lap comparisons. Once the
   fixed-size eight-lap ring wraps, these metrics describe **only the
   remaining recent window**, not the full race history.
2. **Best-gate split review** displays the actual best positive time for
   each scoring gate within the run, together with the mixed-lap
   theoretical sum. Best splits may come from **different** physical
   laps; do not label the sum an achieved lap time or a new record.
3. **Sampled car telemetry** displays chassis speed-integrated distance
   (estimated, not an odometer), observed peak km/h, braking share of
   accepted time, brake activations and high lateral-slip sample time.
   It is not authoritative for official elapsed time or a skill grade.

The existing bounded Results pager shows these detail rows on desktop
and Android. The compact view suppresses optional secondary lines
rather than resizing the right-side touch pads or moving the warning
band. Nothing here alters physics, opponents, checkpoint scoring,
recovery fees, career rewards, settings, save schema, networking or
persistent records.

## Strict data-admission rules

- Both timing reviews require `CanAwardFinishedRace`: missing active
  timing, zero-time and penalty-only laps, incomplete finishes and
  record-invalid clocks must not produce premium results.
- Lap stats require two or more valid recent samples with contiguous
  physical lap IDs, newest equal to completed laps, and an exact window
  of min(completed laps, eight). No average silently incorporates laps
  already evicted from the bounded history buffer.
- Standard deviation is the **population** deviation over the retained
  sample; the signed newest-minus-average comparison treats positive
  values as slower. Equal adjacent laps break the faster-lap streak.
- The gate review requires finite positive per-gate observations and
  the exact theoretical best already computed by real `RaceProgress`.
  The source theoretical sum must agree and be no greater than the
  best achieved lap.
- The sampled driving review requires at least two accepted frames
  and >=0.25s accepted sample time. Counters, distances, speeds,
  sample-time shares and physics-derived fractions are bounded and
  checked for finiteness. Missing evidence displays no review.

## Portable regression tests

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

New strict C++20 targets compile **actual** `RaceProgress.cpp` or
`DrivingSessionStats.cpp` rather than writing a simulation of their
production state. Cases include three- and twelve-lap windows, sample
median/mean/deviation, faster/slower/equal streaks, true mixed-lap gate
bests, reset, missing/penalty-only/zero-time results, touch-screen
compact display binding, interrupted chassis samples, numerical
bounds and time-share calculations.

## Unreal and installed-game acceptance (outstanding)

1. Use a licensed Unreal Engine 5.8 installation; run UHT and build both
   editor and game targets. Source test CI is **not** this step.
2. Author/cook the real original circuit asset and test a three-lap
   event. Deliberately achieve one best sector on a slower completed
   lap and confirm the theoretical mix is never represented as an
   achieved record or reward threshold.
3. Verify recent-lap average, median, range and improvement streak
   against the actual engine timers. Repeat across a long (>8 lap)
   qualified event; older evicted laps must no longer influence stats.
4. Inspect paused and finished Results in landscape/portrait Android,
   narrow Win64 desktop sizes, high DPI and non-ASCII car names. The
   UP/DOWN detail pager, touch buttons and SAVE FAILED message must
   remain unobstructed.
5. Verify accumulated chassis telemetry during braking, cornering,
   normal pause/resume, recovery and restart. Understand speed-
   integrated distance is approximate and may omit invalid samples.
6. Retest changing input focus, menu category, player possession,
   physical package launches and save reload; no late/ghost data may
   survive explicit restart or wrong-run state.
7. Record actual editor compiler logs, build SHA, map/hash, device
   names/OS, screenshots, frame time, failures and package artifacts.
   Add physical Android proof to
   [qualification #13](https://github.com/sanskarIN/latebrake/issues/13).

No licensed engine test, cook, Win64 or Android game package, or
physical device session is claimed by the source suite. v0.5.71 is a
**published source-only pre-release with zero executable binaries**, not
a playable or stable release.
