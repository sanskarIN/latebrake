#pragma once

#include "Telemetry/TelemetryFuelStrategy.h"
#include "Telemetry/TelemetryVehicleHealth.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class PitPhase : std::uint8_t
{
    Outside,
    Entry,
    LaneTransit,
    Stopped,
    ServiceObserved,
    Exit,
    Completed,
    Interrupted
};

enum class PitObservationIssue : std::uint8_t
{
    None,
    InvalidPhysics,
    InvalidClock,
    WrongSession,
    UnverifiedPitFlag,
    UnverifiedStop,
    ImplausibleSpeed,
    InvalidFuelData,
    RaceInterrupted,
    NoVerifiedService,
    TooShort,
    TooLong,
    Duplicate
};

struct PitReviewSettings final
{
    double MaximumPitLaneSpeedCmS = 2300.0;
    double StationaryThresholdCmS = 70.0;
    double MinimumStationarySeconds = 0.20;
    double MinimumPitVisitSeconds = 0.5;
    double MaximumPitVisitSeconds = 240.0;
    double MinimumServiceRefuelFraction = 0.003;
    double MinimumServiceRepairFraction = 0.01;
    double MinimumTireWearResetFraction = 0.02;
    double MaximumFrameGapSeconds = 0.1;
    double MinimumPitLaneDistanceCm = 150.0;
    double ConservativePitLossSeconds = 20.0;
    double MaximumPitEntrySpeedCmS = 3000.0;
    double MaximumPitExitSpeedCmS = 3000.0;
    bool RequireNativePitLane = true;
    bool RequireStationaryService = true;
    bool RequireValidForwardProgress = false;
};

inline bool ValidPitReviewSettings(
    const PitReviewSettings& s) noexcept
{
    return Finite(s.MaximumPitLaneSpeedCmS) &&
           s.MaximumPitLaneSpeedCmS > 0.0 &&
           Finite(s.StationaryThresholdCmS) &&
           s.StationaryThresholdCmS >= 0.0 &&
           s.StationaryThresholdCmS < s.MaximumPitLaneSpeedCmS &&
           Finite(s.MinimumStationarySeconds) &&
           s.MinimumStationarySeconds > 0.0 &&
           Finite(s.MinimumPitVisitSeconds) &&
           s.MinimumPitVisitSeconds > 0.0 &&
           Finite(s.MaximumPitVisitSeconds) &&
           s.MaximumPitVisitSeconds >
               s.MinimumPitVisitSeconds &&
           Finite(s.MinimumServiceRefuelFraction) &&
           s.MinimumServiceRefuelFraction >= 0.0 &&
           Finite(s.MinimumServiceRepairFraction) &&
           s.MinimumServiceRepairFraction >= 0.0 &&
           Finite(s.MinimumTireWearResetFraction) &&
           s.MinimumTireWearResetFraction >= 0.0 &&
           Finite(s.MaximumFrameGapSeconds) &&
           s.MaximumFrameGapSeconds > 0.0 &&
           s.MaximumFrameGapSeconds <= 1.0 &&
           Finite(s.MinimumPitLaneDistanceCm) &&
           s.MinimumPitLaneDistanceCm >= 0.0 &&
           Finite(s.ConservativePitLossSeconds) &&
           s.ConservativePitLossSeconds >= 0.0 &&
           Finite(s.MaximumPitEntrySpeedCmS) &&
           s.MaximumPitEntrySpeedCmS >
               s.MaximumPitLaneSpeedCmS &&
           Finite(s.MaximumPitExitSpeedCmS) &&
           s.MaximumPitExitSpeedCmS >
               s.MaximumPitLaneSpeedCmS;
}

struct PitVisitRecord final
{
    int VisitOrdinal = 0;
    int EntryLapIndex = -1;
    int ExitLapIndex = -1;
    int EntrySector = -1;
    int ExitSector = -1;
    double EntrySeconds = 0.0;
    double ExitSeconds = 0.0;
    double DurationSeconds = 0.0;
    double LaneTransitSeconds = 0.0;
    double StationarySeconds = 0.0;
    double ServiceSeconds = 0.0;
    double ExitTravelSeconds = 0.0;
    double EntrySpeedCmS = 0.0;
    double ExitSpeedCmS = 0.0;
    double MaximumLaneSpeedCmS = 0.0;
    double MinimumLaneSpeedCmS = 0.0;
    double LaneDistanceCm = 0.0;
    double FuelAtEntry = 0.0;
    double FuelAtExit = 0.0;
    double FuelAddedFraction = 0.0;
    double DamageAtEntry = 0.0;
    double DamageAtExit = 0.0;
    double RepairFraction = 0.0;
    double TireWearAtEntry = 0.0;
    double TireWearAtExit = 0.0;
    double TireWearRestoredFraction = 0.0;
    double EstimatedRaceTimeLossSeconds = 0.0;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    int PitSpeedExceedances = 0;
    int StationaryEpisodes = 0;
    bool RefuelObserved = false;
    bool RepairObserved = false;
    bool TireChangeSuspected = false;
    bool ServiceConfirmed = false;
    bool ValidVisit = false;
    bool Aborted = false;
};

struct PitLiveObservation final
{
    PitPhase Phase = PitPhase::Outside;
    PitObservationIssue Issue = PitObservationIssue::None;
    double CurrentLaneSeconds = 0.0;
    double CurrentStationarySeconds = 0.0;
    double CurrentLaneDistanceCm = 0.0;
    double CurrentSpeedCmS = 0.0;
    double LatestFuelFraction = 0.0;
    double LatestDamageFraction = 0.0;
    double LatestWearFraction = 0.0;
    bool InPitLane = false;
    bool Stationary = false;
    bool SpeedLimitExceeded = false;
    bool NewVisit = false;
    bool CompletedVisit = false;
    bool Valid = false;
};

class PitVisitMonitor final
{
public:
    bool Configure(PitReviewSettings settings) noexcept
    {
        if (!ValidPitReviewSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Working = {};
        Latest = {};
        Live = {};
        TotalVisits = 0;
        AcceptedVisits = 0;
        RejectedSamples = 0;
        Session = 0;
        Actor = 0;
        PreviousFuel = 0.0;
        PreviousDamage = 0.0;
        PreviousWear = 0.0;
        StationaryRunSeconds = 0.0;
        EntryClock = 0.0;
        LastTime = 0.0;
        HasPrevious = false;
        Active = false;
        WasStationary = false;
        PreviousPitFlag = false;
        History.fill({});
    }

    void Suspend() noexcept
    {
        if (Active)
        {
            Working.Aborted = true;
            Latest = Working;
        }
        Active = false;
        HasPrevious = false;
        StationaryRunSeconds = 0.0;
        Live = {};
    }

    bool Observe(
        const TelemetryFrame& frame,
        const TelemetryFrameDecision& gate,
        const TelemetryKinematics& motion) noexcept
    {
        if (!gate.Accepted || !motion.Valid ||
            !Finite(gate.ValidatedDeltaSeconds) ||
            gate.ValidatedDeltaSeconds <= 0.0 ||
            gate.ValidatedDeltaSeconds >
                Settings.MaximumFrameGapSeconds ||
            !Finite(frame.MonotonicSeconds) ||
            !Finite(frame.Dynamics.FuelFraction) ||
            !Finite(frame.Dynamics.ChassisDamageFraction) ||
            (Settings.RequireNativePitLane &&
             (!frame.Race.EnginePhysicsReady ||
              !IsEngineObservation(frame.Identity.Trust))))
        {
            RejectedSamples = SaturatingIncrement(RejectedSamples);
            Suspend();
            return false;
        }
        if (Session != 0 &&
            (Session != frame.Identity.SessionToken ||
             Actor != frame.Identity.ActorToken))
        {
            Reset();
        }
        Session = frame.Identity.SessionToken;
        Actor = frame.Identity.ActorToken;
        const double dt = gate.ValidatedDeltaSeconds;
        const double speed = motion.SpeedCmS;
        const bool inLane = frame.Environment.InPitLane;
        if (HasPrevious && frame.MonotonicSeconds <= LastTime)
        {
            RejectedSamples = SaturatingIncrement(RejectedSamples);
            Suspend();
            return false;
        }
        const double wear = MeanWheelWear(frame.Wheels);
        const bool stationary =
            speed <= Settings.StationaryThresholdCmS;
        const bool entry = inLane && !PreviousPitFlag;
        const bool exit = !inLane && PreviousPitFlag;
        Live = {};
        Live.Phase = Active ? PitPhase::LaneTransit : PitPhase::Outside;
        Live.CurrentSpeedCmS = speed;
        Live.CurrentLaneSeconds = Active ?
            frame.MonotonicSeconds - EntryClock : 0.0;
        Live.CurrentLaneDistanceCm =
            Active ? Working.LaneDistanceCm : 0.0;
        Live.CurrentStationarySeconds = StationaryRunSeconds;
        Live.LatestFuelFraction = frame.Dynamics.FuelFraction;
        Live.LatestDamageFraction =
            frame.Dynamics.ChassisDamageFraction;
        Live.LatestWearFraction = wear;
        Live.InPitLane = inLane;
        Live.Stationary = stationary;
        Live.SpeedLimitExceeded = inLane &&
            speed > Settings.MaximumPitLaneSpeedCmS;
        Live.Valid = true;
        if (entry)
        {
            BeginVisit(frame, motion, wear);
            Live.NewVisit = true;
            Live.Phase = PitPhase::Entry;
        }
        if (Active && inLane)
        {
            Working.LaneTransitSeconds += dt;
            Working.LaneDistanceCm += speed * dt;
            Working.MaximumLaneSpeedCmS = std::max(
                Working.MaximumLaneSpeedCmS, speed);
            Working.MinimumLaneSpeedCmS = std::min(
                Working.MinimumLaneSpeedCmS, speed);
            if (Live.SpeedLimitExceeded)
            {
                ++Working.PitSpeedExceedances;
            }
            if (stationary)
            {
                StationaryRunSeconds += dt;
                Working.StationarySeconds += dt;
                Live.Phase = PitPhase::Stopped;
                if (!WasStationary)
                {
                    ++Working.StationaryEpisodes;
                }
            }
            else
            {
                StationaryRunSeconds = 0.0;
                Live.Phase = PitPhase::LaneTransit;
            }
            const double deltaFuel =
                frame.Dynamics.FuelFraction - PreviousFuel;
            const double deltaDamage =
                PreviousDamage -
                frame.Dynamics.ChassisDamageFraction;
            const double deltaWear = PreviousWear - wear;
            const bool serviceWindow =
                (!Settings.RequireStationaryService ||
                 stationary) &&
                Working.StationarySeconds >=
                    Settings.MinimumStationarySeconds;
            if (serviceWindow)
            {
                if (deltaFuel >=
                    Settings.MinimumServiceRefuelFraction)
                {
                    Working.RefuelObserved = true;
                    Working.FuelAddedFraction += deltaFuel;
                }
                if (deltaDamage >=
                    Settings.MinimumServiceRepairFraction)
                {
                    Working.RepairObserved = true;
                    Working.RepairFraction += deltaDamage;
                }
                if (deltaWear >=
                    Settings.MinimumTireWearResetFraction)
                {
                    Working.TireChangeSuspected = true;
                    Working.TireWearRestoredFraction += deltaWear;
                }
                Working.ServiceConfirmed =
                    Working.RefuelObserved ||
                    Working.RepairObserved ||
                    Working.TireChangeSuspected;
                if (Working.ServiceConfirmed)
                {
                    Working.ServiceSeconds += dt;
                    Live.Phase = PitPhase::ServiceObserved;
                }
            }
        }
        if (Active && exit)
        {
            CompleteVisit(frame, motion, wear);
            Live.CompletedVisit = true;
            Live.Phase = PitPhase::Completed;
            Live.CurrentLaneSeconds = Working.DurationSeconds;
        }
        if (Active &&
            frame.MonotonicSeconds - EntryClock >
                Settings.MaximumPitVisitSeconds)
        {
            Working.Aborted = true;
            Latest = Working;
            Active = false;
            Live.Phase = PitPhase::Interrupted;
            Live.Issue = PitObservationIssue::TooLong;
        }
        PreviousFuel = frame.Dynamics.FuelFraction;
        PreviousDamage = frame.Dynamics.ChassisDamageFraction;
        PreviousWear = wear;
        LastTime = frame.MonotonicSeconds;
        PreviousPitFlag = inLane;
        WasStationary = stationary;
        HasPrevious = true;
        return true;
    }

    PitLiveObservation Snapshot() const noexcept { return Live; }
    PitVisitRecord LatestVisit() const noexcept { return Latest; }
    int VisitCount() const noexcept { return TotalVisits; }
    int QualifiedVisits() const noexcept { return AcceptedVisits; }
    std::uint64_t RejectedObservations() const noexcept
    {
        return RejectedSamples;
    }
    PitVisitRecord VisitAt(int index) const noexcept
    {
        return index >= 0 && index < AcceptedVisits &&
               index < MaximumTelemetryLaps ?
            History[static_cast<std::size_t>(index)] :
            PitVisitRecord{};
    }
    double AverageServiceSeconds() const noexcept
    {
        double sum = 0.0;
        for (int i = 0; i < AcceptedVisits; ++i)
        {
            sum += History[static_cast<std::size_t>(i)]
                .ServiceSeconds;
        }
        return SafeRatio(sum, AcceptedVisits);
    }
    double AveragePitVisitSeconds() const noexcept
    {
        double sum = 0.0;
        for (int i = 0; i < AcceptedVisits; ++i)
        {
            sum += History[static_cast<std::size_t>(i)]
                .DurationSeconds;
        }
        return SafeRatio(sum, AcceptedVisits);
    }

private:
    void BeginVisit(
        const TelemetryFrame& frame,
        const TelemetryKinematics& motion,
        double wear) noexcept
    {
        Working = {};
        Working.VisitOrdinal = TotalVisits + 1;
        Working.EntryLapIndex = frame.Race.LapIndex;
        Working.EntrySector = frame.Race.SectorIndex;
        Working.EntrySeconds = frame.MonotonicSeconds;
        Working.EntrySpeedCmS = motion.SpeedCmS;
        Working.MaximumLaneSpeedCmS = motion.SpeedCmS;
        Working.MinimumLaneSpeedCmS = motion.SpeedCmS;
        Working.FuelAtEntry = frame.Dynamics.FuelFraction;
        Working.DamageAtEntry =
            frame.Dynamics.ChassisDamageFraction;
        Working.TireWearAtEntry = wear;
        Working.SessionToken = frame.Identity.SessionToken;
        Working.ActorToken = frame.Identity.ActorToken;
        EntryClock = frame.MonotonicSeconds;
        StationaryRunSeconds = 0.0;
        TotalVisits += 1;
        Active = true;
    }

    void CompleteVisit(
        const TelemetryFrame& frame,
        const TelemetryKinematics& motion,
        double wear) noexcept
    {
        Working.ExitSeconds = frame.MonotonicSeconds;
        Working.DurationSeconds =
            Working.ExitSeconds - Working.EntrySeconds;
        Working.ExitSpeedCmS = motion.SpeedCmS;
        Working.ExitLapIndex = frame.Race.LapIndex;
        Working.ExitSector = frame.Race.SectorIndex;
        Working.FuelAtExit = frame.Dynamics.FuelFraction;
        Working.DamageAtExit =
            frame.Dynamics.ChassisDamageFraction;
        Working.TireWearAtExit = wear;
        Working.EstimatedRaceTimeLossSeconds =
            std::max(0.0,
                Working.DurationSeconds -
                Working.LaneDistanceCm /
                    std::max(100.0, motion.SpeedCmS));
        Working.ValidVisit =
            Working.DurationSeconds >=
                Settings.MinimumPitVisitSeconds &&
            Working.DurationSeconds <=
                Settings.MaximumPitVisitSeconds &&
            Working.LaneDistanceCm >=
                Settings.MinimumPitLaneDistanceCm &&
            !Working.Aborted;
        if (Working.ValidVisit &&
            AcceptedVisits < MaximumTelemetryLaps)
        {
            History[static_cast<std::size_t>(AcceptedVisits++)] =
                Working;
        }
        Latest = Working;
        Active = false;
        StationaryRunSeconds = 0.0;
    }

    PitReviewSettings Settings{};
    std::array<PitVisitRecord, MaximumTelemetryLaps> History{};
    PitVisitRecord Working{};
    PitVisitRecord Latest{};
    PitLiveObservation Live{};
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    std::uint64_t RejectedSamples = 0;
    double PreviousFuel = 0.0;
    double PreviousDamage = 0.0;
    double PreviousWear = 0.0;
    double StationaryRunSeconds = 0.0;
    double EntryClock = 0.0;
    double LastTime = 0.0;
    int TotalVisits = 0;
    int AcceptedVisits = 0;
    bool HasPrevious = false;
    bool Active = false;
    bool WasStationary = false;
    bool PreviousPitFlag = false;
};
} // namespace latebrake::engineering
