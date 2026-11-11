#pragma once

#include "Telemetry/TelemetryTypes.h"
#include "Telemetry/TelemetryKinematics.h"
#include "Telemetry/TelemetryValidation.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class LapJournalStatus : std::uint8_t
{
    Empty,
    InProgress,
    Completed,
    Incomplete,
    Rejected
};

struct LapJournalSettings final
{
    int RequiredSectorCount = 4;
    double MinimumSectorSeconds = 0.10;
    double MaximumSectorSeconds = 900.0;
    double MinimumLapSeconds = 4.0;
    double MaximumLapSeconds = 7200.0;
    double MaximumClockDiscrepancySeconds = 0.30;
    bool RequireEngineQualified = true;
    bool RequireForwardGateEvidence = true;
    bool RejectRecoveryCrossing = true;
    bool RejectOffTrackLap = false;
    bool AllowCautionLap = true;
};

inline bool ValidLapJournalSettings(
    const LapJournalSettings& s) noexcept
{
    return s.RequiredSectorCount >= 2 &&
           s.RequiredSectorCount <= MaximumTelemetrySectors &&
           Finite(s.MinimumSectorSeconds) &&
           Finite(s.MaximumSectorSeconds) &&
           Finite(s.MinimumLapSeconds) &&
           Finite(s.MaximumLapSeconds) &&
           Finite(s.MaximumClockDiscrepancySeconds) &&
           s.MinimumSectorSeconds > 0.0 &&
           s.MaximumSectorSeconds > s.MinimumSectorSeconds &&
           s.MinimumLapSeconds > 0.0 &&
           s.MaximumLapSeconds > s.MinimumLapSeconds &&
           s.MaximumClockDiscrepancySeconds >= 0.0 &&
           s.MaximumClockDiscrepancySeconds <= 10.0;
}

struct LapJournalEntry final
{
    TelemetryLap Lap{};
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    std::uint64_t StartFrameOrdinal = 0;
    std::uint64_t EndFrameOrdinal = 0;
    double StartingRaceClockSeconds = 0.0;
    double EndingRaceClockSeconds = 0.0;
    double SectorDurationSumSeconds = 0.0;
    double BestObservedSectorSumSeconds = 0.0;
    double UnclassifiedSeconds = 0.0;
    double PaceConsistencySeconds = 0.0;
    int VerifiedForwardGateEvents = 0;
    int IneligibleGateEvents = 0;
    int RecoveryEvents = 0;
    int RejectedSectorEvents = 0;
    int SectorCursor = 0;
    LapJournalStatus Status = LapJournalStatus::Empty;
    bool HasEngineClockProof = false;
    bool TimingDisagreesWithSectors = false;
    bool EligibleForPerformanceReview = false;
    bool AuthoritativeLapTime = false;
};

struct LapJournalReceipt final
{
    LapJournalEntry LastFinished{};
    LapJournalEntry BestFinished{};
    double BestLapSeconds = 0.0;
    double MeanQualifiedLapSeconds = 0.0;
    double MedianQualifiedLapSeconds = 0.0;
    double LastImprovementSeconds = 0.0;
    double TheoreticalSectorSumSeconds = 0.0;
    double TotalQualifiedSeconds = 0.0;
    std::uint64_t Completed = 0;
    std::uint64_t Qualified = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t Aborted = 0;
    std::uint64_t Overwritten = 0;
    bool HasCompleteLap = false;
};

class TelemetryLapJournal final
{
public:
    void Reset() noexcept
    {
        Settings = {};
        Entries.fill({});
        BestSectors.fill(0.0);
        Current = {};
        Receipt = {};
        Start = 0;
        Count = 0;
        HasActive = false;
        Session = 0;
        Actor = 0;
        DriverSlot = -1;
    }

    bool Configure(LapJournalSettings settings) noexcept
    {
        if (!ValidLapJournalSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    bool BeginLap(const TelemetryFrame& frame) noexcept
    {
        if (HasActive || !ValidEntryFrame(frame) ||
            frame.Race.LapIndex >= MaximumTelemetryLaps)
        {
            ++Receipt.Rejected;
            return false;
        }
        if (Session != 0 && Session != frame.Identity.SessionToken)
        {
            ResetLapMemory();
        }
        Session = frame.Identity.SessionToken;
        Actor = frame.Identity.ActorToken;
        DriverSlot = frame.Identity.DriverSlot;
        Current = {};
        Current.Lap.LapIndex = frame.Race.LapIndex;
        Current.Lap.StartedAtSeconds = frame.MonotonicSeconds;
        Current.StartingRaceClockSeconds = frame.RaceClockSeconds;
        Current.SessionToken = Session;
        Current.ActorToken = Actor;
        Current.StartFrameOrdinal = frame.Identity.FrameOrdinal;
        Current.SectorCursor = 0;
        Current.Status = LapJournalStatus::InProgress;
        Current.HasEngineClockProof =
            IsEngineObservation(frame.Identity.Trust) &&
            frame.Race.EnginePhysicsReady &&
            frame.Race.LocalRaceClockRunning;
        HasActive = true;
        return true;
    }

    bool ObserveSector(
        const TelemetryFrame& frame,
        int sector,
        double measuredSectorSeconds) noexcept
    {
        if (!HasActive || !ValidEntryFrame(frame) ||
            frame.Identity.SessionToken != Session ||
            frame.Identity.ActorToken != Actor ||
            frame.Identity.DriverSlot != DriverSlot ||
            !Finite(measuredSectorSeconds) ||
            measuredSectorSeconds < Settings.MinimumSectorSeconds ||
            measuredSectorSeconds > Settings.MaximumSectorSeconds ||
            sector != Current.SectorCursor ||
            sector >= Settings.RequiredSectorCount ||
            frame.Identity.FrameOrdinal <=
                Current.StartFrameOrdinal)
        {
            if (HasActive)
            {
                ++Current.RejectedSectorEvents;
            }
            ++Receipt.Rejected;
            return false;
        }
        if (Settings.RequireForwardGateEvidence &&
            !frame.Race.VerifiedForwardGate)
        {
            ++Current.IneligibleGateEvents;
            ++Receipt.Rejected;
            return false;
        }
        if (Settings.RejectRecoveryCrossing &&
            frame.Race.VerifiedRecovery)
        {
            ++Current.RecoveryEvents;
            ++Receipt.Rejected;
            return false;
        }
        const std::size_t index =
            static_cast<std::size_t>(sector);
        Current.Lap.SectorTimes[index] =
            measuredSectorSeconds;
        Current.Lap.SectorCount = sector + 1;
        Current.SectorDurationSumSeconds +=
            measuredSectorSeconds;
        ++Current.SectorCursor;
        ++Current.VerifiedForwardGateEvents;
        if (BestSectors[index] <= 0.0 ||
            measuredSectorSeconds < BestSectors[index])
        {
            BestSectors[index] = measuredSectorSeconds;
        }
        return true;
    }

    bool ObserveFrame(
        const TelemetryFrame& frame,
        const TelemetryKinematics& motion) noexcept
    {
        if (!HasActive || !ValidEntryFrame(frame) ||
            frame.Identity.SessionToken != Session ||
            frame.Identity.ActorToken != Actor ||
            frame.Identity.DriverSlot != DriverSlot ||
            !motion.Valid ||
            !Finite(frame.DeltaSeconds) ||
            frame.DeltaSeconds <= 0.0 ||
            frame.DeltaSeconds > MaximumFrameDeltaSeconds)
        {
            return false;
        }
        const double dt = frame.DeltaSeconds;
        const double speed = motion.SpeedCmS;
        const bool braking =
            frame.Controls.Brake >= 0.18;
        const bool throttle =
            frame.Controls.Throttle >= 0.18;
        const bool coasting = !braking && !throttle;
        const bool offTrack =
            !frame.Environment.OnPhysicalPavement;
        const bool sliding =
            std::abs(motion.SlipAngleRadians) >= 0.18;
        Current.Lap.DistanceCm +=
            std::max(0.0, speed) * dt;
        Current.Lap.TopSpeedKmh = std::max(
            Current.Lap.TopSpeedKmh,
            speed * CmPerSecondToKmh);
        Current.Lap.TotalBrakingSeconds += braking ? dt : 0.0;
        Current.Lap.TotalCoastingSeconds += coasting ? dt : 0.0;
        Current.Lap.TotalOffTrackSeconds += offTrack ? dt : 0.0;
        Current.Lap.TotalHighSlipSeconds += sliding ? dt : 0.0;
        Current.Lap.TotalThrottleSeconds += throttle ? dt : 0.0;
        Current.Lap.TotalWheelspinSeconds +=
            HighestWheelSlip(frame.Wheels) >= 0.25 ? dt : 0.0;
        Current.Lap.TotalTractionLimitedSeconds +=
            HighestWheelSlip(frame.Wheels) >= 0.60 ? dt : 0.0;
        Current.Lap.TotalBrakeTemperatureLoad +=
            frame.Dynamics.BrakeTemperatureCelsius * dt;
        Current.UnclassifiedSeconds += dt;
        Current.EndFrameOrdinal = frame.Identity.FrameOrdinal;
        return true;
    }

    bool CompleteLap(const TelemetryFrame& frame,
                     double explicitLapSeconds) noexcept
    {
        if (!HasActive)
        {
            ++Receipt.Rejected;
            return false;
        }
        Current.Lap.FinishedAtSeconds =
            frame.MonotonicSeconds;
        Current.EndingRaceClockSeconds =
            frame.RaceClockSeconds;
        Current.EndFrameOrdinal =
            frame.Identity.FrameOrdinal;
        Current.Lap.DurationSeconds =
            explicitLapSeconds;
        const double clockDuration =
            Current.EndingRaceClockSeconds -
            Current.StartingRaceClockSeconds;
        Current.TimingDisagreesWithSectors =
            !Finite(clockDuration) ||
            !Finite(explicitLapSeconds) ||
            std::abs(clockDuration - explicitLapSeconds) >
                Settings.MaximumClockDiscrepancySeconds ||
            std::abs(Current.SectorDurationSumSeconds -
                     explicitLapSeconds) >
                Settings.MaximumClockDiscrepancySeconds;
        bool qualified = ValidEntryFrame(frame) &&
            frame.Identity.SessionToken == Session &&
            frame.Identity.ActorToken == Actor &&
            frame.Identity.DriverSlot == DriverSlot &&
            frame.Identity.FrameOrdinal >
                Current.StartFrameOrdinal &&
            Current.SectorCursor ==
                Settings.RequiredSectorCount &&
            explicitLapSeconds >=
                Settings.MinimumLapSeconds &&
            explicitLapSeconds <=
                Settings.MaximumLapSeconds &&
            !Current.TimingDisagreesWithSectors &&
            (!Settings.RejectOffTrackLap ||
             Current.Lap.TotalOffTrackSeconds == 0.0) &&
            (Settings.AllowCautionLap ||
             frame.Race.Phase != TelemetryPhase::Caution);
        if (Settings.RequireEngineQualified &&
            !Current.HasEngineClockProof)
        {
            qualified = false;
        }
        Current.Lap.Quality = qualified ?
            TelemetryQuality::Qualified :
            TelemetryQuality::Invalid;
        Current.Status = qualified ?
            LapJournalStatus::Completed :
            LapJournalStatus::Rejected;
        Current.EligibleForPerformanceReview = qualified;
        Current.AuthoritativeLapTime = false;
        Current.Lap.MeanSpeedKmh = qualified ?
            SafeRatio(Current.Lap.DistanceCm,
                      explicitLapSeconds) *
                CmPerSecondToKmh : 0.0;
        Current.BestObservedSectorSumSeconds =
            TheoreticalBestSeconds();
        Current.PaceConsistencySeconds =
            std::abs(Current.SectorDurationSumSeconds -
                     explicitLapSeconds);
        Current.Lap.Completed = qualified;
        if (qualified)
        {
            ++Receipt.Qualified;
            Receipt.TotalQualifiedSeconds +=
                explicitLapSeconds;
            if (!Receipt.HasCompleteLap ||
                explicitLapSeconds < Receipt.BestLapSeconds)
            {
                Receipt.LastImprovementSeconds =
                    Receipt.HasCompleteLap ?
                    Receipt.BestLapSeconds -
                        explicitLapSeconds : 0.0;
                Receipt.BestFinished = Current;
                Receipt.BestLapSeconds =
                    explicitLapSeconds;
            }
            Receipt.HasCompleteLap = true;
            Receipt.MeanQualifiedLapSeconds =
                Receipt.TotalQualifiedSeconds /
                static_cast<double>(Receipt.Qualified);
        }
        else
        {
            ++Receipt.Rejected;
        }
        ++Receipt.Completed;
        Receipt.LastFinished = Current;
        Receipt.TheoreticalSectorSumSeconds =
            TheoreticalBestSeconds();
        Store(Current);
        Current = {};
        HasActive = false;
        return qualified;
    }

    void AbortLap() noexcept
    {
        if (!HasActive)
        {
            return;
        }
        Current.Status = LapJournalStatus::Incomplete;
        Current.EligibleForPerformanceReview = false;
        Store(Current);
        Receipt.LastFinished = Current;
        ++Receipt.Aborted;
        Current = {};
        HasActive = false;
    }

    LapJournalEntry CurrentLap() const noexcept
    {
        return Current;
    }

    LapJournalEntry LastLap() const noexcept
    {
        return Receipt.LastFinished;
    }

    LapJournalEntry BestLap() const noexcept
    {
        return Receipt.BestFinished;
    }

    LapJournalEntry AtOldest(std::size_t index) const noexcept
    {
        return index < Count ?
            Entries[(Start + index) % Entries.size()] :
            LapJournalEntry{};
    }

    LapJournalEntry AtNewest(std::size_t index = 0) const noexcept
    {
        return index < Count ?
            Entries[(Start + Count - index - 1) %
                    Entries.size()] : LapJournalEntry{};
    }

    LapJournalReceipt Summary() const noexcept
    {
        return Receipt;
    }

    double TheoreticalBestSeconds() const noexcept
    {
        double total = 0.0;
        for (int i = 0; i < Settings.RequiredSectorCount; ++i)
        {
            const double best =
                BestSectors[static_cast<std::size_t>(i)];
            if (best <= 0.0)
            {
                return 0.0;
            }
            total += best;
        }
        return total;
    }

    double BestSectorSeconds(int sector) const noexcept
    {
        return sector >= 0 &&
               sector < Settings.RequiredSectorCount ?
            BestSectors[static_cast<std::size_t>(sector)] : 0.0;
    }

    std::size_t StoredLaps() const noexcept
    {
        return Count;
    }

    bool HasActiveLap() const noexcept
    {
        return HasActive;
    }

private:
    bool ValidEntryFrame(
        const TelemetryFrame& frame) const noexcept
    {
        if (!frame.Race.EnginePhysicsReady ||
            !IsRaceMoving(frame.Race.Phase) ||
            frame.Identity.SessionToken == 0 ||
            frame.Identity.ActorToken == 0 ||
            frame.Identity.DriverSlot < 0 ||
            frame.Identity.DriverSlot >= MaximumTelemetryDrivers ||
            !Finite(frame.MonotonicSeconds) ||
            !Finite(frame.RaceClockSeconds) ||
            frame.MonotonicSeconds < 0.0 ||
            frame.RaceClockSeconds < 0.0)
        {
            return false;
        }
        return !Settings.RequireEngineQualified ||
            IsEngineObservation(frame.Identity.Trust);
    }

    void Store(const LapJournalEntry& entry) noexcept
    {
        if (Count == Entries.size())
        {
            Start = (Start + 1) % Entries.size();
            ++Receipt.Overwritten;
        }
        else
        {
            ++Count;
        }
        Entries[(Start + Count - 1) %
                Entries.size()] = entry;
    }

    void ResetLapMemory() noexcept
    {
        Entries.fill({});
        BestSectors.fill(0.0);
        Current = {};
        Receipt = {};
        Start = 0;
        Count = 0;
        HasActive = false;
    }

    LapJournalSettings Settings{};
    std::array<LapJournalEntry, MaximumTelemetryLaps> Entries{};
    std::array<double, MaximumTelemetrySectors> BestSectors{};
    LapJournalEntry Current{};
    LapJournalReceipt Receipt{};
    std::size_t Start = 0;
    std::size_t Count = 0;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    int DriverSlot = -1;
    bool HasActive = false;
};
} // namespace latebrake::engineering
