#pragma once

#include "Telemetry/TelemetryGrip.h"
#include "Telemetry/TelemetryTrack.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class OpponentRelativePosition : std::uint8_t
{
    Unknown,
    FarAhead,
    Following,
    SideBySide,
    FarBehind,
    Approaching,
    Crossing,
    Receding
};

enum class OpponentRiskLevel : std::uint8_t
{
    Unknown,
    Clear,
    Nearby,
    Closing,
    PredictedContact,
    AlreadyOverlapping
};

struct OpponentObservation final
{
    int Slot = -1;
    std::uint64_t ActorToken = 0;
    Vector2 PositionCm{};
    Vector2 VelocityCmS{};
    double HeadingRadians = 0.0;
    double HalfLengthCm = 220.0;
    double HalfWidthCm = 105.0;
    double ChassisHeightCm = 130.0;
    bool EngineVerified = false;
    bool OnPhysicalTrack = false;
};

struct OpponentAnalysisSettings final
{
    double MaximumPhysicalSpeedCmS = 16000.0;
    double MaximumRangeCm = 20000.0;
    double NearbyRangeCm = 1700.0;
    double FollowingRangeCm = 4000.0;
    double SideBySideLongitudinalCm = 500.0;
    double LaneOverlapCm = 320.0;
    double ContactMarginCm = 65.0;
    double ForecastHorizonSeconds = 1.5;
    double MinimumClosingCmS = 80.0;
    double MinimumOvertakePersistenceSeconds = 0.18;
    double MinimumStableIntervalSeconds = 0.05;
    double ContactTimeWarningSeconds = 1.0;
    double MaximumFrameSeconds = 0.10;
    bool RequireOnTrack = true;
    bool RequireCompletePhysicalRoster = true;
};

inline bool ValidOpponentSettings(
    const OpponentAnalysisSettings& s) noexcept
{
    return Finite(s.MaximumPhysicalSpeedCmS) &&
           s.MaximumPhysicalSpeedCmS > 0.0 &&
           Finite(s.MaximumRangeCm) &&
           s.MaximumRangeCm > s.NearbyRangeCm &&
           Finite(s.NearbyRangeCm) &&
           s.NearbyRangeCm > 0.0 &&
           Finite(s.FollowingRangeCm) &&
           s.FollowingRangeCm > s.NearbyRangeCm &&
           Finite(s.SideBySideLongitudinalCm) &&
           s.SideBySideLongitudinalCm > 0.0 &&
           Finite(s.LaneOverlapCm) &&
           s.LaneOverlapCm > 0.0 &&
           Finite(s.ContactMarginCm) &&
           s.ContactMarginCm >= 0.0 &&
           Finite(s.ForecastHorizonSeconds) &&
           s.ForecastHorizonSeconds > 0.0 &&
           s.ForecastHorizonSeconds <= 5.0 &&
           Finite(s.MinimumClosingCmS) &&
           s.MinimumClosingCmS >= 0.0 &&
           Finite(s.MinimumOvertakePersistenceSeconds) &&
           s.MinimumOvertakePersistenceSeconds > 0.0 &&
           Finite(s.MinimumStableIntervalSeconds) &&
           s.MinimumStableIntervalSeconds > 0.0 &&
           Finite(s.ContactTimeWarningSeconds) &&
           s.ContactTimeWarningSeconds > 0.0 &&
           Finite(s.MaximumFrameSeconds) &&
           s.MaximumFrameSeconds > 0.0 &&
           s.MaximumFrameSeconds <= 1.0;
}

inline bool ValidOpponentObservation(
    const OpponentObservation& o,
    const OpponentAnalysisSettings& settings) noexcept
{
    return o.EngineVerified &&
           o.ActorToken != 0 &&
           o.Slot >= 0 && o.Slot < MaximumTelemetryDrivers &&
           Finite(o.PositionCm) &&
           Finite(o.VelocityCmS) &&
           Finite(o.HeadingRadians) &&
           Finite(o.HalfLengthCm) &&
           Finite(o.HalfWidthCm) &&
           Finite(o.ChassisHeightCm) &&
           Length(o.VelocityCmS) <=
               settings.MaximumPhysicalSpeedCmS &&
           Length(o.PositionCm) <= MaximumWorldMagnitudeCm &&
           o.HalfLengthCm >= 50.0 &&
           o.HalfLengthCm <= 1000.0 &&
           o.HalfWidthCm >= 25.0 &&
           o.HalfWidthCm <= 500.0 &&
           o.ChassisHeightCm >= 25.0 &&
           o.ChassisHeightCm <= 500.0 &&
           (!settings.RequireOnTrack || o.OnPhysicalTrack);
}

struct OpponentEncounter final
{
    int Slot = -1;
    std::uint64_t ActorToken = 0;
    OpponentRelativePosition Relation =
        OpponentRelativePosition::Unknown;
    OpponentRiskLevel Risk = OpponentRiskLevel::Unknown;
    Vector2 RelativePositionCm{};
    Vector2 RelativeVelocityCmS{};
    Vector2 PredictedClosestRelativePositionCm{};
    double DistanceCm = 0.0;
    double LongitudinalCm = 0.0;
    double LateralCm = 0.0;
    double ClosingSpeedCmS = 0.0;
    double RelativeSpeedCmS = 0.0;
    double TimeToClosestSeconds = 0.0;
    double ClosestDistanceCm = 0.0;
    double TimeToContactSeconds =
        std::numeric_limits<double>::infinity();
    double ConservativeContactRadiusCm = 0.0;
    double CombinedHalfLengthCm = 0.0;
    double CombinedHalfWidthCm = 0.0;
    double TrackProgressDifferenceCm = 0.0;
    double RelativeBearingRadians = 0.0;
    double ReciprocalTimeToClosest = 0.0;
    bool CurrentOverlap = false;
    bool PredictedContact = false;
    bool IsAhead = false;
    bool IsBehind = false;
    bool SameNominalLane = false;
    bool CrossesCenterline = false;
    bool Qualified = false;
};

inline OpponentEncounter EvaluateOpponentEncounter(
    const OpponentObservation& self,
    const OpponentObservation& opponent,
    const OpponentAnalysisSettings& settings) noexcept
{
    OpponentEncounter result;
    if (!ValidOpponentSettings(settings) ||
        !ValidOpponentObservation(self, settings) ||
        !ValidOpponentObservation(opponent, settings) ||
        self.Slot == opponent.Slot ||
        self.ActorToken == opponent.ActorToken)
    {
        return result;
    }
    result.Slot = opponent.Slot;
    result.ActorToken = opponent.ActorToken;
    result.RelativePositionCm =
        opponent.PositionCm - self.PositionCm;
    result.RelativeVelocityCmS =
        opponent.VelocityCmS - self.VelocityCmS;
    result.DistanceCm = Length(result.RelativePositionCm);
    result.RelativeSpeedCmS = Length(result.RelativeVelocityCmS);
    if (!Finite(result.DistanceCm) ||
        result.DistanceCm > settings.MaximumRangeCm)
    {
        return result;
    }
    const Vector2 forward = HeadingUnit(self.HeadingRadians);
    const Vector2 right = PerpendicularRight(forward);
    result.LongitudinalCm = Dot(result.RelativePositionCm, forward);
    result.LateralCm = Dot(result.RelativePositionCm, right);
    result.IsAhead = result.LongitudinalCm >
        settings.SideBySideLongitudinalCm;
    result.IsBehind = result.LongitudinalCm <
        -settings.SideBySideLongitudinalCm;
    result.SameNominalLane = std::abs(result.LateralCm) <=
        settings.LaneOverlapCm;
    result.RelativeBearingRadians = std::atan2(
        Cross(forward, result.RelativePositionCm),
        Dot(forward, result.RelativePositionCm));
    result.ClosingSpeedCmS = -SafeRatio(
        Dot(result.RelativePositionCm, result.RelativeVelocityCmS),
        result.DistanceCm);
    result.CombinedHalfLengthCm =
        self.HalfLengthCm + opponent.HalfLengthCm;
    result.CombinedHalfWidthCm =
        self.HalfWidthCm + opponent.HalfWidthCm;
    result.ConservativeContactRadiusCm = std::hypot(
        result.CombinedHalfLengthCm,
        result.CombinedHalfWidthCm) + settings.ContactMarginCm;
    const double tClosest = result.RelativeSpeedCmS > Tiny ?
        -Dot(result.RelativePositionCm, result.RelativeVelocityCmS) /
        LengthSquared(result.RelativeVelocityCmS) : 0.0;
    result.TimeToClosestSeconds = std::clamp(
        tClosest, 0.0, settings.ForecastHorizonSeconds);
    result.PredictedClosestRelativePositionCm =
        result.RelativePositionCm +
        result.RelativeVelocityCmS * result.TimeToClosestSeconds;
    result.ClosestDistanceCm =
        Length(result.PredictedClosestRelativePositionCm);
    result.CurrentOverlap =
        result.DistanceCm <= result.ConservativeContactRadiusCm;
    result.PredictedContact =
        result.ClosestDistanceCm <=
        result.ConservativeContactRadiusCm;
    const double relativeSpeedSquared =
        LengthSquared(result.RelativeVelocityCmS);
    if (relativeSpeedSquared > Tiny)
    {
        const double radius = result.ConservativeContactRadiusCm;
        const double relativePositionSquared =
            LengthSquared(result.RelativePositionCm);
        const double b = Dot(
            result.RelativePositionCm,
            result.RelativeVelocityCmS);
        const double c = relativePositionSquared - radius * radius;
        const double discriminant =
            b * b - relativeSpeedSquared * c;
        if (c <= 0.0)
        {
            result.TimeToContactSeconds = 0.0;
        }
        else if (discriminant >= 0.0 && b < 0.0)
        {
            const double entry = (-b - std::sqrt(discriminant)) /
                relativeSpeedSquared;
            if (entry >= 0.0 &&
                entry <= settings.ForecastHorizonSeconds)
            {
                result.TimeToContactSeconds = entry;
            }
        }
    }
    if (result.CurrentOverlap)
    {
        result.Risk = OpponentRiskLevel::AlreadyOverlapping;
    }
    else if (result.PredictedContact &&
             Finite(result.TimeToContactSeconds))
    {
        result.Risk = OpponentRiskLevel::PredictedContact;
    }
    else if (result.DistanceCm < settings.NearbyRangeCm &&
             result.ClosingSpeedCmS > settings.MinimumClosingCmS)
    {
        result.Risk = OpponentRiskLevel::Closing;
    }
    else if (result.DistanceCm < settings.NearbyRangeCm)
    {
        result.Risk = OpponentRiskLevel::Nearby;
    }
    else
    {
        result.Risk = OpponentRiskLevel::Clear;
    }
    result.Relation = OpponentRelativePosition::Receding;
    if (result.IsAhead)
    {
        result.Relation = OpponentRelativePosition::FarAhead;
        if (result.SameNominalLane &&
            result.LongitudinalCm < settings.FollowingRangeCm)
        {
            result.Relation = OpponentRelativePosition::Following;
        }
    }
    else if (result.IsBehind)
    {
        result.Relation = OpponentRelativePosition::FarBehind;
    }
    else if (std::abs(result.LateralCm) <
             settings.LaneOverlapCm * 1.7)
    {
        result.Relation = OpponentRelativePosition::SideBySide;
    }
    if (result.ClosingSpeedCmS > settings.MinimumClosingCmS &&
        result.Relation != OpponentRelativePosition::SideBySide)
    {
        result.Relation = OpponentRelativePosition::Approaching;
    }
    result.CrossesCenterline =
        (result.LateralCm > 0.0) !=
        (result.LateralCm + Dot(
            result.RelativeVelocityCmS, right) *
            settings.ForecastHorizonSeconds > 0.0);
    if (result.CrossesCenterline &&
        result.PredictedContact)
    {
        result.Relation = OpponentRelativePosition::Crossing;
    }
    result.ReciprocalTimeToClosest = SafeRatio(
        1.0, std::max(0.01, result.TimeToClosestSeconds));
    result.Qualified = true;
    return result;
}

struct OpponentInteractionRecord final
{
    int Slot = -1;
    std::uint64_t ActorToken = 0;
    OpponentEncounter Latest{};
    OpponentRiskLevel PeakRisk = OpponentRiskLevel::Unknown;
    double FirstSeenSeconds = 0.0;
    double LastSeenSeconds = 0.0;
    double NearestDistanceCm =
        std::numeric_limits<double>::infinity();
    double ClosestApproachSeconds = 0.0;
    double TotalNearbySeconds = 0.0;
    double TotalClosingSeconds = 0.0;
    double TotalSideBySideSeconds = 0.0;
    double TotalPredictedContactSeconds = 0.0;
    double MaximumClosingSpeedCmS = 0.0;
    double MaximumRelativeSpeedCmS = 0.0;
    double OvertakeCandidateSeconds = 0.0;
    int NearbyEpisodes = 0;
    int CrossingEpisodes = 0;
    int ConfirmedPasses = 0;
    int ConfirmedPositionLosses = 0;
    std::uint64_t Frames = 0;
    bool WasNearby = false;
    bool WasCrossing = false;
    bool LastAhead = false;
    bool HasLastAhead = false;
    bool Qualified = false;
};

struct FleetTelemetrySnapshot final
{
    std::array<OpponentInteractionRecord,
               MaximumTelemetryDrivers> Opponents{};
    std::array<OpponentEncounter,
               MaximumTelemetryDrivers - 1> LatestEncounters{};
    double NearestDistanceCm =
        std::numeric_limits<double>::infinity();
    double EarliestPredictedContactSeconds =
        std::numeric_limits<double>::infinity();
    double MaximumClosingSpeedCmS = 0.0;
    int NearestSlot = -1;
    int MostUrgentSlot = -1;
    int QualifiedOpponents = 0;
    int NearbyOpponents = 0;
    int PredictedContacts = 0;
    int SideBySideOpponents = 0;
    int TotalConfirmedPasses = 0;
    int TotalConfirmedPositionLosses = 0;
    std::uint64_t QualifiedFleetFrames = 0;
    std::uint64_t RejectedFleetFrames = 0;
    bool CompletePhysicalRoster = false;
    bool Valid = false;
};

class FleetTelemetryMonitor final
{
public:
    bool Configure(OpponentAnalysisSettings settings) noexcept
    {
        if (!ValidOpponentSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Previous.fill({});
        HasPrevious.fill(false);
        Last = {};
        LastCompleted = {};
        QualifiedFrames = 0;
        RejectedFrames = 0;
        Session = 0;
        Subject = 0;
        SubjectSlot = -1;
    }

    void Suspend() noexcept
    {
        Previous.fill({});
        HasPrevious.fill(false);
        Last = {};
    }

    bool Observe(
        const OpponentObservation& self,
        const OpponentObservation* others,
        int count, std::uint64_t sessionToken,
        double monotonicSeconds, double dt) noexcept
    {
        if (!ValidOpponentSettings(Settings) ||
            !others || count < 0 ||
            count >= MaximumTelemetryDrivers ||
            !Finite(monotonicSeconds) ||
            !Finite(dt) || dt <= 0.0 ||
            dt > Settings.MaximumFrameSeconds ||
            sessionToken == 0 ||
            !ValidOpponentObservation(self, Settings))
        {
            RejectedFrames = SaturatingIncrement(RejectedFrames);
            Suspend();
            return false;
        }
        if (Settings.RequireCompletePhysicalRoster &&
            count != MaximumTelemetryDrivers - 1)
        {
            RejectedFrames = SaturatingIncrement(RejectedFrames);
            Suspend();
            return false;
        }
        if (Session != sessionToken ||
            Subject != self.ActorToken || SubjectSlot != self.Slot)
        {
            Reset();
            Session = sessionToken;
            Subject = self.ActorToken;
            SubjectSlot = self.Slot;
        }
        std::array<bool, MaximumTelemetryDrivers> present{};
        present[static_cast<std::size_t>(self.Slot)] = true;
        FleetTelemetrySnapshot out;
        out.Opponents = LastCompleted;
        out.QualifiedFleetFrames =
            SaturatingIncrement(QualifiedFrames);
        out.RejectedFleetFrames = RejectedFrames;
        for (int i = 0; i < count; ++i)
        {
            const OpponentObservation& other =
                others[static_cast<std::size_t>(i)];
            if (!ValidOpponentObservation(other, Settings) ||
                other.Slot == self.Slot ||
                other.ActorToken == self.ActorToken ||
                present[static_cast<std::size_t>(other.Slot)])
            {
                RejectedFrames = SaturatingIncrement(RejectedFrames);
                Suspend();
                return false;
            }
            present[static_cast<std::size_t>(other.Slot)] = true;
            for (int prior = 0; prior < i; ++prior)
            {
                if (other.ActorToken ==
                    others[static_cast<std::size_t>(prior)].ActorToken)
                {
                    RejectedFrames = SaturatingIncrement(RejectedFrames);
                    Suspend();
                    return false;
                }
            }
        }
        for (int i = 0; i < count; ++i)
        {
            const OpponentObservation& other =
                others[static_cast<std::size_t>(i)];
            const OpponentEncounter encounter =
                EvaluateOpponentEncounter(self, other, Settings);
            if (!encounter.Qualified)
            {
                RejectedFrames = SaturatingIncrement(RejectedFrames);
                Suspend();
                return false;
            }
            const std::size_t slot =
                static_cast<std::size_t>(other.Slot);
            OpponentInteractionRecord previous = LastCompleted[slot];
            if (!HasPrevious[slot] ||
                previous.ActorToken != other.ActorToken)
            {
                previous = {};
                previous.Slot = other.Slot;
                previous.ActorToken = other.ActorToken;
                previous.FirstSeenSeconds = monotonicSeconds;
            }
            previous.Latest = encounter;
            previous.LastSeenSeconds = monotonicSeconds;
            previous.Frames = SaturatingIncrement(previous.Frames);
            previous.Qualified = true;
            previous.PeakRisk =
                static_cast<int>(encounter.Risk) >
                    static_cast<int>(previous.PeakRisk) ?
                encounter.Risk : previous.PeakRisk;
            if (encounter.DistanceCm < previous.NearestDistanceCm)
            {
                previous.NearestDistanceCm = encounter.DistanceCm;
                previous.ClosestApproachSeconds = monotonicSeconds;
            }
            previous.MaximumClosingSpeedCmS = std::max(
                previous.MaximumClosingSpeedCmS,
                encounter.ClosingSpeedCmS);
            previous.MaximumRelativeSpeedCmS = std::max(
                previous.MaximumRelativeSpeedCmS,
                encounter.RelativeSpeedCmS);
            const bool nearby =
                encounter.DistanceCm <= Settings.NearbyRangeCm;
            const bool crossing = encounter.CrossesCenterline &&
                encounter.PredictedContact;
            if (nearby)
            {
                previous.TotalNearbySeconds += dt;
                if (!previous.WasNearby)
                {
                    ++previous.NearbyEpisodes;
                }
            }
            if (encounter.ClosingSpeedCmS > Settings.MinimumClosingCmS)
            {
                previous.TotalClosingSeconds += dt;
            }
            if (encounter.Relation ==
                OpponentRelativePosition::SideBySide)
            {
                previous.TotalSideBySideSeconds += dt;
            }
            if (encounter.PredictedContact)
            {
                previous.TotalPredictedContactSeconds += dt;
            }
            if (crossing && !previous.WasCrossing)
            {
                ++previous.CrossingEpisodes;
            }
            previous.WasNearby = nearby;
            previous.WasCrossing = crossing;
            const bool ahead =
                encounter.LongitudinalCm >
                Settings.SideBySideLongitudinalCm;
            const bool behind =
                encounter.LongitudinalCm <
                -Settings.SideBySideLongitudinalCm;
            if ((ahead || behind) && previous.HasLastAhead &&
                previous.LastAhead != ahead && nearby)
            {
                previous.OvertakeCandidateSeconds += dt;
                if (previous.OvertakeCandidateSeconds >=
                    Settings.MinimumOvertakePersistenceSeconds)
                {
                    if (previous.LastAhead && behind)
                    {
                        ++previous.ConfirmedPasses;
                    }
                    else if (!previous.LastAhead && ahead)
                    {
                        ++previous.ConfirmedPositionLosses;
                    }
                    previous.HasLastAhead = false;
                    previous.OvertakeCandidateSeconds = 0.0;
                }
            }
            else if (ahead || behind)
            {
                previous.LastAhead = ahead;
                previous.HasLastAhead = true;
                previous.OvertakeCandidateSeconds = 0.0;
            }
            out.Opponents[slot] = previous;
            HasPrevious[slot] = true;
            out.LatestEncounters[static_cast<std::size_t>(i)] =
                encounter;
            ++out.QualifiedOpponents;
            if (nearby)
            {
                ++out.NearbyOpponents;
            }
            if (encounter.PredictedContact)
            {
                ++out.PredictedContacts;
            }
            if (encounter.Relation ==
                OpponentRelativePosition::SideBySide)
            {
                ++out.SideBySideOpponents;
            }
            if (encounter.DistanceCm < out.NearestDistanceCm)
            {
                out.NearestDistanceCm = encounter.DistanceCm;
                out.NearestSlot = other.Slot;
            }
            if (encounter.TimeToContactSeconds <
                out.EarliestPredictedContactSeconds)
            {
                out.EarliestPredictedContactSeconds =
                    encounter.TimeToContactSeconds;
                out.MostUrgentSlot = other.Slot;
            }
            out.MaximumClosingSpeedCmS = std::max(
                out.MaximumClosingSpeedCmS,
                encounter.ClosingSpeedCmS);
            out.TotalConfirmedPasses += previous.ConfirmedPasses;
            out.TotalConfirmedPositionLosses +=
                previous.ConfirmedPositionLosses;
        }
        out.CompletePhysicalRoster =
            count == MaximumTelemetryDrivers - 1;
        out.Valid = true;
        QualifiedFrames = SaturatingIncrement(QualifiedFrames);
        out.QualifiedFleetFrames = QualifiedFrames;
        LastCompleted = out.Opponents;
        Last = out;
        return true;
    }

    FleetTelemetrySnapshot Snapshot() const noexcept { return Last; }
    std::uint64_t AcceptedFrames() const noexcept
    {
        return QualifiedFrames;
    }
    std::uint64_t RejectedObservations() const noexcept
    {
        return RejectedFrames;
    }
    std::uint64_t SessionToken() const noexcept { return Session; }
    int OwnSlot() const noexcept { return SubjectSlot; }

private:
    OpponentAnalysisSettings Settings{};
    std::array<OpponentObservation, MaximumTelemetryDrivers> Previous{};
    std::array<bool, MaximumTelemetryDrivers> HasPrevious{};
    std::array<OpponentInteractionRecord,
               MaximumTelemetryDrivers> LastCompleted{};
    FleetTelemetrySnapshot Last{};
    std::uint64_t QualifiedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    std::uint64_t Session = 0;
    std::uint64_t Subject = 0;
    int SubjectSlot = -1;
};
} // namespace latebrake::engineering
