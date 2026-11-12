# Advanced race engineering analytics — source-only QA

**Milestone: 10,092 newly audited, qualifying C++ source lines.**
This draft extends LateBrake's verified telemetry foundation (PR #343)
without changing that pinned parent or the protected main branch.

The tracked-source audit of this branch measures **35,257** qualifying
production C++ lines, versus **25,165** in the parent. **164,743**
remain before the separate 200,000-line v0.6.0 source-quantity gate.

This is an engine-neutral, run-local C++20 *observability* feature.
It does **not** change any live Unreal car, actor, official lap clock,
checkpoint credit, reward, AI throttle or steering, career/save data,
published executable or installed Windows game. Real UE5.8/UHT and
Chaos QA remain separately required.

## Source components

These are 22 distinct original C++20 modules in the
Source/Latebrake/Public/Telemetry directory:

1. TelemetrySessionRecorder.h — verified actor/session ring recorder,
   bounded samples, reset receipts, downsample and fault suspension.
2. TelemetryReplaySampler.h — same-epoch and same-segment frame
   interpolation (pose, yaw, four-wheel data, controls). No writeback.
3. TelemetrySignalSeries.h — time-weighted signal history, interpolation,
   integral, variance, threshold occupancy and crossing counts.
4. TelemetryLapJournal.h — run-local forward-gate-qualified lap evidence,
   sector duration verification, incomplete/invalid lap protection.
5. TelemetrySectorMatrix.h — per-sector distribution and drift trends,
   confidence thresholds, complete theoretical-best sector sums.
6. TelemetryLapComparison.h — observed lap/sector deltas and explicit
   mixed-conditions versus comparable-conditions grading.
7. TelemetryDistancePace.h — distance-bin speed, throttle, braking,
   line deviation, slip and force observations.
8. TelemetryBrakeTrace.h — physical brake attack/hold/trail/release,
   lock time, heat exposure, kinetic loss estimate and history.
9. TelemetryRaceLineProfile.h — track-center lateral offset, road margin,
   stable line variance and confidence from multiple physical samples.
10. TelemetryCornerSequence.h — entry/apex/exit linked to observed brake
    events and read-only line/exit performance review.
11. TelemetryFuelStint.h — only actual verified fuel-sensor observations,
    depletion/refuel continuity and reserve forecasting.
12. TelemetryTireStint.h — four-corner physical tire heat, wear, contact,
    measured slip and time-to-wear estimates.
13. TelemetryWeatherTrend.h — physical wetness/friction/temperature
    trends, source validation, rainfall and sensor discontinuity.
14. TelemetryPitReview.h — evidence-qualified pit scenario comparison;
    cannot queue any pit work or alter a player's race.
15. TelemetryVehicleBalance.h — four-wheel normal-load/axle grip and
    handling balance observations, no physics replacement.
16. TelemetrySuspensionReview.h — genuine per-wheel compression,
    topping/bottoming, normal load and contact loss diagnostics.
17. TelemetryTransmissionReview.h — observed gear transition, RPM,
    clutch, shift shock, overspeed and transient history.
18. TelemetryIncidentJournal.h — real source contact/off-track episodes,
    duplicate suppression, severity estimates with no official penalty.
19. TelemetryFleetGapReview.h — exact six-member real actor identity
    checks and relative progress/gap trends, not official race order.
20. TelemetryDriverCoach.h — human-review guidance conditioned on
    actual physics and source evidence; never commands a vehicle.
21. TelemetrySessionIntegrity.h — source/session/actor/clock, physics,
    motion and duplicate-frame integrity report.
22. TelemetryReplayBookmarks.h — bounded replay event markers, source
    timestamp windows, clip metadata, no exported fake video.

## Official source-line verification

Run from the checked-out branch root:

    python3 scripts/MeasureSourceScale.py --json

The authoritative metric is game_source.nonblank_noncomment_lines.
It counts unique tracked game C++ source under Source and excludes all
tests, documentation, comment/blank lines, generated outputs,
third-party/vendor code and exact duplicate game-source files.

- Parent PR #343: **25,165** qualifying C++ lines.
- Current branch: **35,257** qualifying C++ lines.
- New development: **+10,092** qualifying C++ lines.
- Requested minimum: **+10,000** qualifying C++ lines.
- Remaining to v0.6.0 source quantity milestone: **164,743** lines.

A numerical gate is not a measure of art quality, installation,
licensed Unreal compilation or whether the game is fun/playable.

## Portable CI and regression gates

The native-independent C++20 suite uses strict MSVC /W4 /WX and Clang
or GCC -Wall -Wextra -Wpedantic -Werror. Run the entire suite:

    cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
    cmake --build Tests/Standalone/build --config Release --parallel 4
    ctest --test-dir Tests/Standalone/build -C Release --output-on-failure

The new separate TelemetryAdvancedAnalytics and
TelemetryExtendedSubsystem CTests include and compile all 22 modules.
They cover replay ring rollover, invalid times/actors, lap sector
evidence and weather comparisons, track bins, physical pedal traces,
4-wheel tire/wear, finite suspension changes, gearbox changes, actual
contact duplicates, six unique fleet actors, conservative coaching
and immutable replay bookmarks. The earlier RaceEngineerTelemetry
CTest retains its own deterministic 30,000-frame motion sweep.

All positive physics fixtures in source CI are synthetic *unit-test
inputs*. They test algorithms, not a genuine instrumented Chaos car.

Python source-contract tests: 

    python3 -m unittest discover -s Tests/Editor -p 'TelemetryAdvancedAnalyticsBindingTests.py' -v

## Licensed engine work remains separate

The owner must validate original UE5.8 Editor/UHT and a true Windows
Win64 installed build using exact-head, main-targeted, trusted-runner
workflow permissions. The pinned earlier PR #341 is a separate
candidate and must not be changed to accommodate this draft.

A future real native telemetry integration needs original verified
actor identity, actual wheel contact and normal load, real tire
temperature/wear, actual fuel levels and pit infrastructure. An
unavailable sensor must display as missing; never synthesize values
to falsely present them as engine measurements. Keep telemetry
read-only and avoid changing race authority.

Do not tag or publish a new pre-release based on source LOC or portable
CI. Keep issue #148 open until verified installed-game testing.
