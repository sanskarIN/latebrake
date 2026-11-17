#pragma once

#include "Track/WorldScaleRacePacePlan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// An engine-neutral, read-only six-physical-actor race observation.
// This works only with measured world positions, world velocities,
// independently qualified physical contact and ordered source identities.
// Future Unreal adapters must supply ALL six genuine chassis samples.
// This is not an AI controller, penalty system, standings authority,
// collision solver, racing-result clock, or qualified playable feature.
inline constexpr std::size_t SixCarRouteAuditCapacity = 6;
inline constexpr double SixCarRouteAuditMaxSpeedCmps = 15000.0;
// Independent route-to-position matching may need a wider projection
// tolerance on tessellated bends. Consecutive measured frames MUST meet
// separate, much tighter kinematic limits: at 20m/s and 120Hz the chassis
// advances only ~17cm, so an absolute 180cm allowance would accept a
// completely frozen actor indefinitely.
inline constexpr double SixCarRouteAuditMaxMotionResidualCm = 5.0;
inline constexpr double SixCarRouteAuditMaxAdvanceErrorCm = 5.0;
inline constexpr double SixCarRouteAuditMaxProgressResidualCm = 220.0;
inline constexpr double SixCarRouteAuditPassHysteresisCm = 35.0;
inline constexpr double SixCarRouteAuditMinFrameSeconds = 0.001;
inline constexpr double SixCarRouteAuditMaxFrameSeconds = 0.10;
inline constexpr unsigned SixCarRouteAuditMaxRecordedLaps = 1000;

struct SixCarRouteActorSample final
{
    int GridSlot = -1;
    std::uint64_t ActorIdentity = 0;
    unsigned CompletedLaps = 0;
    double ReportedLapProgressCm = 0.0;
    double WorldX = 0.0;
    double WorldY = 0.0;
    double VelocityX = 0.0;
    double VelocityY = 0.0;
    bool QualifiedRoadContact = false;
};

struct SixCarRouteFrameSample final
{
    std::uint64_t SessionIdentity = 0;
    std::uint64_t FrameOrdinal = 0;
    double DeltaSeconds = 0.0;
    bool ActiveRace = false;
    std::array<SixCarRouteActorSample, SixCarRouteAuditCapacity> Actors{};
};

struct SixCarRouteCarAdvisory final
{
    std::uint64_t ActorIdentity = 0;
    int GridSlot = -1;
    double TotalReportedProgressCm = 0.0;
    // Independently geometrically projected station plus the
    // externally qualified completed-lap count. Advisory only.
    double TotalProjectedProgressCm = 0.0;
    double MeasuredSpeedCmps = 0.0;
    double SignedRoadOffsetCm = 0.0;
    double RouteProjectionResidualCm = 0.0;
};

struct SixCarRouteAuditReadout final
{
    bool Valid = false;
    bool MotionContinuous = false;
    std::array<SixCarRouteCarAdvisory, SixCarRouteAuditCapacity> CarsBySlot{};
    std::array<int, SixCarRouteAuditCapacity> AdvisoryOrder{};
    // Hysteresis-qualified pairwise rank reversals in this observed frame.
    // Do NOT use for race positions, overtaking awards or checkpoint state.
    unsigned ConfirmedRelativePasses = 0;
    std::uint64_t AcceptedContinuityFrames = 0;
    std::uint64_t RejectedFrames = 0;
    double ContinuousObservedSeconds = 0.0;
};

inline bool ValidSixCarRouteActor(const SixCarRouteActorSample& a) noexcept
{
    if (a.GridSlot < 0 ||
        a.GridSlot >= static_cast<int>(SixCarRouteAuditCapacity) ||
        a.ActorIdentity == 0 || !a.QualifiedRoadContact ||
        a.CompletedLaps > SixCarRouteAuditMaxRecordedLaps ||
        !std::isfinite(a.ReportedLapProgressCm) ||
        a.ReportedLapProgressCm < 0.0 ||
        a.ReportedLapProgressCm >= WorldScaleCircuitLapCm ||
        !std::isfinite(a.WorldX) || !std::isfinite(a.WorldY) ||
        !std::isfinite(a.VelocityX) || !std::isfinite(a.VelocityY) ||
        std::abs(a.WorldX) > 1000000.0 ||
        std::abs(a.WorldY) > 1000000.0 ||
        std::hypot(a.VelocityX, a.VelocityY) >
            SixCarRouteAuditMaxSpeedCmps)
    {
        return false;
    }
    return true;
}

inline double SixCarRouteDistanceDifference(double a, double b) noexcept
{
    if (!std::isfinite(a) || !std::isfinite(b) ||
        a < 0.0 || b < 0.0 ||
        a >= WorldScaleCircuitLapCm || b >= WorldScaleCircuitLapCm)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double raw = std::abs(a - b);
    return std::min(raw, WorldScaleCircuitLapCm - raw);
}

class SixCarRouteContinuityAudit final
{
public:
    SixCarRouteContinuityAudit() noexcept
        : Reference(BuildWorldScalePacePlan())
    {
        Reset();
    }

    bool HasQualifiedReference() const noexcept
    {
        return Reference.Valid;
    }

    void Reset() noexcept
    {
        Session = 0;
        PreviousFrameOrdinal = 0;
        HaveBaseline = false;
        PreviousActors = {};
        PreviousReportedDistance = {};
        PreviousSignedRouteSpeed = {};
        PairwiseLead = {};
        AcceptedFrames = 0;
        RejectedCount = 0;
        ContinuousSeconds = 0.0;
        Last = {};
    }

    void Suspend() noexcept
    {
        // Pause, recovery, focus loss and unloaded actors have no
        // trusted successor. Never credit a gap with made-up motion.
        Reset();
    }

    SixCarRouteAuditReadout Readout() const noexcept
    {
        return Last;
    }

    SixCarRouteAuditReadout Observe(
        const SixCarRouteFrameSample& frame) noexcept
    {
        if (!Reference.Valid || frame.SessionIdentity == 0 ||
            frame.FrameOrdinal == 0 || !frame.ActiveRace ||
            !std::isfinite(frame.DeltaSeconds) ||
            frame.DeltaSeconds < SixCarRouteAuditMinFrameSeconds ||
            frame.DeltaSeconds > SixCarRouteAuditMaxFrameSeconds)
        {
            RejectAndBreakContinuity();
            return Last;
        }

        std::array<SixCarRouteActorSample, SixCarRouteAuditCapacity> ordered{};
        std::array<double, SixCarRouteAuditCapacity> currentDistance{};
        std::array<double, SixCarRouteAuditCapacity> projectedDistance{};
        std::array<double, SixCarRouteAuditCapacity> currentSignedRouteSpeed{};
        std::array<SixCarRouteCarAdvisory, SixCarRouteAuditCapacity> current{};
        std::array<bool, SixCarRouteAuditCapacity> seenSlots{};
        for (const auto& actor : frame.Actors)
        {
            if (!ValidSixCarRouteActor(actor))
            {
                RejectAndBreakContinuity();
                return Last;
            }
            const std::size_t slot = static_cast<std::size_t>(actor.GridSlot);
            if (seenSlots[slot])
            {
                RejectAndBreakContinuity();
                return Last;
            }
            seenSlots[slot] = true;
            const auto projection = ProjectWorldScaleRoute(
                Reference, {actor.WorldX, actor.WorldY});
            if (!projection.Valid ||
                !projection.InsidePhysicalCandidateRoad ||
                !std::isfinite(projection.SignedLateralCm) ||
                !std::isfinite(projection.CentrelineSeparationCm) ||
                SixCarRouteDistanceDifference(
                    actor.ReportedLapProgressCm,
                    projection.LapDistanceCm) >
                    SixCarRouteAuditMaxProgressResidualCm)
            {
                RejectAndBreakContinuity();
                return Last;
            }
            // Compare reported progress to the ACTUAL geometric forward
            // tangent, not speed magnitude. A car travelling backward or
            // sliding sideways cannot forge forward lap progress by merely
            // supplying a plausible speedometer value. Tangent is derived
            // from independently projected authored geometry, not the
            // untrusted reported station or a driver-facing nose vector.
            constexpr double TangentSampleHalfWindowCm = 100.0;
            const auto tangentBefore = WorldScaleCenterlineAt(
                projection.LapDistanceCm - TangentSampleHalfWindowCm);
            const auto tangentAfter = WorldScaleCenterlineAt(
                projection.LapDistanceCm + TangentSampleHalfWindowCm);
            if (!tangentBefore.Valid || !tangentAfter.Valid)
            {
                RejectAndBreakContinuity();
                return Last;
            }
            const double tangentX =
                tangentAfter.Pose.Position.X -
                tangentBefore.Pose.Position.X;
            const double tangentY =
                tangentAfter.Pose.Position.Y -
                tangentBefore.Pose.Position.Y;
            const double tangentLength = std::hypot(tangentX, tangentY);
            if (!std::isfinite(tangentLength) || tangentLength < 1.0)
            {
                RejectAndBreakContinuity();
                return Last;
            }
            const double signedProgressSpeed =
                (actor.VelocityX * tangentX +
                 actor.VelocityY * tangentY) / tangentLength;
            if (!std::isfinite(signedProgressSpeed))
            {
                RejectAndBreakContinuity();
                return Last;
            }
            currentSignedRouteSpeed[slot] = signedProgressSpeed;
            ordered[slot] = actor;
            const double total =
                static_cast<double>(actor.CompletedLaps) *
                    WorldScaleCircuitLapCm +
                actor.ReportedLapProgressCm;
            currentDistance[slot] = total;
            // Reported distance may be an honest noisy measurement,
            // but must NOT alone authorize a passing or ordering event.
            // The independently measured XY chassis projection is used
            // for advisory relative ranking, while reported distance is
            // separately checked against both geometry and elapsed motion.
            const double projectedTotal =
                static_cast<double>(actor.CompletedLaps) *
                    WorldScaleCircuitLapCm +
                projection.LapDistanceCm;
            projectedDistance[slot] = projectedTotal;
            current[slot] = {
                actor.ActorIdentity, actor.GridSlot, total,
                projectedTotal,
                std::hypot(actor.VelocityX, actor.VelocityY),
                projection.SignedLateralCm,
                projection.CentrelineSeparationCm,
            };
        }

        // A complete six-slot roster is mandatory. An unknown, duplicated
        // or replaced identity cannot inherit a different car's ordering.
        for (std::size_t i = 0; i < SixCarRouteAuditCapacity; ++i)
        {
            if (!seenSlots[i]) { RejectAndBreakContinuity(); return Last; }
            for (std::size_t j = i + 1; j < SixCarRouteAuditCapacity; ++j)
            {
                if (ordered[i].ActorIdentity == ordered[j].ActorIdentity)
                {
                    RejectAndBreakContinuity();
                    return Last;
                }
            }
        }

        if (Session != frame.SessionIdentity)
        {
            // A new race epoch is not a continuation of the old one.
            Reset();
            Session = frame.SessionIdentity;
        }
        const bool nextOrdinal = HaveBaseline &&
            PreviousFrameOrdinal != std::numeric_limits<std::uint64_t>::max() &&
            frame.FrameOrdinal == PreviousFrameOrdinal + 1;
        bool matchingActors = nextOrdinal;
        if (matchingActors)
        {
            for (std::size_t i = 0; i < SixCarRouteAuditCapacity; ++i)
            {
                if (ordered[i].ActorIdentity !=
                    PreviousActors[i].ActorIdentity)
                {
                    matchingActors = false;
                    break;
                }
            }
        }

        if (matchingActors)
        {
            for (std::size_t i = 0; i < SixCarRouteAuditCapacity; ++i)
            {
                const auto& before = PreviousActors[i];
                const auto& after = ordered[i];
                const double expectedX = before.WorldX +
                    0.5 * frame.DeltaSeconds *
                        (before.VelocityX + after.VelocityX);
                const double expectedY = before.WorldY +
                    0.5 * frame.DeltaSeconds *
                        (before.VelocityY + after.VelocityY);
                const double worldResidual = std::hypot(
                    after.WorldX - expectedX, after.WorldY - expectedY);
                const double measuredAdvance =
                    currentDistance[i] - PreviousReportedDistance[i];
                // True route travel is SIGNED: magnitude cannot prove
                // forward motion. Drifting almost perpendicular to the
                // road and reversing cannot claim forward progress.
                const double expectedAdvance = 0.5 * frame.DeltaSeconds *
                    (PreviousSignedRouteSpeed[i] +
                     currentSignedRouteSpeed[i]);
                const double measuredTravelMagnitude = 0.5 *
                    frame.DeltaSeconds *
                    (std::hypot(before.VelocityX, before.VelocityY) +
                     current[i].MeasuredSpeedCmps);
                // A wide 220cm geometric projection tolerance does NOT
                // authorize the same 220cm of forged motion per physics
                // tick. Use physically scaled, much tighter successive
                // frame error budgets, including at 30/60/120Hz.
                const double maxWorldError = std::max(
                    SixCarRouteAuditMaxMotionResidualCm,
                    0.12 * measuredTravelMagnitude);
                const double maxAdvanceError = std::max(
                    SixCarRouteAuditMaxAdvanceErrorCm,
                    0.12 * measuredTravelMagnitude);
                if (!std::isfinite(worldResidual) ||
                    !std::isfinite(measuredAdvance) ||
                    !std::isfinite(expectedAdvance) ||
                    !std::isfinite(measuredTravelMagnitude) ||
                    worldResidual > maxWorldError ||
                    measuredAdvance < -5.0 ||
                    std::abs(measuredAdvance - expectedAdvance) >
                        maxAdvanceError)
                {
                    // An impossible teleport, lap shortcut, rollback, or
                    // synthetic speed jump invalidates the ENTIRE frame.
                    // Never expose a partially updated roster/rank.
                    RejectAndBreakContinuity();
                    return Last;
                }
            }
        }

        // A missing timestamp or replacement car is a new advisory
        // baseline, not evidence of six simultaneous continuous cars.
        const bool continuous = matchingActors;
        if (!continuous)
        {
            AcceptedFrames = 0;
            ContinuousSeconds = 0.0;
            PairwiseLead = {};
        }
        else
        {
            ++AcceptedFrames;
            ContinuousSeconds += frame.DeltaSeconds;
        }

        SixCarRouteAuditReadout report{};
        report.Valid = true;
        report.MotionContinuous = continuous;
        report.CarsBySlot = current;
        for (std::size_t i = 0; i < SixCarRouteAuditCapacity; ++i)
        {
            report.AdvisoryOrder[i] = static_cast<int>(i);
        }
        std::sort(report.AdvisoryOrder.begin(),
                  report.AdvisoryOrder.end(),
                  [&](int a, int b) noexcept {
            const double lhs =
                projectedDistance[static_cast<std::size_t>(a)];
            const double rhs =
                projectedDistance[static_cast<std::size_t>(b)];
            return lhs > rhs ||
                (lhs == rhs && a < b);
        });

        // A 35cm hysteresis prevents rank chatter from creating repeated
        // passing events when two observations are nearly level.
        for (std::size_t i = 0; i < SixCarRouteAuditCapacity; ++i)
        for (std::size_t j = i + 1; j < SixCarRouteAuditCapacity; ++j)
        {
            const double gap = projectedDistance[i] - projectedDistance[j];
            int lead = 0;
            if (gap > SixCarRouteAuditPassHysteresisCm) lead = 1;
            else if (gap < -SixCarRouteAuditPassHysteresisCm) lead = -1;
            if (lead != 0)
            {
                if (continuous && PairwiseLead[i][j] != 0 &&
                    PairwiseLead[i][j] != lead)
                {
                    // Count an advisory FORWARD pass only when the newly
                    // leading physical chassis has genuinely positive
                    // longitudinal road velocity (at least 0.5 m/s).
                    const std::size_t newLeader =
                        lead > 0 ? i : j;
                    if (currentSignedRouteSpeed[newLeader] >= 50.0)
                    {
                        ++report.ConfirmedRelativePasses;
                    }
                }
                PairwiseLead[i][j] = lead;
            }
        }

        PreviousActors = ordered;
        PreviousReportedDistance = currentDistance;
        PreviousSignedRouteSpeed = currentSignedRouteSpeed;
        PreviousFrameOrdinal = frame.FrameOrdinal;
        HaveBaseline = true;
        report.AcceptedContinuityFrames = AcceptedFrames;
        report.RejectedFrames = RejectedCount;
        report.ContinuousObservedSeconds = ContinuousSeconds;
        Last = report;
        return Last;
    }

private:
    void RejectAndBreakContinuity() noexcept
    {
        ++RejectedCount;
        HaveBaseline = false;
        PreviousFrameOrdinal = 0;
        PairwiseLead = {};
        AcceptedFrames = 0;
        ContinuousSeconds = 0.0;
        Last = {};
        Last.RejectedFrames = RejectedCount;
    }

    WorldScalePacePlan Reference{};
    std::uint64_t Session = 0;
    std::uint64_t PreviousFrameOrdinal = 0;
    bool HaveBaseline = false;
    std::array<SixCarRouteActorSample, SixCarRouteAuditCapacity>
        PreviousActors{};
    std::array<double, SixCarRouteAuditCapacity>
        PreviousReportedDistance{};
    std::array<double, SixCarRouteAuditCapacity>
        PreviousSignedRouteSpeed{};
    std::array<std::array<int, SixCarRouteAuditCapacity>,
               SixCarRouteAuditCapacity> PairwiseLead{};
    std::uint64_t AcceptedFrames = 0;
    std::uint64_t RejectedCount = 0;
    double ContinuousSeconds = 0.0;
    SixCarRouteAuditReadout Last{};
};
} // namespace latebrake
