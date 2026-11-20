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
// This is a candidate, read-only path geometry planner for the future
// original 1.7-km circuit. It does NOT steer a live actor, widen the
// WorldStatic road, alter car collision, generate race lap credit or
// replace the trusted centerline on a shipped level.
inline constexpr std::size_t WorldScaleLineStationCount =
    WorldScalePaceStations;
inline constexpr double WorldScaleLinePathStepCm =
    WorldScalePaceStepCm;

struct WorldScaleLinePlannerSettings final
{
    double MaximumCandidateOffsetCm = 250.0;
    double MaximumNeighborOffsetChangeCm = 65.0;
    double VehicleHalfWidthCm = 98.0;
    double RoadEdgeSafetyMarginCm = 55.0;
    double MaximumLocalLateralAccelerationCmS2 =
        WorldScalePaceLateralAccelCmps2;
    double MaximumBrakingAccelerationCmS2 =
        WorldScalePaceBrakeAccelCmps2;
    double MaximumDriveAccelerationCmS2 =
        WorldScalePaceDriveAccelCmps2;
    unsigned SmoothingPasses = 12;
    bool RequireOriginalPhysicalRoad = true;
    bool ForbidOfficialCheckpointMutation = true;
    bool NoActualVehicleSteeringAuthority = true;
};

struct WorldScaleLineRequest final
{
    std::uint64_t ProposalIdentity = 0;
    bool AuthoredReferenceConfirmed = false;
    bool SourceOnlyAdvisory = true;
    std::array<double, WorldScaleLineStationCount> DesiredOffsetCm{};
};

struct WorldScaleLineNode final
{
    CircuitPoint OriginalCenter{};
    CircuitPoint CandidateCenter{};
    double SourceStationCm = 0.0;
    double OffsetCm = 0.0;
    double CurvaturePerCm = 0.0;
    double LocalSpeedLimitCmps = 0.0;
    double TargetSpeedCmps = 0.0;
    double MinimumBodyEdgeClearanceCm = 0.0;
    bool PhysicallySupportedAtSource = false;
};

struct WorldScaleLinePlan final
{
    bool Valid = false;
    std::uint64_t ProposalIdentity = 0;
    unsigned RelaxationPasses = 0;
    unsigned SmoothingPasses = 0;
    unsigned VerifiedSegments = 0;
    unsigned SpeedReducedStations = 0;
    double MaximumAbsoluteLateralOffsetCm = 0.0;
    double MaximumNeighborLateralChangeCm = 0.0;
    double EstimatedLapSeconds = 0.0;
    bool OriginalPhysicalRoadUnchanged = true;
    bool NoOfficialProgressAuthority = true;
    bool DoesNotSteerChaos = true;
    std::array<WorldScaleLineNode, WorldScaleLineStationCount> Nodes{};
};

struct WorldScaleLineSample final
{
    bool Valid = false;
    double WrappedStationCm = 0.0;
    CircuitPoint CandidateCenter{};
    double OffsetCm = 0.0;
    double TargetSpeedCmps = 0.0;
    double DirectionRadians = 0.0;
    bool InsideOriginalPhysicalRoad = false;
};

inline bool ValidWorldScaleLineSettings(
    const WorldScaleLinePlannerSettings& p) noexcept
{
    return std::isfinite(p.MaximumCandidateOffsetCm) &&
        p.MaximumCandidateOffsetCm >= 0.0 &&
        p.MaximumCandidateOffsetCm <= 300.0 &&
        std::isfinite(p.MaximumNeighborOffsetChangeCm) &&
        p.MaximumNeighborOffsetChangeCm >= 5.0 &&
        p.MaximumNeighborOffsetChangeCm <= 120.0 &&
        std::isfinite(p.VehicleHalfWidthCm) &&
        p.VehicleHalfWidthCm >= 80.0 &&
        p.VehicleHalfWidthCm <= 120.0 &&
        std::isfinite(p.RoadEdgeSafetyMarginCm) &&
        p.RoadEdgeSafetyMarginCm >= 30.0 &&
        p.RoadEdgeSafetyMarginCm <= 120.0 &&
        p.MaximumCandidateOffsetCm + p.VehicleHalfWidthCm +
            p.RoadEdgeSafetyMarginCm < WorldScaleRoadHalfWidthCm &&
        std::isfinite(p.MaximumLocalLateralAccelerationCmS2) &&
        p.MaximumLocalLateralAccelerationCmS2 > 0.0 &&
        p.MaximumLocalLateralAccelerationCmS2 <=
            WorldScalePaceLateralAccelCmps2 &&
        std::isfinite(p.MaximumBrakingAccelerationCmS2) &&
        p.MaximumBrakingAccelerationCmS2 > 0.0 &&
        p.MaximumBrakingAccelerationCmS2 <=
            WorldScalePaceBrakeAccelCmps2 &&
        std::isfinite(p.MaximumDriveAccelerationCmS2) &&
        p.MaximumDriveAccelerationCmS2 > 0.0 &&
        p.MaximumDriveAccelerationCmS2 <=
            WorldScalePaceDriveAccelCmps2 &&
        p.SmoothingPasses >= 1u && p.SmoothingPasses <= 64u &&
        p.RequireOriginalPhysicalRoad &&
        p.ForbidOfficialCheckpointMutation &&
        p.NoActualVehicleSteeringAuthority;
}

inline CircuitPoint WorldScaleLineOffsetPoint(
    const RoundedCenterlinePose& center,
    double offsetCm) noexcept
{
    const double heading =
        center.Pose.HeadingDegrees * WorldScalePaceRadPerDegree;
    return {
        center.Pose.Position.X - std::sin(heading) * offsetCm,
        center.Pose.Position.Y + std::cos(heading) * offsetCm
    };
}

inline double WorldScaleLineCircleCurvature(
    CircuitPoint before,
    CircuitPoint here,
    CircuitPoint after) noexcept
{
    const double ax = here.X - before.X;
    const double ay = here.Y - before.Y;
    const double bx = after.X - here.X;
    const double by = after.Y - here.Y;
    const double cx = after.X - before.X;
    const double cy = after.Y - before.Y;
    const double la = std::hypot(ax, ay);
    const double lb = std::hypot(bx, by);
    const double lc = std::hypot(cx, cy);
    const double divisor = la * lb * lc;
    if (!std::isfinite(divisor) || divisor <= 1.0e-8)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return 2.0 * (ax * by - ay * bx) / divisor;
}

inline bool WorldScaleLineSpanSupported(
    CircuitPoint first, CircuitPoint second) noexcept
{
    if (!IsOnWorldScaleRoad(first) ||
        !IsOnWorldScaleRoad(second))
    {
        return false;
    }
    // A source point inside an asphalt slab does not imply a straight
    // cut between two route samples is inside the physical union.
    // Probe every quarter of the candidate chord to avoid granting
    // advisory permission through the infield on tight bends.
    for (double f : {0.25, 0.50, 0.75})
    {
        const CircuitPoint at{
            first.X + (second.X - first.X) * f,
            first.Y + (second.Y - first.Y) * f
        };
        if (!IsOnWorldScaleRoad(at)) return false;
    }
    return true;
}

inline WorldScaleLinePlan BuildWorldScaleCandidateRacingLine(
    const WorldScalePacePlan& reference,
    const WorldScaleLineRequest& request,
    WorldScaleLinePlannerSettings settings = {}) noexcept
{
    if (!reference.Valid || !ValidWorldScaleLineSettings(settings) ||
        request.ProposalIdentity == 0 ||
        !request.AuthoredReferenceConfirmed ||
        !request.SourceOnlyAdvisory)
    {
        return {};
    }

    WorldScaleLinePlan plan{};
    std::array<double, WorldScaleLineStationCount> offsets{};
    std::array<double, WorldScaleLineStationCount> smooth{};
    std::array<double, WorldScaleLineStationCount> speedSquared{};
    for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
    {
        const double requested = request.DesiredOffsetCm[i];
        if (!std::isfinite(requested) ||
            std::abs(requested) > settings.MaximumCandidateOffsetCm)
        {
            return {};
        }
        offsets[i] = requested;
    }

    // Discrete diffusion attenuates harsh one-frame lane changes, using
    // the circular 511 -> 0 neighborhood rather than treating the
    // starting grid as a hard unsmoothed boundary.
    for (unsigned pass = 0; pass < settings.SmoothingPasses; ++pass)
    {
        for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
        {
            const auto prev =
                (i + WorldScaleLineStationCount - 1u) %
                WorldScaleLineStationCount;
            const auto next = (i + 1u) % WorldScaleLineStationCount;
            const double blended =
                0.50 * offsets[i] + 0.25 *
                    (offsets[prev] + offsets[next]);
            smooth[i] = std::clamp(
                blended, -settings.MaximumCandidateOffsetCm,
                settings.MaximumCandidateOffsetCm);
        }
        offsets = smooth;
    }
    plan.SmoothingPasses = settings.SmoothingPasses;

    // Strict directed neighbor-slope constraint. Relaxation never
    // increases requested extremes and cannot create lateral motion
    // at the source start/finish seam by forgetting wrap adjacency.
    for (unsigned pass = 0; pass < WorldScaleLineStationCount; ++pass)
    {
        bool changed = false;
        for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
        {
            const auto next = (i + 1u) % WorldScaleLineStationCount;
            const double clamped = std::clamp(
                offsets[next],
                offsets[i] - settings.MaximumNeighborOffsetChangeCm,
                offsets[i] + settings.MaximumNeighborOffsetChangeCm);
            if (clamped != offsets[next])
            {
                offsets[next] = clamped;
                changed = true;
            }
        }
        for (std::size_t i = WorldScaleLineStationCount; i-- > 0u;)
        {
            const auto next = (i + 1u) % WorldScaleLineStationCount;
            const double clamped = std::clamp(
                offsets[i],
                offsets[next] - settings.MaximumNeighborOffsetChangeCm,
                offsets[next] + settings.MaximumNeighborOffsetChangeCm);
            if (clamped != offsets[i])
            {
                offsets[i] = clamped;
                changed = true;
            }
        }
        if (!changed) break;
        if (pass + 1u == WorldScaleLineStationCount) return {};
    }

    for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
    {
        const auto center = WorldScaleCenterlineAt(
            i * WorldScaleLinePathStepCm);
        if (!center.Valid ||
            !IsOnWorldScaleRoad(center.Pose.Position))
        {
            return {};
        }
        const CircuitPoint candidate =
            WorldScaleLineOffsetPoint(center, offsets[i]);
        if (!std::isfinite(candidate.X) ||
            !std::isfinite(candidate.Y) ||
            !IsOnWorldScaleRoad(candidate))
        {
            return {};
        }
        const double clearance =
            WorldScaleRoadHalfWidthCm -
            std::abs(offsets[i]) - settings.VehicleHalfWidthCm;
        if (!std::isfinite(clearance) ||
            clearance < settings.RoadEdgeSafetyMarginCm)
        {
            return {};
        }
        plan.Nodes[i].OriginalCenter = center.Pose.Position;
        plan.Nodes[i].CandidateCenter = candidate;
        plan.Nodes[i].SourceStationCm = i * WorldScaleLinePathStepCm;
        plan.Nodes[i].OffsetCm = offsets[i];
        plan.Nodes[i].MinimumBodyEdgeClearanceCm = clearance;
        plan.Nodes[i].PhysicallySupportedAtSource = true;
        plan.MaximumAbsoluteLateralOffsetCm = std::max(
            plan.MaximumAbsoluteLateralOffsetCm,
            std::abs(offsets[i]));
    }
    for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
    {
        const auto next = (i + 1u) % WorldScaleLineStationCount;
        const auto prev =
            (i + WorldScaleLineStationCount - 1u) %
            WorldScaleLineStationCount;
        const auto a = plan.Nodes[i].CandidateCenter;
        const auto b = plan.Nodes[next].CandidateCenter;
        if (!WorldScaleLineSpanSupported(a, b)) return {};
        ++plan.VerifiedSegments;
        const double diff =
            std::abs(offsets[next] - offsets[i]);
        if (diff > settings.MaximumNeighborOffsetChangeCm + 1.0e-9)
            return {};
        plan.MaximumNeighborLateralChangeCm = std::max(
            plan.MaximumNeighborLateralChangeCm, diff);

        const double curvature = std::abs(
            WorldScaleLineCircleCurvature(
                plan.Nodes[prev].CandidateCenter, a, b));
        if (!std::isfinite(curvature) || curvature > 0.002)
            return {};
        const double localLimit = curvature <= 1.0e-12 ?
            WorldScalePaceMaxSpeedCmps :
            std::sqrt(
                settings.MaximumLocalLateralAccelerationCmS2 / curvature);
        const double conservativeCap = std::min({
            localLimit, reference.Nodes[i].LocalLimitCmps,
            reference.Nodes[i].TargetCmps
        });
        if (!std::isfinite(conservativeCap) ||
            conservativeCap < WorldScalePaceMinSpeedCmps)
        {
            return {};
        }
        plan.Nodes[i].CurvaturePerCm = curvature;
        plan.Nodes[i].LocalSpeedLimitCmps = conservativeCap;
        speedSquared[i] = conservativeCap * conservativeCap;
    }

    const double maxBrake =
        2.0 * settings.MaximumBrakingAccelerationCmS2 *
        WorldScaleLinePathStepCm;
    const double maxDrive =
        2.0 * settings.MaximumDriveAccelerationCmS2 *
        WorldScaleLinePathStepCm;
    bool settled = false;
    for (unsigned pass = 0; pass < WorldScaleLineStationCount; ++pass)
    {
        bool changed = false;
        for (std::size_t i = WorldScaleLineStationCount; i-- > 0u;)
        {
            const auto next = (i + 1u) % WorldScaleLineStationCount;
            const double permitted = speedSquared[next] + maxBrake;
            if (speedSquared[i] > permitted)
            {
                speedSquared[i] = permitted;
                changed = true;
            }
        }
        for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
        {
            const auto next = (i + 1u) % WorldScaleLineStationCount;
            const double permitted = speedSquared[i] + maxDrive;
            if (speedSquared[next] > permitted)
            {
                speedSquared[next] = permitted;
                changed = true;
            }
        }
        plan.RelaxationPasses = pass + 1u;
        if (!changed)
        {
            settled = true;
            break;
        }
    }
    if (!settled) return {};

    double lapSeconds = 0.0;
    for (std::size_t i = 0; i < WorldScaleLineStationCount; ++i)
    {
        const auto next = (i + 1u) % WorldScaleLineStationCount;
        const double a = std::sqrt(speedSquared[i]);
        const double b = std::sqrt(speedSquared[next]);
        if (!std::isfinite(a) || !std::isfinite(b) ||
            a < WorldScalePaceMinSpeedCmps - 1e-6 ||
            a > reference.Nodes[i].TargetCmps + 1e-6 ||
            speedSquared[i] > speedSquared[next] + maxBrake + 0.01 ||
            speedSquared[next] > speedSquared[i] + maxDrive + 0.01)
        {
            return {};
        }
        plan.Nodes[i].TargetSpeedCmps = a;
        if (a < reference.Nodes[i].TargetCmps - 1e-4)
            ++plan.SpeedReducedStations;
        lapSeconds += 2.0 * WorldScaleLinePathStepCm / (a + b);
    }
    if (plan.VerifiedSegments != WorldScaleLineStationCount ||
        !std::isfinite(lapSeconds) ||
        lapSeconds < reference.EstimatedLapSeconds - 1.0e-6 ||
        lapSeconds > 1200.0)
    {
        return {};
    }
    plan.EstimatedLapSeconds = lapSeconds;
    plan.ProposalIdentity = request.ProposalIdentity;
    plan.Valid = true;
    return plan;
}

inline WorldScaleLineSample SampleWorldScaleCandidateRacingLine(
    const WorldScaleLinePlan& plan, double stationCm) noexcept
{
    if (!plan.Valid || !std::isfinite(stationCm))
    {
        return {};
    }
    double wrapped = std::fmod(stationCm, WorldScaleCircuitLapCm);
    if (wrapped < 0.0) wrapped += WorldScaleCircuitLapCm;
    const auto i = std::min(
        static_cast<std::size_t>(wrapped / WorldScaleLinePathStepCm),
        WorldScaleLineStationCount - 1u);
    const auto next = (i + 1u) % WorldScaleLineStationCount;
    const double fraction = std::clamp(
        (wrapped - plan.Nodes[i].SourceStationCm) /
            WorldScaleLinePathStepCm, 0.0, 1.0);
    const auto a = plan.Nodes[i].CandidateCenter;
    const auto b = plan.Nodes[next].CandidateCenter;
    const CircuitPoint at{
        a.X + (b.X - a.X) * fraction,
        a.Y + (b.Y - a.Y) * fraction
    };
    const double v0 = plan.Nodes[i].TargetSpeedCmps;
    const double v1 = plan.Nodes[next].TargetSpeedCmps;
    const double squared = v0 * v0 +
        fraction * (v1 * v1 - v0 * v0);
    const double speed = std::sqrt(squared);
    if (!std::isfinite(at.X) || !std::isfinite(at.Y) ||
        !std::isfinite(speed) || speed < WorldScalePaceMinSpeedCmps ||
        !IsOnWorldScaleRoad(at))
    {
        return {};
    }
    return {
        true, wrapped, at,
        plan.Nodes[i].OffsetCm +
            (plan.Nodes[next].OffsetCm -
             plan.Nodes[i].OffsetCm) * fraction,
        speed, std::atan2(b.Y - a.Y, b.X - a.X),
        true
    };
}
} // namespace latebrake
