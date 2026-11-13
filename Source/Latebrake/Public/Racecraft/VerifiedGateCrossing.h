#pragma once
#include "Racecraft/RacecraftFrameGate.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::racecraft
{
struct GateCrossingParameters final
{
    double GateHalfWidthCm = 800.0;
    double MaximumChassisHalfWidthCm = 180.0;
    double RequiredForwardNormalCmS = 120.0;
    double MinimumSignedCrossingCm = 5.0;
    double MaximumCrossingDistanceCm = 1500.0;
    double MaximumCrossingTimeSeconds = 0.15;
    double MaximumClockToleranceSeconds = 0.02;
    double MaximumReverseVelocityCmS = 75.0;
    double MinimumBetweenGatesSeconds = 1.25;
    double MaximumLongitudinalJumpCm = 1800.0;
    double MaximumUnverifiedTeleportCm = 400.0;
    double FinishStartAllowedOffsetCm = 20.0;
    bool RequireOriginalPhysicalRoad = true;
    bool RequireSameActor = true;
    bool RequireDirection = true;
    bool RejectRecoveryCrossing = true;
};
struct GateCrossingEvidence final
{
    RaceIdentity Identity{};
    Pose2 Before{};
    Pose2 After{};
    Vec2 BeforeVelocityCmS{};
    Vec2 AfterVelocityCmS{};
    double BeforeSeconds = 0.0;
    double AfterSeconds = 0.0;
    double BeforeRaceClockSeconds = 0.0;
    double AfterRaceClockSeconds = 0.0;
    double CrossingFraction = 0.0;
    double CrossingTimestampSeconds = 0.0;
    double CrossingRaceClockSeconds = 0.0;
    double LateralAtCrossingCm = 0.0;
    double ForwardSpeedAtCrossingCmS = 0.0;
    double ChassisHalfWidthCm = 100.0;
    int ExpectedGate = -1;
    bool BeforeOnPavement = false;
    bool AfterOnPavement = false;
    bool EnginePhysicsQualified = false;
    bool RecoveryInProgress = false;
    bool ActualGatePlaneCrossed = false;
};
enum class GateCrossingFault : std::uint8_t
{
    None, NotPhysical, WrongGate, WrongDirection,
    OffPavement, OutsideGate, StaleFrame,
    WrongActor, CrossingTooFast, Recovery,
    MissingCrossing, UnqualifiedBodyWidth
};
struct GateCrossingResult final
{
    GateCrossingFault Fault = GateCrossingFault::NotPhysical;
    GateCrossingEvidence Evidence{};
    int PassedGate = -1;
    double CrossingSeconds = 0.0;
    double CrossingClockSeconds = 0.0;
    double CrossingForwardSpeedCmS = 0.0;
    double LateralGateErrorCm = 0.0;
    double DistanceTraversedCm = 0.0;
    bool IsFinalGate = false;
    bool AuthoritativeGatePassed = false;
    bool Valid = false;
};
inline bool ValidGateParameters(GateCrossingParameters s) noexcept
{
    return Finite(s.GateHalfWidthCm) &&
        s.GateHalfWidthCm > 0.0 &&
        Finite(s.MaximumChassisHalfWidthCm) &&
        s.MaximumChassisHalfWidthCm > 0.0 &&
        s.MaximumChassisHalfWidthCm < s.GateHalfWidthCm &&
        Finite(s.RequiredForwardNormalCmS) &&
        s.RequiredForwardNormalCmS > 0.0 &&
        Finite(s.MinimumSignedCrossingCm) &&
        s.MinimumSignedCrossingCm >= 0.0 &&
        Finite(s.MaximumCrossingDistanceCm) &&
        s.MaximumCrossingDistanceCm > 0.0 &&
        Finite(s.MaximumCrossingTimeSeconds) &&
        s.MaximumCrossingTimeSeconds > 0.0 &&
        Finite(s.MaximumClockToleranceSeconds) &&
        s.MaximumClockToleranceSeconds >= 0.0 &&
        Finite(s.MaximumReverseVelocityCmS) &&
        s.MaximumReverseVelocityCmS >= 0.0 &&
        Finite(s.MinimumBetweenGatesSeconds) &&
        s.MinimumBetweenGatesSeconds >= 0.0 &&
        Finite(s.MaximumLongitudinalJumpCm) &&
        s.MaximumLongitudinalJumpCm >
            s.MinimumSignedCrossingCm &&
        Finite(s.MaximumUnverifiedTeleportCm) &&
        s.MaximumUnverifiedTeleportCm > 0.0;
}
inline Segment2 PhysicalGateLine(int gateIndex,
                                  double halfWidthCm) noexcept
{
    const CircuitSegment segment = SegmentAt(gateIndex);
    if (!ValidCircuitSegment(segment) ||
        !Finite(halfWidthCm) || halfWidthCm <= 0.0)
        return {};
    const Vec2 axis =
        PerpendicularLeft(HeadingVector(
            segment.HeadingRadians));
    return {
        segment.Centerline.B - axis * halfWidthCm,
        segment.Centerline.B + axis * halfWidthCm
    };
}
inline double SignedDistanceFromGate(
    Vec2 position, int gateIndex) noexcept
{
    const CircuitSegment segment = SegmentAt(gateIndex);
    if (!ValidCircuitSegment(segment) ||
        !Finite(position)) return 0.0;
    return Dot(position - segment.Centerline.B,
               HeadingVector(segment.HeadingRadians));
}
inline double LateralPositionAtGate(
    Vec2 position, int gateIndex) noexcept
{
    const CircuitSegment segment = SegmentAt(gateIndex);
    if (!ValidCircuitSegment(segment) ||
        !Finite(position)) return 0.0;
    return Cross(
        HeadingVector(segment.HeadingRadians),
        position - segment.Centerline.B);
}
inline GateCrossingResult EvaluateGateCrossing(
    GateCrossingEvidence evidence,
    GateCrossingParameters settings = {}) noexcept
{
    GateCrossingResult out;
    out.Evidence = evidence;
    if (!ValidGateParameters(settings) ||
        evidence.ExpectedGate < 0 ||
        evidence.ExpectedGate >= OriginalSegmentCount)
    {
        out.Fault = GateCrossingFault::WrongGate;
        return out;
    }
    if (!PhysicalActorIdentityValid(evidence.Identity) ||
        !evidence.EnginePhysicsQualified ||
        !evidence.ActualGatePlaneCrossed)
    {
        out.Fault = GateCrossingFault::NotPhysical;
        return out;
    }
    if (evidence.RecoveryInProgress &&
        settings.RejectRecoveryCrossing)
    {
        out.Fault = GateCrossingFault::Recovery;
        return out;
    }
    if (!Finite(evidence.Before) ||
        !Finite(evidence.After) ||
        !Finite(evidence.BeforeVelocityCmS) ||
        !Finite(evidence.AfterVelocityCmS) ||
        !Finite(evidence.BeforeSeconds) ||
        !Finite(evidence.AfterSeconds) ||
        !Finite(evidence.BeforeRaceClockSeconds) ||
        !Finite(evidence.AfterRaceClockSeconds) ||
        !Finite(evidence.ChassisHalfWidthCm) ||
        evidence.ChassisHalfWidthCm <= 0.0 ||
        evidence.ChassisHalfWidthCm >
            settings.MaximumChassisHalfWidthCm)
    {
        out.Fault = GateCrossingFault::UnqualifiedBodyWidth;
        return out;
    }
    const double dt = evidence.AfterSeconds -
        evidence.BeforeSeconds;
    if (dt <= 0.0 ||
        dt > settings.MaximumCrossingTimeSeconds ||
        evidence.AfterRaceClockSeconds <
            evidence.BeforeRaceClockSeconds ||
        std::abs((evidence.AfterRaceClockSeconds -
                  evidence.BeforeRaceClockSeconds) - dt) >
            settings.MaximumClockToleranceSeconds)
    {
        out.Fault = GateCrossingFault::StaleFrame;
        return out;
    }
    if (settings.RequireOriginalPhysicalRoad &&
        (!evidence.BeforeOnPavement ||
         !evidence.AfterOnPavement ||
         !CenterOnOriginalRoad(evidence.Before.Position) ||
         !CenterOnOriginalRoad(evidence.After.Position)))
    {
        out.Fault = GateCrossingFault::OffPavement;
        return out;
    }
    const double from = SignedDistanceFromGate(
        evidence.Before.Position, evidence.ExpectedGate);
    const double to = SignedDistanceFromGate(
        evidence.After.Position, evidence.ExpectedGate);
    if (from > -settings.MinimumSignedCrossingCm ||
        to < settings.MinimumSignedCrossingCm)
    {
        out.Fault = GateCrossingFault::MissingCrossing;
        return out;
    }
    const double denominator = to - from;
    if (denominator <= PhysicsEpsilon)
    {
        out.Fault = GateCrossingFault::WrongDirection;
        return out;
    }
    const double fraction =
        std::clamp(-from / denominator, 0.0, 1.0);
    const Vec2 position = Lerp(
        evidence.Before.Position,
        evidence.After.Position, fraction);
    const double lateral = LateralPositionAtGate(
        position, evidence.ExpectedGate);
    const double gateClearance =
        settings.GateHalfWidthCm -
        evidence.ChassisHalfWidthCm;
    if (std::abs(lateral) > gateClearance)
    {
        out.Fault = GateCrossingFault::OutsideGate;
        return out;
    }
    const Vec2 direction = HeadingVector(
        SegmentAt(evidence.ExpectedGate).HeadingRadians);
    const double speed = Lerp(
        Dot(evidence.BeforeVelocityCmS, direction),
        Dot(evidence.AfterVelocityCmS, direction),
        fraction);
    if (settings.RequireDirection &&
        speed < settings.RequiredForwardNormalCmS)
    {
        out.Fault = GateCrossingFault::WrongDirection;
        return out;
    }
    const double displacement = Distance(
        evidence.Before.Position,
        evidence.After.Position);
    if (displacement > settings.MaximumCrossingDistanceCm ||
        displacement > settings.MaximumLongitudinalJumpCm ||
        displacement > settings.MaximumUnverifiedTeleportCm)
    {
        out.Fault = GateCrossingFault::CrossingTooFast;
        return out;
    }
    out.CrossingSeconds = Lerp(
        evidence.BeforeSeconds,
        evidence.AfterSeconds, fraction);
    out.CrossingClockSeconds = Lerp(
        evidence.BeforeRaceClockSeconds,
        evidence.AfterRaceClockSeconds, fraction);
    out.CrossingForwardSpeedCmS = speed;
    out.LateralGateErrorCm = lateral;
    out.DistanceTraversedCm = displacement;
    out.PassedGate = evidence.ExpectedGate;
    out.IsFinalGate =
        evidence.ExpectedGate ==
            OriginalSegmentCount - 1;
    out.Fault = GateCrossingFault::None;
    out.AuthoritativeGatePassed = false;
    out.Valid = true;
    return out;
}
struct GateSequenceReceipt final
{
    int NextGate = 0;
    int CompletedLaps = 0;
    int QualifiedCrossings = 0;
    int RejectedCrossings = 0;
    int RepeatedCrossings = 0;
    int WrongOrderEvents = 0;
    double LastCrossingSeconds = 0.0;
    double LastLapSeconds = 0.0;
    double BestLapSeconds = 0.0;
    double CurrentLapStartSeconds = 0.0;
    double MaximumCrossingGapSeconds = 0.0;
    std::array<double, OriginalSegmentCount> CurrentSectors{};
    std::array<double, OriginalSegmentCount> BestSectors{};
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    bool HasQualifiedLap = false;
    bool OfficialRaceProgressChanged = false;
};
class OriginalGateSequence final
{
public:
    bool Configure(GateCrossingParameters settings,
                   int lapsRequired = 3) noexcept
    {
        if (!ValidGateParameters(settings) ||
            lapsRequired < 1 || lapsRequired > 500)
            return false;
        Settings = settings;
        RequiredLaps = lapsRequired;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Receipt = {};
        LastTimestamp = -1.0;
        LastFrame = 0;
        HasEpoch = false;
        Session = 0;
        Actor = 0;
    }
    bool Begin(RaceIdentity identity,
               double startingSeconds) noexcept
    {
        if (!PhysicalActorIdentityValid(identity) ||
            !Finite(startingSeconds) ||
            startingSeconds < 0.0)
            return false;
        Reset();
        Session = identity.Session;
        Actor = identity.Actor;
        LastTimestamp = startingSeconds;
        Receipt.CurrentLapStartSeconds = startingSeconds;
        Receipt.LastCrossingSeconds = startingSeconds;
        Receipt.Session = Session;
        Receipt.Actor = Actor;
        HasEpoch = true;
        return true;
    }
    bool Observe(GateCrossingEvidence crossing) noexcept
    {
        if (!HasEpoch ||
            crossing.Identity.Session != Session ||
            crossing.Identity.Actor != Actor ||
            crossing.Identity.Frame <= LastFrame ||
            Receipt.CompletedLaps >= RequiredLaps)
        {
            ++Receipt.RejectedCrossings;
            return false;
        }
        if (crossing.ExpectedGate != Receipt.NextGate)
        {
            ++Receipt.WrongOrderEvents;
            ++Receipt.RejectedCrossings;
            return false;
        }
        const GateCrossingResult result =
            EvaluateGateCrossing(crossing, Settings);
        if (!result.Valid)
        {
            ++Receipt.RejectedCrossings;
            return false;
        }
        const double since = result.CrossingSeconds -
            Receipt.LastCrossingSeconds;
        if (since < Settings.MinimumBetweenGatesSeconds)
        {
            ++Receipt.RepeatedCrossings;
            ++Receipt.RejectedCrossings;
            return false;
        }
        Receipt.MaximumCrossingGapSeconds =
            std::max(Receipt.MaximumCrossingGapSeconds,
                     since);
        Receipt.CurrentSectors[static_cast<std::size_t>(
            Receipt.NextGate)] = since;
        double& best =
            Receipt.BestSectors[static_cast<std::size_t>(
                Receipt.NextGate)];
        if (best == 0.0 || since < best) best = since;
        Receipt.LastCrossingSeconds =
            result.CrossingSeconds;
        LastFrame = crossing.Identity.Frame;
        ++Receipt.QualifiedCrossings;
        ++Receipt.NextGate;
        if (Receipt.NextGate >= OriginalSegmentCount)
        {
            Receipt.NextGate = 0;
            ++Receipt.CompletedLaps;
            Receipt.LastLapSeconds =
                result.CrossingSeconds -
                Receipt.CurrentLapStartSeconds;
            if (!Receipt.HasQualifiedLap ||
                Receipt.LastLapSeconds < Receipt.BestLapSeconds)
                Receipt.BestLapSeconds =
                    Receipt.LastLapSeconds;
            Receipt.HasQualifiedLap =
                Receipt.LastLapSeconds > 0.0;
            Receipt.CurrentLapStartSeconds =
                result.CrossingSeconds;
            Receipt.CurrentSectors.fill(0.0);
        }
        Receipt.OfficialRaceProgressChanged = false;
        return true;
    }
    GateSequenceReceipt Snapshot() const noexcept
    {
        return Receipt;
    }
    bool CompletedRequiredLaps() const noexcept
    {
        return Receipt.CompletedLaps >= RequiredLaps;
    }
private:
    GateCrossingParameters Settings{};
    GateSequenceReceipt Receipt{};
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    std::uint64_t LastFrame = 0;
    int RequiredLaps = 3;
    double LastTimestamp = -1.0;
    bool HasEpoch = false;
};
} // namespace latebrake::racecraft
