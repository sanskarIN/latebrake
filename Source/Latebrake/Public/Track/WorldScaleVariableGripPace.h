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
// Conservative, portable, source-only variable-grip pace ADVISORY for
// the planned 1.7km circuit. No gameplay, Chaos force, lap credit or AI
// throttle authority is changed. Observations MUST be supplied by a
// future trusted adapter; this library never invents measured friction.
inline constexpr std::size_t WorldScaleGripPaceStationCount =
    WorldScalePaceStations;

struct WorldScaleGripPaceStation final
{
    double StationCm = 0.0;
    double MeasuredGripFraction = 0.0;
    double SourceConfidence = 0.0;
    double MeasurementClockSeconds = 0.0;
    std::uint64_t RoadComponentIdentity = 0;
    bool PhysicalRoadContactVerified = false;
    bool RealFrictionMeasurementVerified = false;
};

struct WorldScaleGripPaceInput final
{
    std::uint64_t SessionIdentity = 0;
    double SnapshotClockSeconds = 0.0;
    bool OriginalWorldScaleRoadVerified = false;
    bool CompleteMeasuredStationCoverage = false;
    std::array<WorldScaleGripPaceStation,
               WorldScaleGripPaceStationCount> Stations{};
};

struct WorldScaleGripPaceSettings final
{
    double MinimumAcceptedGripFraction = 0.50;
    double MaximumAcceptedGripFraction = 1.00;
    double MinimumMeasurementConfidence = 0.75;
    double MaximumMeasurementAgeSeconds = 2.0;
    double MaximumStationMismatchCm = 0.10;
    double BaselineLateralAccelCmS2 = WorldScalePaceLateralAccelCmps2;
    double BaselineBrakeAccelCmS2 = WorldScalePaceBrakeAccelCmps2;
    double BaselineDriveAccelCmS2 = WorldScalePaceDriveAccelCmps2;
};

struct WorldScaleGripPaceNode final
{
    double StationCm = 0.0;
    double VerifiedGripFraction = 0.0;
    double CurvaturePerCm = 0.0;
    double LocalSpeedLimitCmps = 0.0;
    double TargetSpeedCmps = 0.0;
    double MaxNextBrakeDeltaSpeedSquared = 0.0;
    double MaxNextDriveDeltaSpeedSquared = 0.0;
};

struct WorldScaleVariableGripPacePlan final
{
    bool Valid = false;
    std::uint64_t SourceSessionIdentity = 0;
    double SourceClockSeconds = 0.0;
    double MinimumMeasuredGripFraction = 0.0;
    double MaximumMeasurementAgeSeconds = 0.0;
    unsigned ReducedSpeedStations = 0;
    unsigned RelaxationPasses = 0;
    double EstimatedLapSeconds = 0.0;
    // These physical constraints are evaluated on the ORIGINAL ordered
    // 512-station closed ring, including the 511 -> 0 braking edge.
    std::array<WorldScaleGripPaceNode,
               WorldScaleGripPaceStationCount> Nodes{};
};

struct WorldScaleGripPaceReference final
{
    bool Valid = false;
    double WrappedStationCm = 0.0;
    double TargetSpeedCmps = 0.0;
    double GripAtPreviousNode = 0.0;
    double GripAtNextNode = 0.0;
};

inline bool ValidWorldScaleGripPaceSettings(
    WorldScaleGripPaceSettings p) noexcept
{
    return std::isfinite(p.MinimumAcceptedGripFraction) &&
        p.MinimumAcceptedGripFraction >= 0.50 &&
        p.MinimumAcceptedGripFraction <= 1.0 &&
        std::isfinite(p.MaximumAcceptedGripFraction) &&
        p.MaximumAcceptedGripFraction >=
            p.MinimumAcceptedGripFraction &&
        p.MaximumAcceptedGripFraction <= 1.0 &&
        std::isfinite(p.MinimumMeasurementConfidence) &&
        p.MinimumMeasurementConfidence >= 0.5 &&
        p.MinimumMeasurementConfidence <= 1.0 &&
        std::isfinite(p.MaximumMeasurementAgeSeconds) &&
        p.MaximumMeasurementAgeSeconds > 0.0 &&
        p.MaximumMeasurementAgeSeconds <= 60.0 &&
        std::isfinite(p.MaximumStationMismatchCm) &&
        p.MaximumStationMismatchCm >= 0.0 &&
        p.MaximumStationMismatchCm <= WorldScalePaceStepCm * 0.01 &&
        std::isfinite(p.BaselineLateralAccelCmS2) &&
        p.BaselineLateralAccelCmS2 > 0.0 &&
        p.BaselineLateralAccelCmS2 <=
            WorldScalePaceLateralAccelCmps2 &&
        std::isfinite(p.BaselineBrakeAccelCmS2) &&
        p.BaselineBrakeAccelCmS2 > 0.0 &&
        p.BaselineBrakeAccelCmS2 <= WorldScalePaceBrakeAccelCmps2 &&
        std::isfinite(p.BaselineDriveAccelCmS2) &&
        p.BaselineDriveAccelCmS2 > 0.0 &&
        p.BaselineDriveAccelCmS2 <= WorldScalePaceDriveAccelCmps2;
}

// Sensor ingestion is fail-closed: all 512 individually qualified source
// stations must be present. A missing, old, disconnected, duplicated or
// nonphysical observation never becomes assumed dry grip. The resulting
// plan is an advisory hypothesis until the real world-scale roadway
// exists and is sampled in a licensed Unreal session.
inline WorldScaleVariableGripPacePlan BuildWorldScaleVariableGripPace(
    const WorldScalePacePlan& reference,
    const WorldScaleGripPaceInput& input,
    WorldScaleGripPaceSettings settings = {}) noexcept
{
    if (!reference.Valid || input.SessionIdentity == 0 ||
        !input.OriginalWorldScaleRoadVerified ||
        !input.CompleteMeasuredStationCoverage ||
        !std::isfinite(input.SnapshotClockSeconds) ||
        input.SnapshotClockSeconds < 0.0 ||
        !ValidWorldScaleGripPaceSettings(settings))
    {
        return {};
    }

    WorldScaleVariableGripPacePlan plan{};
    std::array<double, WorldScaleGripPaceStationCount> speedSquared{};
    std::array<double, WorldScaleGripPaceStationCount> grip{};

    double minimumGrip = 1.0;
    double oldestMeasurement = 0.0;
    for (std::size_t i = 0; i < WorldScaleGripPaceStationCount; ++i)
    {
        const auto& observation = input.Stations[i];
        const auto& source = reference.Nodes[i];
        const double age = input.SnapshotClockSeconds -
            observation.MeasurementClockSeconds;
        if (!observation.PhysicalRoadContactVerified ||
            !observation.RealFrictionMeasurementVerified ||
            observation.RoadComponentIdentity == 0 ||
            !std::isfinite(observation.StationCm) ||
            !std::isfinite(observation.MeasuredGripFraction) ||
            !std::isfinite(observation.SourceConfidence) ||
            !std::isfinite(observation.MeasurementClockSeconds) ||
            !std::isfinite(age) || age < 0.0 ||
            age > settings.MaximumMeasurementAgeSeconds ||
            std::abs(observation.StationCm - source.DistanceCm) >
                settings.MaximumStationMismatchCm ||
            observation.MeasuredGripFraction <
                settings.MinimumAcceptedGripFraction ||
            observation.MeasuredGripFraction >
                settings.MaximumAcceptedGripFraction ||
            observation.SourceConfidence <
                settings.MinimumMeasurementConfidence ||
            observation.SourceConfidence > 1.0 ||
            !std::isfinite(source.CurvaturePerCm) ||
            source.CurvaturePerCm < 0.0 ||
            !std::isfinite(source.LocalLimitCmps) ||
            !std::isfinite(source.TargetCmps) ||
            source.TargetCmps < WorldScalePaceMinSpeedCmps)
        {
            return {};
        }

        const double fraction = observation.MeasuredGripFraction;
        const double lateral = settings.BaselineLateralAccelCmS2 * fraction;
        const double gripLimitedSpeed = source.CurvaturePerCm <= 1.0e-12 ?
            WorldScalePaceMaxSpeedCmps :
            std::sqrt(lateral / source.CurvaturePerCm);
        const double cap = std::min({
            source.LocalLimitCmps, source.TargetCmps, gripLimitedSpeed
        });
        if (!std::isfinite(cap) || cap < WorldScalePaceMinSpeedCmps)
        {
            return {};
        }
        grip[i] = fraction;
        speedSquared[i] = cap * cap;
        plan.Nodes[i].StationCm = source.DistanceCm;
        plan.Nodes[i].VerifiedGripFraction = fraction;
        plan.Nodes[i].CurvaturePerCm = source.CurvaturePerCm;
        plan.Nodes[i].LocalSpeedLimitCmps = cap;
        minimumGrip = std::min(minimumGrip, fraction);
        oldestMeasurement = std::max(oldestMeasurement, age);
    }

    // For each edge use the WORSE of its two observed source-friction
    // stations. This prevents a sudden dry->wet seam from retaining the
    // old dry braking budget, or a wet->dry exit granting unmeasured grip.
    for (std::size_t i = 0; i < WorldScaleGripPaceStationCount; ++i)
    {
        const std::size_t next =
            (i + 1u) % WorldScaleGripPaceStationCount;
        const double edgeGrip = std::min(grip[i], grip[next]);
        plan.Nodes[i].MaxNextBrakeDeltaSpeedSquared =
            2.0 * WorldScalePaceStepCm *
            settings.BaselineBrakeAccelCmS2 * edgeGrip;
        plan.Nodes[i].MaxNextDriveDeltaSpeedSquared =
            2.0 * WorldScalePaceStepCm *
            settings.BaselineDriveAccelCmS2 * edgeGrip;
    }

    // Closed-loop constrained velocity planning. The source's previous
    // dry target speeds are UPPER bounds; this solver may ONLY lower
    // them. Repeating forward/reverse relaxation removes dependency on
    // whichever segment happens to be called the start line.
    bool settled = false;
    for (unsigned pass = 0; pass < WorldScalePaceStations; ++pass)
    {
        bool changed = false;
        for (std::size_t i = WorldScaleGripPaceStationCount;
             i-- > 0u;)
        {
            const std::size_t next =
                (i + 1u) % WorldScaleGripPaceStationCount;
            const double allowed = speedSquared[next] +
                plan.Nodes[i].MaxNextBrakeDeltaSpeedSquared;
            if (speedSquared[i] > allowed)
            {
                speedSquared[i] = allowed;
                changed = true;
            }
        }
        for (std::size_t i = 0; i < WorldScaleGripPaceStationCount; ++i)
        {
            const std::size_t next =
                (i + 1u) % WorldScaleGripPaceStationCount;
            const double allowed = speedSquared[i] +
                plan.Nodes[i].MaxNextDriveDeltaSpeedSquared;
            if (speedSquared[next] > allowed)
            {
                speedSquared[next] = allowed;
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
    for (std::size_t i = 0; i < WorldScaleGripPaceStationCount; ++i)
    {
        const auto next = (i + 1u) % WorldScaleGripPaceStationCount;
        const double squared = speedSquared[i];
        const double nextSquared = speedSquared[next];
        const double speed = std::sqrt(squared);
        const double nextSpeed = std::sqrt(nextSquared);
        if (!std::isfinite(speed) || !std::isfinite(nextSpeed) ||
            speed < WorldScalePaceMinSpeedCmps - 1.0e-6 ||
            squared > nextSquared +
                plan.Nodes[i].MaxNextBrakeDeltaSpeedSquared + 0.01 ||
            nextSquared > squared +
                plan.Nodes[i].MaxNextDriveDeltaSpeedSquared + 0.01 ||
            speed > plan.Nodes[i].LocalSpeedLimitCmps + 1.0e-6 ||
            speed > reference.Nodes[i].TargetCmps + 1.0e-6)
        {
            return {};
        }
        plan.Nodes[i].TargetSpeedCmps = speed;
        if (speed < reference.Nodes[i].TargetCmps - 1.0e-5)
        {
            ++plan.ReducedSpeedStations;
        }
        lapSeconds += 2.0 * WorldScalePaceStepCm /
            (speed + nextSpeed);
    }
    if (!std::isfinite(lapSeconds) ||
        lapSeconds < reference.EstimatedLapSeconds - 1.0e-6 ||
        lapSeconds > 1200.0)
    {
        return {};
    }
    plan.EstimatedLapSeconds = lapSeconds;
    plan.MinimumMeasuredGripFraction = minimumGrip;
    plan.MaximumMeasurementAgeSeconds = oldestMeasurement;
    plan.SourceSessionIdentity = input.SessionIdentity;
    plan.SourceClockSeconds = input.SnapshotClockSeconds;
    plan.Valid = true;
    return plan;
}

// Samples SQUARED velocity continuously along each verified station edge,
// including the signed and positive wraps at 0 -> WorldScaleCircuitLapCm.
// Never extrapolate beyond an unobserved station.
inline WorldScaleGripPaceReference SampleWorldScaleVariableGripPace(
    const WorldScaleVariableGripPacePlan& plan,
    double stationCm) noexcept
{
    if (!plan.Valid || !std::isfinite(stationCm)) return {};
    double wrapped = std::fmod(stationCm, WorldScaleCircuitLapCm);
    if (wrapped < 0.0) wrapped += WorldScaleCircuitLapCm;
    const auto i = std::min(
        static_cast<std::size_t>(wrapped / WorldScalePaceStepCm),
        WorldScaleGripPaceStationCount - 1u);
    const auto next = (i + 1u) % WorldScaleGripPaceStationCount;
    const double fraction = std::clamp(
        (wrapped - plan.Nodes[i].StationCm) /
            WorldScalePaceStepCm, 0.0, 1.0);
    const double v0 = plan.Nodes[i].TargetSpeedCmps;
    const double v1 = plan.Nodes[next].TargetSpeedCmps;
    const double squared = v0 * v0 +
        fraction * (v1 * v1 - v0 * v0);
    const double speed = std::sqrt(squared);
    if (!std::isfinite(speed) ||
        speed < WorldScalePaceMinSpeedCmps - 1.0e-6) return {};
    return {
        true, wrapped, speed,
        plan.Nodes[i].VerifiedGripFraction,
        plan.Nodes[next].VerifiedGripFraction
    };
}
} // namespace latebrake
