#pragma once

#include "Telemetry/TelemetryLapAnalysis.h"
#include "Telemetry/TelemetryTrack.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class SectorObservationIssue : std::uint8_t
{
    None,
    MissingIdentity,
    InvalidNativeFrame,
    NoRaceClock,
    NoSectorTransition,
    BackwardsSector,
    SkippedSector,
    UnverifiedGate,
    InvalidDuration,
    EpochChanged,
    GapDiscontinuity,
    CapacityExceeded,
    InconsistentLap
};

struct SectorTimingSettings final
{
    int SectorCount = 4;
    double MinimumSectorDurationSeconds = 0.08;
    double MaximumSectorDurationSeconds = 1800.0;
    double MaximumSampleGapSeconds = 0.20;
    double MaximumSplitDisagreementSeconds = 0.25;
    double MinimumDistanceCm = 200.0;
    double MaximumDistanceCm = 2000000.0;
    double MinimumSpeedCmS = 0.0;
    bool RequireVerifiedGate = true;
    bool RequireClockRunning = true;
    bool RequireSameEpoch = true;
    bool RecordCautionSectors = false;
};

inline bool ValidSectorSettings(
    const SectorTimingSettings& s) noexcept
{
    return s.SectorCount >= 2 &&
           s.SectorCount <= MaximumTelemetrySectors &&
           Finite(s.MinimumSectorDurationSeconds) &&
           s.MinimumSectorDurationSeconds > 0.0 &&
           Finite(s.MaximumSectorDurationSeconds) &&
           s.MaximumSectorDurationSeconds >
               s.MinimumSectorDurationSeconds &&
           Finite(s.MaximumSampleGapSeconds) &&
           s.MaximumSampleGapSeconds > 0.0 &&
           s.MaximumSampleGapSeconds <= 1.0 &&
           Finite(s.MaximumSplitDisagreementSeconds) &&
           s.MaximumSplitDisagreementSeconds >= 0.0 &&
           Finite(s.MinimumDistanceCm) &&
           s.MinimumDistanceCm >= 0.0 &&
           Finite(s.MaximumDistanceCm) &&
           s.MaximumDistanceCm > s.MinimumDistanceCm &&
           Finite(s.MinimumSpeedCmS) &&
           s.MinimumSpeedCmS >= 0.0;
}

struct SectorPassage final
{
    int LapIndex = -1;
    int SectorIndex = -1;
    int GateOrdinal = -1;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    std::uint64_t FrameOrdinal = 0;
    double StartRaceSeconds = 0.0;
    double FinishRaceSeconds = 0.0;
    double DurationSeconds = 0.0;
    double DrivenDistanceCm = 0.0;
    double StartTrackCm = 0.0;
    double EndTrackCm = 0.0;
    double MaximumSpeedCmS = 0.0;
    double MinimumSpeedCmS = 0.0;
    double MeanSpeedCmS = 0.0;
    double BrakingSeconds = 0.0;
    double FullThrottleSeconds = 0.0;
    double SlipSeconds = 0.0;
    double OffTrackSeconds = 0.0;
    double LateralErrorIntegral = 0.0;
    double MaximumLateralErrorCm = 0.0;
    double DeltaToBestSeconds = 0.0;
    double DeltaToPreviousSeconds = 0.0;
    double DeltaToMeanSeconds = 0.0;
    double EstimatedLossSeconds = 0.0;
    int BrakeApplications = 0;
    bool VerifiedForwardGate = false;
    bool Qualified = false;
    bool CautionAffected = false;
};

struct SectorStatistics final
{
    int SectorIndex = -1;
    std::uint64_t Samples = 0;
    std::uint64_t CleanSamples = 0;
    double BestSeconds = 0.0;
    double SlowestSeconds = 0.0;
    double LastSeconds = 0.0;
    double MeanSeconds = 0.0;
    double VarianceSecondsSquared = 0.0;
    double BestCleanSeconds = 0.0;
    double MedianSeconds = 0.0;
    double MeanDrivenDistanceCm = 0.0;
    double MeanBrakingSeconds = 0.0;
    double MeanApexSpeedCmS = 0.0;
    double TotalLostSeconds = 0.0;
    double ConsistencyCoefficient = 0.0;
    bool HasReference = false;
};

class SectorTimeHistory final
{
public:
    void Reset() noexcept
    {
        Data.fill({});
        Latest.fill({});
        StoredSeconds.fill({});
        Counts.fill(0);
        Next.fill(0);
        Records = 0;
    }

    bool Record(const SectorPassage& event) noexcept
    {
        if (!event.Qualified ||
            event.SectorIndex < 0 ||
            event.SectorIndex >= MaximumTelemetrySectors ||
            !Finite(event.DurationSeconds) ||
            event.DurationSeconds <= 0.0 ||
            event.DurationSeconds > 1800.0)
        {
            return false;
        }
        const std::size_t i =
            static_cast<std::size_t>(event.SectorIndex);
        SectorStatistics& stats = Data[i];
        stats.SectorIndex = event.SectorIndex;
        if (stats.Samples == 0)
        {
            stats.BestSeconds = event.DurationSeconds;
            stats.SlowestSeconds = event.DurationSeconds;
            stats.MeanSeconds = event.DurationSeconds;
            stats.BestCleanSeconds =
                event.OffTrackSeconds <= 0.0 ?
                    event.DurationSeconds : 0.0;
        }
        else
        {
            stats.BestSeconds = std::min(
                stats.BestSeconds, event.DurationSeconds);
            stats.SlowestSeconds = std::max(
                stats.SlowestSeconds, event.DurationSeconds);
            const double oldMean = stats.MeanSeconds;
            const double count =
                static_cast<double>(stats.Samples + 1);
            stats.MeanSeconds +=
                (event.DurationSeconds - oldMean) / count;
            stats.VarianceSecondsSquared +=
                (event.DurationSeconds - oldMean) *
                (event.DurationSeconds - stats.MeanSeconds);
            if (event.OffTrackSeconds <= 0.0 &&
                (stats.BestCleanSeconds <= 0.0 ||
                 event.DurationSeconds < stats.BestCleanSeconds))
            {
                stats.BestCleanSeconds = event.DurationSeconds;
            }
        }
        stats.Samples = SaturatingIncrement(stats.Samples);
        stats.CleanSamples += event.OffTrackSeconds <= 0.0 ? 1U : 0U;
        stats.LastSeconds = event.DurationSeconds;
        stats.TotalLostSeconds += std::max(
            0.0, event.DurationSeconds - stats.BestSeconds);
        stats.MeanDrivenDistanceCm +=
            (event.DrivenDistanceCm -
             stats.MeanDrivenDistanceCm) /
            static_cast<double>(stats.Samples);
        stats.MeanBrakingSeconds +=
            (event.BrakingSeconds -
             stats.MeanBrakingSeconds) /
            static_cast<double>(stats.Samples);
        stats.MeanApexSpeedCmS +=
            (event.MinimumSpeedCmS -
             stats.MeanApexSpeedCmS) /
            static_cast<double>(stats.Samples);
        stats.HasReference = true;
        Latest[i] = event;
        constexpr int Capacity = 16;
        StoredSeconds[i][static_cast<std::size_t>(Next[i])] =
            event.DurationSeconds;
        Next[i] = (Next[i] + 1) % Capacity;
        Counts[i] = std::min(Counts[i] + 1, Capacity);
        std::array<double, Capacity> sorted{};
        for (int n = 0; n < Counts[i]; ++n)
        {
            const int index = Counts[i] == Capacity ?
                (Next[i] + n) % Capacity : n;
            const double seconds =
                StoredSeconds[i][static_cast<std::size_t>(index)];
            int j = n;
            while (j > 0 && sorted[static_cast<std::size_t>(j - 1)] >
                             seconds)
            {
                sorted[static_cast<std::size_t>(j)] =
                    sorted[static_cast<std::size_t>(j - 1)];
                --j;
            }
            sorted[static_cast<std::size_t>(j)] = seconds;
        }
        const int middle = Counts[i] / 2;
        stats.MedianSeconds = Counts[i] % 2 ?
            sorted[static_cast<std::size_t>(middle)] :
            0.5 * (sorted[static_cast<std::size_t>(middle - 1)] +
                   sorted[static_cast<std::size_t>(middle)]);
        const double variance =
            stats.Samples > 1 ?
                stats.VarianceSecondsSquared /
                static_cast<double>(stats.Samples - 1) : 0.0;
        stats.ConsistencyCoefficient =
            SafeRatio(std::sqrt(std::max(0.0, variance)),
                      std::max(stats.MeanSeconds, 0.01));
        Records = SaturatingIncrement(Records);
        return true;
    }

    SectorStatistics Snapshot(int sector) const noexcept
    {
        if (sector < 0 || sector >= MaximumTelemetrySectors)
        {
            return {};
        }
        return Data[static_cast<std::size_t>(sector)];
    }

    SectorPassage LastPassage(int sector) const noexcept
    {
        if (sector < 0 || sector >= MaximumTelemetrySectors)
        {
            return {};
        }
        return Latest[static_cast<std::size_t>(sector)];
    }

    double BestReference(int sector, bool cleanOnly) const noexcept
    {
        const SectorStatistics s = Snapshot(sector);
        return cleanOnly ? s.BestCleanSeconds : s.BestSeconds;
    }

    double TheoreticalLapSeconds(int sectorCount,
                                 bool cleanOnly) const noexcept
    {
        if (sectorCount <= 0 ||
            sectorCount > MaximumTelemetrySectors)
        {
            return 0.0;
        }
        double total = 0.0;
        for (int i = 0; i < sectorCount; ++i)
        {
            const double best = BestReference(i, cleanOnly);
            if (best <= 0.0)
            {
                return 0.0;
            }
            total += best;
        }
        return total;
    }

    std::uint64_t RecordedPassages() const noexcept
    {
        return Records;
    }

private:
    std::array<SectorStatistics, MaximumTelemetrySectors> Data{};
    std::array<SectorPassage, MaximumTelemetrySectors> Latest{};
    std::array<std::array<double, 16>, MaximumTelemetrySectors>
        StoredSeconds{};
    std::array<int, MaximumTelemetrySectors> Counts{};
    std::array<int, MaximumTelemetrySectors> Next{};
    std::uint64_t Records = 0;
};

struct SectorSplitObservation final
{
    SectorObservationIssue Issue = SectorObservationIssue::None;
    SectorPassage Passage{};
    double CurrentSectorSeconds = 0.0;
    double CurrentDrivenDistanceCm = 0.0;
    double CurrentLateralErrorCm = 0.0;
    double CurrentPredictedSplitSeconds = 0.0;
    double CurrentDeltaToBestSeconds = 0.0;
    int CurrentSector = -1;
    int LastCompletedSector = -1;
    bool Accepted = false;
    bool CompletedSplit = false;
    bool SplitQualified = false;
    bool HasBestReference = false;
};

class SectorTimingObserver final
{
public:
    bool Configure(SectorTimingSettings settings) noexcept
    {
        if (!ValidSectorSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        History.Reset();
        Current = {};
        Last = {};
        PreviousFrame = {};
        LastProjection = {};
        Started = false;
        HasPrevious = false;
        StartClock = 0.0;
        LastClock = 0.0;
        Identity = 0;
        Actor = 0;
        Lap = -1;
        CurrentSector = -1;
        LastGateOrdinal = -1;
        Rejected = 0;
        Qualified = 0;
    }

    void Suspend() noexcept
    {
        HasPrevious = false;
        Started = false;
        Current = {};
    }

    SectorSplitObservation Observe(
        const TelemetryFrame& frame,
        const TelemetryFrameDecision& gate,
        const TelemetryKinematics& motion,
        const TrackProjection& projection) noexcept
    {
        SectorSplitObservation out;
        if (!gate.Accepted || !motion.Valid ||
            !IsEngineObservation(frame.Identity.Trust) ||
            !frame.Race.EnginePhysicsReady)
        {
            return Reject(SectorObservationIssue::InvalidNativeFrame, out);
        }
        if (!Finite(gate.ValidatedDeltaSeconds) ||
            gate.ValidatedDeltaSeconds <= 0.0 ||
            gate.ValidatedDeltaSeconds > Settings.MaximumSampleGapSeconds ||
            !Finite(frame.RaceClockSeconds) ||
            frame.RaceClockSeconds < 0.0 ||
            !Finite(frame.MonotonicSeconds))
        {
            return Reject(SectorObservationIssue::InvalidDuration, out);
        }
        if (Settings.RequireClockRunning &&
            !frame.Race.LocalRaceClockRunning)
        {
            return Reject(SectorObservationIssue::NoRaceClock, out);
        }
        if (frame.Identity.SessionToken == 0 ||
            frame.Identity.ActorToken == 0)
        {
            return Reject(SectorObservationIssue::MissingIdentity, out);
        }
        if (Identity != 0 &&
            (Identity != frame.Identity.SessionToken ||
             Actor != frame.Identity.ActorToken))
        {
            Reset();
            out.Issue = SectorObservationIssue::EpochChanged;
        }
        if (Identity == 0)
        {
            Identity = frame.Identity.SessionToken;
            Actor = frame.Identity.ActorToken;
        }
        if (!Started)
        {
            Start(frame, projection);
            out.Accepted = true;
            out.CurrentSector = CurrentSector;
            Last = out;
            return out;
        }
        if (HasPrevious &&
            (frame.RaceClockSeconds < LastClock ||
             frame.Identity.FrameOrdinal <=
                 PreviousFrame.Identity.FrameOrdinal))
        {
            Suspend();
            return Reject(SectorObservationIssue::GapDiscontinuity, out);
        }
        if (frame.Race.LapIndex < Lap ||
            frame.Race.LapIndex > Lap + 1)
        {
            return Reject(SectorObservationIssue::InconsistentLap, out);
        }
        const double dt = gate.ValidatedDeltaSeconds;
        Current.DrivenDistanceCm += motion.SpeedCmS * dt;
        Current.MaximumSpeedCmS = std::max(
            Current.MaximumSpeedCmS, motion.SpeedCmS);
        Current.MinimumSpeedCmS = std::min(
            Current.MinimumSpeedCmS, motion.SpeedCmS);
        Current.MeanSpeedCmS = SafeRatio(
            Current.DrivenDistanceCm,
            std::max(frame.RaceClockSeconds - StartClock, 0.001));
        Current.BrakingSeconds +=
            frame.Controls.Brake > 0.12 ? dt : 0.0;
        Current.FullThrottleSeconds +=
            frame.Controls.Throttle > 0.95 ? dt : 0.0;
        Current.SlipSeconds +=
            std::abs(motion.SlipAngleRadians) > 0.20 ? dt : 0.0;
        Current.OffTrackSeconds +=
            !frame.Environment.OnPhysicalPavement ? dt : 0.0;
        Current.CautionAffected =
            Current.CautionAffected ||
            frame.Race.Phase == TelemetryPhase::Caution;
        if (projection.Valid)
        {
            Current.LateralErrorIntegral +=
                std::abs(projection.SignedLateralCm) * dt;
            Current.MaximumLateralErrorCm = std::max(
                Current.MaximumLateralErrorCm,
                std::abs(projection.SignedLateralCm));
            Current.EndTrackCm = projection.DistanceAlongCm;
        }
        const bool gateAdvanced =
            frame.Race.CreditedGateOrdinal > LastGateOrdinal;
        const bool sectorChanged =
            frame.Race.SectorIndex != CurrentSector;
        if (sectorChanged)
        {
            if (frame.Race.SectorIndex !=
                    (CurrentSector + 1) % Settings.SectorCount)
            {
                return Reject(SectorObservationIssue::SkippedSector, out);
            }
            if (Settings.RequireVerifiedGate &&
                (!frame.Race.VerifiedForwardGate || !gateAdvanced))
            {
                return Reject(SectorObservationIssue::UnverifiedGate, out);
            }
            const double seconds =
                frame.RaceClockSeconds - StartClock;
            if (seconds < Settings.MinimumSectorDurationSeconds ||
                seconds > Settings.MaximumSectorDurationSeconds ||
                Current.DrivenDistanceCm < Settings.MinimumDistanceCm ||
                Current.DrivenDistanceCm > Settings.MaximumDistanceCm)
            {
                return Reject(SectorObservationIssue::InvalidDuration, out);
            }
            Current.FinishRaceSeconds = frame.RaceClockSeconds;
            Current.DurationSeconds = seconds;
            Current.GateOrdinal = frame.Race.CreditedGateOrdinal;
            Current.VerifiedForwardGate = true;
            Current.Qualified = !Current.CautionAffected ||
                Settings.RecordCautionSectors;
            const SectorStatistics reference =
                History.Snapshot(CurrentSector);
            Current.DeltaToBestSeconds = reference.HasReference ?
                seconds - reference.BestSeconds : 0.0;
            Current.DeltaToPreviousSeconds = reference.HasReference ?
                seconds - reference.LastSeconds : 0.0;
            Current.DeltaToMeanSeconds = reference.HasReference ?
                seconds - reference.MeanSeconds : 0.0;
            Current.EstimatedLossSeconds = std::max(
                0.0, Current.DeltaToBestSeconds);
            out.CompletedSplit = true;
            out.SplitQualified = Current.Qualified;
            out.Passage = Current;
            out.LastCompletedSector = CurrentSector;
            if (Current.Qualified)
            {
                History.Record(Current);
                Qualified = SaturatingIncrement(Qualified);
            }
            Start(frame, projection);
        }
        else
        {
            LastClock = frame.RaceClockSeconds;
            PreviousFrame = frame;
            HasPrevious = true;
        }
        out.CurrentSector = CurrentSector;
        out.CurrentSectorSeconds =
            std::max(0.0, frame.RaceClockSeconds - StartClock);
        out.CurrentDrivenDistanceCm = Current.DrivenDistanceCm;
        out.CurrentLateralErrorCm =
            projection.Valid ?
                std::abs(projection.SignedLateralCm) : 0.0;
        const SectorStatistics reference =
            History.Snapshot(CurrentSector);
        if (reference.HasReference)
        {
            out.HasBestReference = true;
            out.CurrentDeltaToBestSeconds =
                out.CurrentSectorSeconds - reference.BestSeconds;
            out.CurrentPredictedSplitSeconds =
                reference.BestSeconds + out.CurrentDeltaToBestSeconds;
        }
        out.Accepted = true;
        Last = out;
        return out;
    }

    SectorSplitObservation Snapshot() const noexcept { return Last; }
    SectorStatistics SectorStats(int sector) const noexcept
    {
        return History.Snapshot(sector);
    }
    SectorPassage LastSector(int sector) const noexcept
    {
        return History.LastPassage(sector);
    }
    int ActiveSector() const noexcept { return CurrentSector; }
    double CurrentSectorSeconds() const noexcept
    {
        return std::max(0.0, LastClock - StartClock);
    }
    double TheoreticalBestLapSeconds(bool cleanOnly) const noexcept
    {
        return History.TheoreticalLapSeconds(
            Settings.SectorCount, cleanOnly);
    }
    std::uint64_t QualifiedSplits() const noexcept { return Qualified; }
    std::uint64_t RejectedSamples() const noexcept { return Rejected; }
    void ResetReferences() noexcept { History.Reset(); }

private:
    void Start(
        const TelemetryFrame& frame,
        const TrackProjection& projection) noexcept
    {
        Current = {};
        Current.LapIndex = frame.Race.LapIndex;
        Current.SectorIndex = frame.Race.SectorIndex;
        Current.GateOrdinal = frame.Race.CreditedGateOrdinal;
        Current.StartRaceSeconds = frame.RaceClockSeconds;
        Current.FinishRaceSeconds = frame.RaceClockSeconds;
        Current.StartTrackCm = projection.Valid ?
            projection.DistanceAlongCm : 0.0;
        Current.EndTrackCm = Current.StartTrackCm;
        Current.MinimumSpeedCmS =
            Length(PlanarVelocity(frame));
        Current.MaximumSpeedCmS = Current.MinimumSpeedCmS;
        Current.SessionToken = frame.Identity.SessionToken;
        Current.ActorToken = frame.Identity.ActorToken;
        Current.FrameOrdinal = frame.Identity.FrameOrdinal;
        Started = true;
        HasPrevious = true;
        CurrentSector = frame.Race.SectorIndex;
        LastGateOrdinal = frame.Race.CreditedGateOrdinal;
        Lap = frame.Race.LapIndex;
        StartClock = frame.RaceClockSeconds;
        LastClock = frame.RaceClockSeconds;
        PreviousFrame = frame;
        LastProjection = projection;
    }

    SectorSplitObservation Reject(
        SectorObservationIssue issue,
        SectorSplitObservation out) noexcept
    {
        Rejected = SaturatingIncrement(Rejected);
        out.Issue = issue;
        out.Accepted = false;
        Last = out;
        return out;
    }

    SectorTimingSettings Settings{};
    SectorTimeHistory History;
    SectorPassage Current{};
    SectorSplitObservation Last{};
    TelemetryFrame PreviousFrame{};
    TrackProjection LastProjection{};
    std::uint64_t Identity = 0;
    std::uint64_t Actor = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t Qualified = 0;
    double StartClock = 0.0;
    double LastClock = 0.0;
    int CurrentSector = -1;
    int LastGateOrdinal = -1;
    int Lap = -1;
    bool Started = false;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering
