#pragma once

#include "Telemetry/TelemetrySessionRecorder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class SessionIntegrityFault : std::uint8_t
{
    None,
    MissingPhysics,
    MissingActor,
    SessionChanged,
    DuplicateFrame,
    ReversedClock,
    InconsistentClock,
    ImpossibleTravel,
    NonFiniteState,
    InvalidControl,
    MissingWheelSupport,
    UnexpectedOfficialResult
};

struct SessionIntegritySettings final
{
    double MaximumFrameGapSeconds = 0.25;
    double MaximumRaceClockGapSeconds = 0.30;
    double MaximumClockDifferenceSeconds = 0.12;
    double MaximumWorldJumpCm = 1000.0;
    double MaximumSpeedCmS = MaximumPlanarSpeedCmS;
    double MaximumAccelerationCmS2 = 9000.0;
    double MaximumAngularSpeedRadS = 30.0;
    double MinimumWheelContactFraction = 0.50;
    double MaximumInputStalenessSeconds = 0.35;
    double MaximumUnexpectedLapJump = 1.0;
    double MaximumGateJump = 1.0;
    bool RequireSameActor = true;
    bool RequireSameSessionRevision = true;
    bool RequireStrictFrameOrdinal = true;
    bool RequireActivePhysics = true;
    bool RequireOriginalWheelContact = false;
    bool AllowRaceClockPause = true;
    bool AllowOfficialResultStatus = true;
};

inline bool ValidSessionIntegritySettings(
    SessionIntegritySettings s) noexcept
{
    return Finite(s.MaximumFrameGapSeconds) &&
        s.MaximumFrameGapSeconds > 0.0 &&
        s.MaximumFrameGapSeconds <= 10.0 &&
        Finite(s.MaximumRaceClockGapSeconds) &&
        s.MaximumRaceClockGapSeconds > 0.0 &&
        Finite(s.MaximumClockDifferenceSeconds) &&
        s.MaximumClockDifferenceSeconds >= 0.0 &&
        Finite(s.MaximumWorldJumpCm) &&
        s.MaximumWorldJumpCm > 0.0 &&
        Finite(s.MaximumSpeedCmS) &&
        s.MaximumSpeedCmS > 0.0 &&
        Finite(s.MaximumAccelerationCmS2) &&
        s.MaximumAccelerationCmS2 > 0.0 &&
        Finite(s.MaximumAngularSpeedRadS) &&
        s.MaximumAngularSpeedRadS > 0.0 &&
        Finite(s.MinimumWheelContactFraction) &&
        s.MinimumWheelContactFraction >= 0.0 &&
        s.MinimumWheelContactFraction <= 1.0 &&
        Finite(s.MaximumInputStalenessSeconds) &&
        s.MaximumInputStalenessSeconds >= 0.0 &&
        Finite(s.MaximumUnexpectedLapJump) &&
        s.MaximumUnexpectedLapJump >= 0.0 &&
        Finite(s.MaximumGateJump) &&
        s.MaximumGateJump >= 0.0;
}

struct SessionIntegrityDecision final
{
    SessionIntegrityFault PrimaryFault =
        SessionIntegrityFault::None;
    double FrameGapSeconds = 0.0;
    double RaceClockGapSeconds = 0.0;
    double ClockDiscrepancySeconds = 0.0;
    double TravelCm = 0.0;
    double ImpliedSpeedCmS = 0.0;
    double ReportedSpeedCmS = 0.0;
    double SpeedDifferenceCmS = 0.0;
    double ReportedAccelerationCmS2 = 0.0;
    double YawDeltaRadians = 0.0;
    double InputStalenessSeconds = 0.0;
    int LapJump = 0;
    int GateJump = 0;
    bool ResetReplayFilters = false;
    bool SessionEpochChanged = false;
    bool ActorIdentityChanged = false;
    bool ExactFrameDuplicate = false;
    bool PhysicsObserved = false;
    bool Qualified = false;
    bool ReadOnly = true;
};

inline bool FiniteSessionPhysics(
    const TelemetryFrame& frame,
    SessionIntegritySettings settings) noexcept
{
    const TelemetryDynamics& d = frame.Dynamics;
    return Finite(frame.MonotonicSeconds) &&
        Finite(frame.RaceClockSeconds) &&
        Finite(frame.DeltaSeconds) &&
        Finite(d.WorldPositionCm) &&
        Finite(d.WorldVelocityCmS) &&
        Finite(d.WorldAccelerationCmS2) &&
        Finite(d.AngularVelocityRadS) &&
        Finite(d.YawRadians) &&
        Finite(frame.Controls.Throttle) &&
        Finite(frame.Controls.Brake) &&
        Finite(frame.Controls.Steering) &&
        frame.Controls.Throttle >= 0.0 &&
        frame.Controls.Throttle <= 1.0 &&
        frame.Controls.Brake >= 0.0 &&
        frame.Controls.Brake <= 1.0 &&
        frame.Controls.Steering >= -1.0 &&
        frame.Controls.Steering <= 1.0 &&
        Length(d.WorldVelocityCmS) <=
            settings.MaximumSpeedCmS &&
        Length(d.WorldAccelerationCmS2) <=
            settings.MaximumAccelerationCmS2 &&
        Length(d.AngularVelocityRadS) <=
            settings.MaximumAngularSpeedRadS &&
        frame.MonotonicSeconds >= 0.0 &&
        frame.RaceClockSeconds >= 0.0;
}

inline SessionIntegrityDecision EvaluateSessionIntegrity(
    const TelemetryFrame& frame,
    const TelemetryFrame* previous,
    SessionIntegritySettings settings = {}) noexcept
{
    SessionIntegrityDecision result;
    if (!ValidSessionIntegritySettings(settings))
    {
        result.PrimaryFault = SessionIntegrityFault::NonFiniteState;
        return result;
    }
    if (settings.RequireActivePhysics &&
        (!frame.Race.EnginePhysicsReady ||
         !IsEngineObservation(frame.Identity.Trust)))
    {
        result.PrimaryFault = SessionIntegrityFault::MissingPhysics;
        return result;
    }
    if (frame.Identity.SessionToken == 0 ||
        frame.Identity.ActorToken == 0 ||
        frame.Identity.DriverSlot < 0 ||
        frame.Identity.DriverSlot >= MaximumTelemetryDrivers)
    {
        result.PrimaryFault = SessionIntegrityFault::MissingActor;
        return result;
    }
    if (!FiniteSessionPhysics(frame, settings))
    {
        result.PrimaryFault = SessionIntegrityFault::NonFiniteState;
        return result;
    }
    if (settings.RequireOriginalWheelContact &&
        WheelContactFraction(frame.Wheels) <
        settings.MinimumWheelContactFraction)
    {
        result.PrimaryFault =
            SessionIntegrityFault::MissingWheelSupport;
        return result;
    }
    if (!settings.AllowOfficialResultStatus &&
        frame.Race.RaceResultOfficial)
    {
        result.PrimaryFault =
            SessionIntegrityFault::UnexpectedOfficialResult;
        return result;
    }
    result.PhysicsObserved = true;
    result.ReportedSpeedCmS =
        Length(frame.Dynamics.WorldVelocityCmS);
    result.ReportedAccelerationCmS2 =
        Length(frame.Dynamics.WorldAccelerationCmS2);
    result.InputStalenessSeconds =
        frame.InputLatencySeconds;
    if (result.InputStalenessSeconds >
        settings.MaximumInputStalenessSeconds)
    {
        result.PrimaryFault = SessionIntegrityFault::InvalidControl;
        return result;
    }
    if (!previous)
    {
        result.ResetReplayFilters = true;
        result.Qualified = true;
        return result;
    }
    result.SessionEpochChanged =
        frame.Identity.SessionToken !=
            previous->Identity.SessionToken ||
        frame.Identity.SessionRevision !=
            previous->Identity.SessionRevision;
    result.ActorIdentityChanged =
        frame.Identity.ActorToken !=
            previous->Identity.ActorToken ||
        frame.Identity.DriverSlot !=
            previous->Identity.DriverSlot;
    if (result.SessionEpochChanged &&
        settings.RequireSameSessionRevision)
    {
        result.PrimaryFault =
            SessionIntegrityFault::SessionChanged;
        result.ResetReplayFilters = true;
        return result;
    }
    if (result.ActorIdentityChanged &&
        settings.RequireSameActor)
    {
        result.PrimaryFault = SessionIntegrityFault::MissingActor;
        result.ResetReplayFilters = true;
        return result;
    }
    result.ExactFrameDuplicate =
        frame.Identity.FrameOrdinal ==
            previous->Identity.FrameOrdinal;
    if (settings.RequireStrictFrameOrdinal &&
        frame.Identity.FrameOrdinal <=
            previous->Identity.FrameOrdinal)
    {
        result.PrimaryFault =
            SessionIntegrityFault::DuplicateFrame;
        return result;
    }
    result.FrameGapSeconds =
        frame.MonotonicSeconds -
        previous->MonotonicSeconds;
    result.RaceClockGapSeconds =
        frame.RaceClockSeconds -
        previous->RaceClockSeconds;
    if (result.FrameGapSeconds <= 0.0)
    {
        result.PrimaryFault =
            SessionIntegrityFault::ReversedClock;
        result.ResetReplayFilters = true;
        return result;
    }
    if (result.FrameGapSeconds >
            settings.MaximumFrameGapSeconds ||
        result.RaceClockGapSeconds <
            -settings.MaximumClockDifferenceSeconds ||
        result.RaceClockGapSeconds >
            settings.MaximumRaceClockGapSeconds)
    {
        result.PrimaryFault =
            SessionIntegrityFault::InconsistentClock;
        result.ResetReplayFilters = true;
        return result;
    }
    result.ClockDiscrepancySeconds =
        std::abs(result.FrameGapSeconds -
                 result.RaceClockGapSeconds);
    if (!settings.AllowRaceClockPause &&
        result.ClockDiscrepancySeconds >
            settings.MaximumClockDifferenceSeconds)
    {
        result.PrimaryFault =
            SessionIntegrityFault::InconsistentClock;
        return result;
    }
    const Vector3 displacement =
        frame.Dynamics.WorldPositionCm -
        previous->Dynamics.WorldPositionCm;
    result.TravelCm = Length(displacement);
    result.ImpliedSpeedCmS = SafeRatio(
        result.TravelCm, result.FrameGapSeconds);
    result.SpeedDifferenceCmS =
        std::abs(result.ReportedSpeedCmS -
                 result.ImpliedSpeedCmS);
    if (!Finite(result.TravelCm) ||
        result.TravelCm >
            settings.MaximumWorldJumpCm ||
        result.ImpliedSpeedCmS >
            settings.MaximumSpeedCmS)
    {
        result.PrimaryFault =
            SessionIntegrityFault::ImpossibleTravel;
        result.ResetReplayFilters = true;
        return result;
    }
    result.YawDeltaRadians = AngleDifference(
        previous->Dynamics.YawRadians,
        frame.Dynamics.YawRadians);
    if (std::abs(result.YawDeltaRadians) >
        settings.MaximumAngularSpeedRadS *
        result.FrameGapSeconds + 0.2)
    {
        result.PrimaryFault =
            SessionIntegrityFault::ImpossibleTravel;
        return result;
    }
    result.LapJump = frame.Race.LapIndex -
        previous->Race.LapIndex;
    result.GateJump = frame.Race.CreditedGateOrdinal -
        previous->Race.CreditedGateOrdinal;
    if (result.LapJump < 0 ||
        result.LapJump >
            settings.MaximumUnexpectedLapJump ||
        result.GateJump < 0 ||
        result.GateJump >
            settings.MaximumGateJump)
    {
        result.PrimaryFault =
            SessionIntegrityFault::UnexpectedOfficialResult;
        return result;
    }
    result.Qualified = true;
    return result;
}

struct SessionIntegrityReport final
{
    std::array<std::uint64_t, 12> FaultCounts{};
    std::uint64_t AcceptedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    std::uint64_t SegmentResets = 0;
    std::uint64_t UniqueEpochs = 0;
    std::uint64_t ActorReplacements = 0;
    std::uint64_t DuplicateFrames = 0;
    double LastAcceptedTimestampSeconds = 0.0;
    double TotalQualifiedSeconds = 0.0;
    double TotalQualifiedDistanceCm = 0.0;
    double MaximumObservedGapSeconds = 0.0;
    double MaximumObservedTravelCm = 0.0;
    double MaximumSpeedMismatchCmS = 0.0;
    double AverageFrameGapSeconds = 0.0;
    double AverageClockDifferenceSeconds = 0.0;
    double QualityFraction = 0.0;
    bool HasAcceptedEngineSample = false;
    bool OfficialTimeAuthority = false;
};

class TelemetrySessionIntegrityMonitor final
{
public:
    void Reset() noexcept
    {
        Previous = {};
        HasPrevious = false;
        Settings = {};
        Report = {};
    }

    bool Configure(SessionIntegritySettings settings) noexcept
    {
        if (!ValidSessionIntegritySettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    SessionIntegrityDecision Observe(
        const TelemetryFrame& frame) noexcept
    {
        const SessionIntegrityDecision decision =
            EvaluateSessionIntegrity(
                frame, HasPrevious ? &Previous : nullptr, Settings);
        const std::size_t reason =
            static_cast<std::size_t>(decision.PrimaryFault);
        if (reason < Report.FaultCounts.size())
        {
            Report.FaultCounts[reason] =
                SaturatingIncrement(Report.FaultCounts[reason]);
        }
        if (!decision.Qualified)
        {
            ++Report.RejectedFrames;
            if (decision.ResetReplayFilters)
            {
                ++Report.SegmentResets;
                HasPrevious = false;
            }
        }
        else
        {
            ++Report.AcceptedFrames;
            if (!HasPrevious)
            {
                ++Report.UniqueEpochs;
            }
            if (decision.SessionEpochChanged)
            {
                ++Report.UniqueEpochs;
            }
            if (decision.ActorIdentityChanged)
            {
                ++Report.ActorReplacements;
            }
            Report.TotalQualifiedSeconds +=
                decision.FrameGapSeconds;
            Report.TotalQualifiedDistanceCm +=
                decision.TravelCm;
            Report.MaximumObservedGapSeconds = std::max(
                Report.MaximumObservedGapSeconds,
                decision.FrameGapSeconds);
            Report.MaximumObservedTravelCm = std::max(
                Report.MaximumObservedTravelCm,
                decision.TravelCm);
            Report.MaximumSpeedMismatchCmS = std::max(
                Report.MaximumSpeedMismatchCmS,
                decision.SpeedDifferenceCmS);
            Report.LastAcceptedTimestampSeconds =
                frame.MonotonicSeconds;
            Report.HasAcceptedEngineSample = true;
            Previous = frame;
            HasPrevious = true;
        }
        Report.DuplicateFrames +=
            decision.ExactFrameDuplicate ? 1 : 0;
        Report.AverageFrameGapSeconds = SafeRatio(
            Report.TotalQualifiedSeconds,
            static_cast<double>(Report.AcceptedFrames));
        Report.QualityFraction = SafeRatio(
            static_cast<double>(Report.AcceptedFrames),
            static_cast<double>(Report.AcceptedFrames) +
            static_cast<double>(Report.RejectedFrames));
        Report.OfficialTimeAuthority = false;
        return decision;
    }

    SessionIntegrityReport Snapshot() const noexcept
    {
        return Report;
    }

    bool HasQualifiedBaseline() const noexcept
    {
        return HasPrevious;
    }

private:
    SessionIntegritySettings Settings{};
    TelemetryFrame Previous{};
    SessionIntegrityReport Report{};
    bool HasPrevious = false;
};
} // namespace latebrake::engineering
