#include "Telemetry/TelemetrySessionRecorder.h"
#include "Telemetry/TelemetryReplaySampler.h"
#include "Telemetry/TelemetrySignalSeries.h"
#include "Telemetry/TelemetryLapJournal.h"
#include "Telemetry/TelemetrySectorMatrix.h"
#include "Telemetry/TelemetryLapComparison.h"
#include "Telemetry/TelemetryDistancePace.h"
#include "Telemetry/TelemetryBrakeTrace.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake::engineering;

void Check(bool accepted, const char* message)
{
    if (!accepted)
    {
        std::cerr << "TelemetryAdvancedAnalytics FAIL: "
                  << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

TelemetryFrame EngineFrame(
    std::uint64_t index, double time, double position) noexcept
{
    TelemetryFrame f;
    f.Identity.Trust = TelemetryTrust::EngineObserved;
    f.Identity.SessionToken = 1001;
    f.Identity.ActorToken = 505;
    f.Identity.FrameOrdinal = index;
    f.Identity.DriverSlot = 0;
    f.Identity.SessionRevision = 2;
    f.Race.EnginePhysicsReady = true;
    f.Race.Phase = TelemetryPhase::Racing;
    f.Race.LocalRaceClockRunning = true;
    f.Race.LapIndex = 0;
    f.MonotonicSeconds = time;
    f.RaceClockSeconds = time;
    f.DeltaSeconds = 0.05;
    f.Dynamics.WorldPositionCm = {position, -1800.0, 180.0};
    f.Dynamics.WorldVelocityCmS = {250.0, 0.0, 0.0};
    f.Dynamics.FuelFraction = 0.9;
    f.Dynamics.GripScale = 1.0;
    f.Dynamics.EngineRpm = 2400.0;
    f.Controls.Throttle = 0.5;
    f.SpeedometerKmH = 9.0;
    f.Environment.Surface = TelemetrySurface::Asphalt;
    f.Environment.OnPhysicalPavement = true;
    f.Wheels.Contact.fill(true);
    f.Wheels.NormalLoadNewtons.fill(3000.0);
    f.Wheels.TireTemperatureCelsius.fill(65.0);
    return f;
}

void TestRecorderAndReplay()
{
    TelemetrySessionRecorder<16> store;
    Check(store.Configure({}), "bounded source recording config");
    const auto first = EngineFrame(1, 1.0, 100.0);
    Check(store.Start(first), "verified physics starts a record");
    for (int i = 2; i < 25; ++i)
    {
        Check(store.Observe(EngineFrame(
            static_cast<std::uint64_t>(i),
            1.0 + 0.05 * (i - 1),
            100.0 + 12.5 * (i - 1))),
            "bounded physical actor captures frames");
    }
    Check(store.Size() == 16 &&
          store.Snapshot().TotalOverwrites == 8 &&
          store.Snapshot().WasEngineObserved,
          "fixed allocation recycles oldest frames with accounting");
    TelemetryReplaySampler<16> sampler;
    Check(sampler.Configure(&store, {}),
          "read-only replay can bind a recorded actor");
    const auto middle = sampler.Sample(1.55);
    Check(middle.Qualified &&
          middle.Frame.Dynamics.WorldPositionCm.X > 100.0 &&
          !middle.AuthoritativeGameState,
          "interpolated replay cannot grant game authority");
    const auto invalid = sampler.Sample(
        std::numeric_limits<double>::quiet_NaN());
    Check(!invalid.Qualified, "invalid replay time rejected");
    store.Stop();
    const auto receipt = AssessRecordingHealth(store.Snapshot());
    Check(receipt.EngineQualified &&
          !receipt.TrustworthyForOfficialTiming,
          "replay health never substitutes for official clock");
}

void TestSignalHistory()
{
    TelemetrySignalSeries<32> signal;
    for (int i = 0; i < 20; ++i)
    {
        const auto frame = EngineFrame(
            static_cast<std::uint64_t>(i + 1),
            1.0 + i * 0.05, i * 12.5);
        Check(signal.Observe(frame, SignalKind::Speed),
              "qualified read-only speed channel");
    }
    Check(signal.Size() == 20, "bounded sample count");
    const SignalSummary summary = signal.Summarize();
    Check(summary.Valid && summary.Mean > 240.0 &&
          summary.Mean < 260.0 &&
          summary.StandardDeviation < 1.0,
          "signal statistical summary matches measured speed");
    Check(signal.TimeAbove(200.0) > 0.5 &&
          signal.ThresholdCrossings(200.0, true) == 0,
          "threshold occupancy and crossing semantics");
    const auto midpoint = signal.Interpolate(1.225, 0.10);
    Check(midpoint.Qualified &&
          std::abs(midpoint.Value - 250.0) < 0.001,
          "time-interpolated channel is finite and source-derived");
}

LapJournalEntry MakeJournalLap(double multiplier)
{
    LapJournalEntry e;
    e.EligibleForPerformanceReview = true;
    e.SessionToken = 1001;
    e.ActorToken = 505;
    e.Lap.LapIndex = 1;
    e.Lap.Completed = true;
    e.Lap.SectorCount = 4;
    e.Lap.DurationSeconds = 40.0 * multiplier;
    for (int sector = 0; sector < 4; ++sector)
    {
        e.Lap.SectorTimes[static_cast<std::size_t>(sector)] =
            10.0 * multiplier;
    }
    return e;
}

void TestSectorComparisons()
{
    TelemetrySectorMatrix matrix;
    Check(matrix.Configure({}), "four-sector configuration");
    for (int lap = 0; lap < 8; ++lap)
    {
        LapJournalEntry entry =
            MakeJournalLap(1.0 + 0.01 * lap);
        entry.Lap.LapIndex = lap;
        Check(matrix.ObserveLap(entry),
              "engine-qualified independent sector observation");
    }
    const auto sector = matrix.Evaluate(0);
    Check(sector.Valid && sector.QualifiedSamples == 8 &&
          sector.BestSeconds == 10.0 &&
          sector.MeanSeconds > 10.0 &&
          matrix.BestTheoreticalLapSeconds() == 40.0,
          "sector quality follows real timing distribution");
    LapComparisonInput reference;
    reference.Lap = MakeJournalLap(1.0);
    reference.EngineQualified = true;
    reference.Vehicle = TelemetryVehicleClass::Touring;
    LapComparisonInput candidate = reference;
    candidate.Lap = MakeJournalLap(0.98);
    const LapComparisonReport report =
        CompareRecordedLaps(reference, candidate);
    Check(report.Valid &&
          report.Quality == LapDeltaQuality::Qualified &&
          report.OverallDeltaSeconds < 0.0 &&
          report.CandidateFasterSectors == 4 &&
          report.MostGainedSector >= 0 &&
          !report.AuthoritativeTiming,
          "lap comparison is advisory and preserves sector attribution");
    LapComparisonHistory history;
    Check(history.Push(report) &&
          history.Latest().Valid,
          "bounded lap comparison history stores complete report");
    candidate.Wetness = 0.8;
    const auto weatherChanged =
        CompareRecordedLaps(reference, candidate);
    Check(weatherChanged.Valid &&
          weatherChanged.Quality ==
              LapDeltaQuality::MixedConditions,
          "rain prevents claiming equal lap comparison conditions");
}

void TestDistancePace()
{
    DistancePaceProfiler<64> profiler;
    DistancePaceSettings settings;
    settings.BinLengthCm = 200.0;
    settings.MinimumSamplesPerBin = 1;
    Check(profiler.Configure(3000.0, settings) &&
          profiler.StartLap(1001, 505, 2),
          "bounded track-distance bin profile initialized");
    for (int i = 0; i < 30; ++i)
    {
        DistancePaceSample sample;
        sample.DistanceCm = i * 100.0;
        sample.ElapsedSeconds = i * 0.1;
        sample.SpeedCmS = 1000.0;
        sample.Throttle = 0.4;
        sample.Brake = 0.2;
        sample.OnRoad = true;
        sample.Qualified = true;
        sample.Sector = 0;
        Check(profiler.Observe(sample, 1001, 505, 2),
              "distance-domain motion reconstructs per-bin pace");
    }
    const auto summary = profiler.Summary();
    Check(summary.Qualified &&
          summary.RecordedSamples >= 20 &&
          summary.MaximumObservedSpeedCmS > 0.0 &&
          summary.MeanWeightedSpeedCmS > 0.0 &&
          profiler.Bin(1).Samples > 0,
          "distance-domain track review accumulates real positions");
}

void TestBrakeTrace()
{
    BrakeTraceRecorder recorder;
    Check(recorder.Configure({}), "brake trace thresholds");
    for (int i = 1; i < 48; ++i)
    {
        BrakeTraceSample sample;
        sample.TimeSeconds = i * 0.05;
        sample.TrackDistanceCm = i * 30.0;
        sample.SpeedCmS = 1100.0 - i * 5.0;
        sample.BrakePressure = i < 30 ? 0.78 : 0.0;
        sample.LongitudinalDecelerationG = 0.4;
        sample.BrakeTemperatureC = 400.0;
        sample.EngineQualified = true;
        sample.OnPavedRoad = true;
        sample.DeltaSeconds = 0.05;
        Check(recorder.Observe(sample),
              "observed pedal trace preserves distinct attacks and release");
    }
    Check(recorder.StoredEvents() == 1 &&
          recorder.AtNewest().Valid &&
          recorder.AtNewest().EndSpeedCmS < 1100.0 &&
          recorder.BrakingSeconds() > 0.7,
          "complete source brake trace is replayable and bounded");
}
}

int main()
{
    TestRecorderAndReplay();
    TestSignalHistory();
    TestSectorComparisons();
    TestDistancePace();
    TestBrakeTrace();
    std::cout << "Source-only telemetry analytics verified.\n";
    return EXIT_SUCCESS;
}
