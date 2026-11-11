#pragma once

#include "Telemetry/TelemetryTrack.h"
#include "Telemetry/TelemetryOpponents.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class IncidentKind : std::uint8_t
{
    Unknown,
    OffTrack,
    WallContact,
    OpponentContact,
    WheelLift,
    Recovery,
    TireFailure,
    Overheat,
    SensorLoss,
    Spin,
    WrongWay,
    UnsafeStop
};

enum class IncidentSeverity : std::uint8_t
{
    Unclassified,
    Minor,
    Moderate,
    Major,
    Critical
};

struct IncidentObservation final
{
    IncidentKind Kind = IncidentKind::Unknown;
    double MonotonicSeconds = 0.0;
    double TrackDistanceCm = 0.0;
    double SpeedCmS = 0.0;
    double RelativeImpactSpeedCmS = 0.0;
    double ChassisAccelerationG = 0.0;
    double WheelContactFraction = 1.0;
    double ChassisDamageBefore = 0.0;
    double ChassisDamageAfter = 0.0;
    double LateralSlipRadians = 0.0;
    double DurationSeconds = 0.0;
    double OffRoadDistanceCm = 0.0;
    int LapIndex = -1;
    int SectorIndex = -1;
    int OwnSlot = -1;
    int OtherSlot = -1;
    std::uint64_t Session = 0;
    std::uint64_t OwnActor = 0;
    std::uint64_t OtherActor = 0;
    bool PhysicalContactConfirmed = false;
    bool EngineQualified = false;
    bool AuthoritativePenaltyIssued = false;
};

struct IncidentJournalSettings final
{
    double MinimumSeparateIncidentSeconds = 0.25;
    double MaximumInputIncidentSeconds = 180.0;
    double MinorImpactSpeedCmS = 150.0;
    double ModerateImpactSpeedCmS = 700.0;
    double MajorImpactSpeedCmS = 1600.0;
    double CriticalImpactSpeedCmS = 3200.0;
    double ModerateDamageIncrease = 0.08;
    double MajorDamageIncrease = 0.25;
    double CriticalDamageIncrease = 0.50;
    double SpinSlipThresholdRadians = 0.60;
    double WheelLiftContactFraction = 0.50;
    double MaximumValidSpeedCmS = MaximumPlanarSpeedCmS;
    int MaximumIncidentsPerLap = 1000;
    bool RequirePhysicalContactForCollision = true;
};

inline bool ValidIncidentSettings(
    IncidentJournalSettings s) noexcept
{
    return Finite(s.MinimumSeparateIncidentSeconds) &&
        s.MinimumSeparateIncidentSeconds >= 0.0 &&
        Finite(s.MaximumInputIncidentSeconds) &&
        s.MaximumInputIncidentSeconds > 0.0 &&
        Finite(s.MinorImpactSpeedCmS) &&
        Finite(s.ModerateImpactSpeedCmS) &&
        Finite(s.MajorImpactSpeedCmS) &&
        Finite(s.CriticalImpactSpeedCmS) &&
        s.MinorImpactSpeedCmS > 0.0 &&
        s.ModerateImpactSpeedCmS > s.MinorImpactSpeedCmS &&
        s.MajorImpactSpeedCmS > s.ModerateImpactSpeedCmS &&
        s.CriticalImpactSpeedCmS > s.MajorImpactSpeedCmS &&
        Finite(s.ModerateDamageIncrease) &&
        Finite(s.MajorDamageIncrease) &&
        Finite(s.CriticalDamageIncrease) &&
        s.ModerateDamageIncrease > 0.0 &&
        s.MajorDamageIncrease > s.ModerateDamageIncrease &&
        s.CriticalDamageIncrease > s.MajorDamageIncrease &&
        Finite(s.SpinSlipThresholdRadians) &&
        s.SpinSlipThresholdRadians > 0.0 &&
        Finite(s.WheelLiftContactFraction) &&
        s.WheelLiftContactFraction >= 0.0 &&
        s.WheelLiftContactFraction < 1.0 &&
        Finite(s.MaximumValidSpeedCmS) &&
        s.MaximumValidSpeedCmS > 0.0 &&
        s.MaximumIncidentsPerLap >= 1;
}

inline bool ValidIncidentObservation(
    IncidentObservation e,
    IncidentJournalSettings settings) noexcept
{
    if (!e.EngineQualified ||
        e.Session == 0 || e.OwnActor == 0 ||
        e.OwnSlot < 0 ||
        e.OwnSlot >= MaximumTelemetryDrivers ||
        e.LapIndex < 0 ||
        e.LapIndex > MaximumTelemetryLaps ||
        e.SectorIndex < -1 ||
        e.SectorIndex >= MaximumTelemetrySectors ||
        !Finite(e.MonotonicSeconds) ||
        !Finite(e.TrackDistanceCm) ||
        !Finite(e.SpeedCmS) ||
        !Finite(e.RelativeImpactSpeedCmS) ||
        !Finite(e.ChassisAccelerationG) ||
        !Finite(e.WheelContactFraction) ||
        !Finite(e.ChassisDamageBefore) ||
        !Finite(e.ChassisDamageAfter) ||
        !Finite(e.LateralSlipRadians) ||
        !Finite(e.DurationSeconds) ||
        !Finite(e.OffRoadDistanceCm) ||
        e.MonotonicSeconds < 0.0 ||
        e.TrackDistanceCm < 0.0 ||
        e.SpeedCmS < 0.0 ||
        e.SpeedCmS > settings.MaximumValidSpeedCmS ||
        e.RelativeImpactSpeedCmS < 0.0 ||
        e.RelativeImpactSpeedCmS > settings.MaximumValidSpeedCmS * 2.0 ||
        e.WheelContactFraction < 0.0 ||
        e.WheelContactFraction > 1.0 ||
        e.ChassisDamageBefore < 0.0 ||
        e.ChassisDamageBefore > 1.0 ||
        e.ChassisDamageAfter < 0.0 ||
        e.ChassisDamageAfter > 1.0 ||
        e.DurationSeconds < 0.0 ||
        e.DurationSeconds > settings.MaximumInputIncidentSeconds ||
        e.OffRoadDistanceCm < 0.0)
    {
        return false;
    }
    if (settings.RequirePhysicalContactForCollision &&
        (e.Kind == IncidentKind::WallContact ||
         e.Kind == IncidentKind::OpponentContact) &&
        !e.PhysicalContactConfirmed)
    {
        return false;
    }
    if (e.Kind == IncidentKind::OpponentContact &&
        (e.OtherActor == 0 ||
         e.OtherActor == e.OwnActor ||
         e.OtherSlot < 0 ||
         e.OtherSlot >= MaximumTelemetryDrivers ||
         e.OtherSlot == e.OwnSlot))
    {
        return false;
    }
    return true;
}

struct IncidentRecord final
{
    IncidentObservation Observation{};
    IncidentSeverity Severity = IncidentSeverity::Unclassified;
    double EstimatedLostTimeSeconds = 0.0;
    double DamageIncreaseFraction = 0.0;
    double EstimatedEnergyPerKg = 0.0;
    double ClosestPriorIncidentSeconds = 0.0;
    std::uint64_t UniqueOrdinal = 0;
    int RepeatedInSector = 0;
    int RepeatedInLap = 0;
    bool CollisionVerified = false;
    bool MayAffectOfficialRace = false;
    bool DuplicateSuppressed = false;
    bool AdvisoryOnly = true;
    bool Valid = false;
};

inline IncidentSeverity RateIncidentSeverity(
    IncidentObservation e,
    IncidentJournalSettings settings) noexcept
{
    const double damage = std::max(
        0.0, e.ChassisDamageAfter -
             e.ChassisDamageBefore);
    const double speed =
        e.RelativeImpactSpeedCmS;
    if (damage >= settings.CriticalDamageIncrease ||
        speed >= settings.CriticalImpactSpeedCmS)
    {
        return IncidentSeverity::Critical;
    }
    if (damage >= settings.MajorDamageIncrease ||
        speed >= settings.MajorImpactSpeedCmS)
    {
        return IncidentSeverity::Major;
    }
    if (damage >= settings.ModerateDamageIncrease ||
        speed >= settings.ModerateImpactSpeedCmS)
    {
        return IncidentSeverity::Moderate;
    }
    if (speed >= settings.MinorImpactSpeedCmS ||
        e.DurationSeconds >= 2.0)
    {
        return IncidentSeverity::Minor;
    }
    return IncidentSeverity::Unclassified;
}

inline IncidentRecord EvaluateIncident(
    IncidentObservation observation,
    IncidentJournalSettings settings = {}) noexcept
{
    IncidentRecord record;
    if (!ValidIncidentSettings(settings) ||
        !ValidIncidentObservation(observation, settings))
    {
        return record;
    }
    record.Observation = observation;
    record.Severity =
        RateIncidentSeverity(observation, settings);
    record.DamageIncreaseFraction = std::max(
        0.0, observation.ChassisDamageAfter -
             observation.ChassisDamageBefore);
    const double impactMS =
        observation.RelativeImpactSpeedCmS / CmPerMeter;
    record.EstimatedEnergyPerKg =
        0.5 * impactMS * impactMS;
    record.EstimatedLostTimeSeconds =
        observation.DurationSeconds;
    if (observation.Kind == IncidentKind::OffTrack)
    {
        record.EstimatedLostTimeSeconds += SafeRatio(
            observation.OffRoadDistanceCm,
            std::max(100.0, observation.SpeedCmS));
    }
    record.CollisionVerified =
        observation.PhysicalContactConfirmed &&
        (observation.Kind == IncidentKind::WallContact ||
         observation.Kind == IncidentKind::OpponentContact);
    record.MayAffectOfficialRace = false;
    record.AdvisoryOnly = true;
    record.Valid = true;
    return record;
}

struct IncidentJournalReview final
{
    std::array<int, 12> EventsByKind{};
    std::array<int, 5> EventsBySeverity{};
    double TotalEstimatedLostSeconds = 0.0;
    double TotalContactEnergyPerKg = 0.0;
    double TotalOffRoadSeconds = 0.0;
    double TotalRecoverySeconds = 0.0;
    double MaximumDamageIncreaseFraction = 0.0;
    double MaximumRelativeContactSpeedCmS = 0.0;
    double TimeSinceMostRecentSeconds = 0.0;
    int Recorded = 0;
    int Rejected = 0;
    int DuplicateSuppressed = 0;
    int WallContacts = 0;
    int OpponentContacts = 0;
    int MajorIncidents = 0;
    int CriticalIncidents = 0;
    bool HasEngineProof = false;
    bool OfficialRacePenaltyAuthorized = false;
};

class IncidentJournal final
{
public:
    static constexpr int MaximumRecordedIncidents = 256;

    void Reset() noexcept
    {
        Settings = {};
        Events.fill({});
        Review = {};
        Head = 0;
        Count = 0;
        Session = 0;
        Actor = 0;
        PreviousTimestamp = 0.0;
        HasPrevious = false;
        LastByKind.fill(
            -std::numeric_limits<double>::infinity());
    }

    bool Configure(IncidentJournalSettings settings) noexcept
    {
        if (!ValidIncidentSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    bool Observe(IncidentObservation observation) noexcept
    {
        IncidentRecord event =
            EvaluateIncident(observation, Settings);
        if (!event.Valid)
        {
            ++Review.Rejected;
            return false;
        }
        if (Session != observation.Session ||
            Actor != observation.OwnActor)
        {
            ResetHistoryOnEpoch();
            Session = observation.Session;
            Actor = observation.OwnActor;
        }
        if (HasPrevious &&
            observation.MonotonicSeconds <
                PreviousTimestamp)
        {
            ++Review.Rejected;
            return false;
        }
        const std::size_t kind =
            static_cast<std::size_t>(observation.Kind);
        if (kind >= LastByKind.size())
        {
            ++Review.Rejected;
            return false;
        }
        if (observation.MonotonicSeconds -
            LastByKind[kind] <
            Settings.MinimumSeparateIncidentSeconds)
        {
            ++Review.DuplicateSuppressed;
            return false;
        }
        event.ClosestPriorIncidentSeconds =
            HasPrevious ?
            observation.MonotonicSeconds -
            PreviousTimestamp : 0.0;
        event.UniqueOrdinal =
            static_cast<std::uint64_t>(
                Review.Recorded + 1);
        for (int i = 0; i < Count; ++i)
        {
            const IncidentRecord existing =
                AtOldest(i);
            if (existing.Observation.LapIndex ==
                observation.LapIndex)
            {
                ++event.RepeatedInLap;
                if (existing.Observation.SectorIndex ==
                    observation.SectorIndex)
                {
                    ++event.RepeatedInSector;
                }
            }
        }
        if (event.RepeatedInLap >=
            Settings.MaximumIncidentsPerLap)
        {
            ++Review.Rejected;
            return false;
        }
        Store(event);
        LastByKind[kind] =
            observation.MonotonicSeconds;
        PreviousTimestamp =
            observation.MonotonicSeconds;
        HasPrevious = true;
        ++Review.Recorded;
        ++Review.EventsByKind[kind];
        const std::size_t severity =
            static_cast<std::size_t>(event.Severity);
        if (severity < Review.EventsBySeverity.size())
        {
            ++Review.EventsBySeverity[severity];
        }
        Review.TotalEstimatedLostSeconds +=
            event.EstimatedLostTimeSeconds;
        Review.TotalContactEnergyPerKg +=
            event.CollisionVerified ?
            event.EstimatedEnergyPerKg : 0.0;
        Review.TotalOffRoadSeconds +=
            observation.Kind == IncidentKind::OffTrack ?
                observation.DurationSeconds : 0.0;
        Review.TotalRecoverySeconds +=
            observation.Kind == IncidentKind::Recovery ?
                observation.DurationSeconds : 0.0;
        Review.MaximumDamageIncreaseFraction =
            std::max(Review.MaximumDamageIncreaseFraction,
                     event.DamageIncreaseFraction);
        Review.MaximumRelativeContactSpeedCmS =
            std::max(Review.MaximumRelativeContactSpeedCmS,
                     observation.RelativeImpactSpeedCmS);
        Review.WallContacts +=
            observation.Kind == IncidentKind::WallContact ? 1 : 0;
        Review.OpponentContacts +=
            observation.Kind == IncidentKind::OpponentContact ? 1 : 0;
        Review.MajorIncidents +=
            event.Severity == IncidentSeverity::Major ? 1 : 0;
        Review.CriticalIncidents +=
            event.Severity == IncidentSeverity::Critical ? 1 : 0;
        Review.HasEngineProof = true;
        Review.OfficialRacePenaltyAuthorized = false;
        return true;
    }

    IncidentRecord AtOldest(int index) const noexcept
    {
        return index >= 0 && index < Count ?
            Events[static_cast<std::size_t>(
                (Head + index) % MaximumRecordedIncidents)] :
            IncidentRecord{};
    }

    IncidentRecord AtNewest(int offset = 0) const noexcept
    {
        return offset >= 0 && offset < Count ?
            AtOldest(Count - 1 - offset) : IncidentRecord{};
    }

    IncidentJournalReview Snapshot() const noexcept
    {
        return Review;
    }

    int StoredEvents() const noexcept
    {
        return Count;
    }

private:
    void Store(IncidentRecord event) noexcept
    {
        if (Count == MaximumRecordedIncidents)
        {
            Head = (Head + 1) %
                MaximumRecordedIncidents;
        }
        else
        {
            ++Count;
        }
        Events[static_cast<std::size_t>(
            (Head + Count - 1) %
            MaximumRecordedIncidents)] = event;
    }

    void ResetHistoryOnEpoch() noexcept
    {
        Events.fill({});
        Review = {};
        Head = 0;
        Count = 0;
        HasPrevious = false;
        LastByKind.fill(
            -std::numeric_limits<double>::infinity());
    }

    IncidentJournalSettings Settings{};
    std::array<IncidentRecord, MaximumRecordedIncidents> Events{};
    std::array<double, 12> LastByKind{};
    IncidentJournalReview Review{};
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    double PreviousTimestamp = 0.0;
    int Head = 0;
    int Count = 0;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering
