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
// Geometric proposals for NON-COLLIDING braking boards at the FUTURE
// 1.7km circuit, not physical roadside actor placement or verified
// sightline/pit-wall collision. No live race owns these positions.
inline constexpr unsigned WorldScaleMaximumTurnApproaches = 16u;
inline constexpr unsigned WorldScaleBoardDistancesPerTurn = 3u;
inline constexpr unsigned WorldScaleBoardSides = 2u;
inline constexpr unsigned WorldScaleMaximumApproachBoards =
    WorldScaleMaximumTurnApproaches * WorldScaleBoardDistancesPerTurn *
    WorldScaleBoardSides;

enum class WorldScaleApproachFault : std::uint8_t
{
    None, InvalidSettings, MissingReference,
    MissingPhysicalRoad, TooManyTurns, UnsafePlacement,
    IncompleteBoardSequence
};

struct WorldScaleApproachSettings final
{
    double CurveThresholdPerCm = 0.00006;
    double BoardLateralOffsetCm = 1150.0;
    double BoardHeightCm = 180.0;
    double MinimumSeparateBoardDistanceCm = 400.0;
    bool PlaceOnBothRoadSides = true;
    bool RequireBoardOutsidePhysicalRoad = true;
    bool DoNotCreatePhysicalCollision = true;
    bool NeverChangeActualBrakingAuthority = true;
};

struct WorldScaleApproachBoard final
{
    CircuitPoint GroundLocation{};
    double GroundTopCm = RoundedRoadTopCm;
    double BoardTopCm = 0.0;
    double FacingDegrees = 0.0;
    double StationCm = 0.0;
    double UpcomingCornerStationCm = 0.0;
    unsigned TurnIndex = 0;
    int RoadSide = 0;
    int DistanceToCornerMeters = 0;
    std::uint64_t UniqueBoardToken = 0;
    bool VisualOnly = true;
};

struct WorldScaleApproachBoardPlan final
{
    WorldScaleApproachFault Fault =
        WorldScaleApproachFault::MissingReference;
    std::array<WorldScaleApproachBoard,
               WorldScaleMaximumApproachBoards> Boards{};
    std::array<double, WorldScaleMaximumTurnApproaches> TurnStartsCm{};
    unsigned TurnCount = 0;
    unsigned BoardCount = 0;
    bool NoPhysicalCollision = true;
    bool NoRaceControlAuthority = true;
    bool Valid = false;
};

inline bool ValidWorldScaleApproachSettings(
    WorldScaleApproachSettings s) noexcept
{
    return std::isfinite(s.CurveThresholdPerCm) &&
        s.CurveThresholdPerCm >= 0.00003 &&
        s.CurveThresholdPerCm <= 0.00015 &&
        std::isfinite(s.BoardLateralOffsetCm) &&
        s.BoardLateralOffsetCm >= 900.0 &&
        s.BoardLateralOffsetCm <= 2200.0 &&
        std::isfinite(s.BoardHeightCm) &&
        s.BoardHeightCm >= 100.0 &&
        s.BoardHeightCm <= 350.0 &&
        std::isfinite(s.MinimumSeparateBoardDistanceCm) &&
        s.MinimumSeparateBoardDistanceCm > 100.0 &&
        s.MinimumSeparateBoardDistanceCm <= 1000.0 &&
        s.PlaceOnBothRoadSides &&
        s.RequireBoardOutsidePhysicalRoad &&
        s.DoNotCreatePhysicalCollision &&
        s.NeverChangeActualBrakingAuthority;
}

inline double WrapWorldScaleApproachStation(
    double stationCm) noexcept
{
    if (!std::isfinite(stationCm))
        return std::numeric_limits<double>::quiet_NaN();
    double wrapped = std::fmod(stationCm, WorldScaleCircuitLapCm);
    if (wrapped < 0.0) wrapped += WorldScaleCircuitLapCm;
    return wrapped;
}

inline bool WorldScaleApproachCurveAt(
    const WorldScalePacePlan& source,
    unsigned station,
    double curvatureThreshold) noexcept
{
    return source.Nodes[station % WorldScalePaceStations].CurvaturePerCm >
        curvatureThreshold;
}

inline WorldScaleApproachBoardPlan BuildWorldScaleApproachBoardPlan(
    const WorldScalePacePlan& source,
    WorldScaleApproachSettings settings = {}) noexcept
{
    WorldScaleApproachBoardPlan plan{};
    if (!ValidWorldScaleApproachSettings(settings))
    {
        plan.Fault = WorldScaleApproachFault::InvalidSettings;
        return plan;
    }
    if (!source.Valid)
    {
        plan.Fault = WorldScaleApproachFault::MissingReference;
        return plan;
    }

    // Cross the 511->0 seam explicitly, so a bend that begins near
    // the start line is detected once rather than being counted twice.
    for (unsigned i = 0; i < WorldScalePaceStations; ++i)
    {
        const unsigned before =
            (i + WorldScalePaceStations - 1u) %
            WorldScalePaceStations;
        const bool curved = WorldScaleApproachCurveAt(
            source, i, settings.CurveThresholdPerCm);
        const bool previous = WorldScaleApproachCurveAt(
            source, before, settings.CurveThresholdPerCm);
        if (!curved || previous) continue;
        if (plan.TurnCount >= WorldScaleMaximumTurnApproaches)
        {
            plan.Fault = WorldScaleApproachFault::TooManyTurns;
            return plan;
        }
        plan.TurnStartsCm[plan.TurnCount++] = source.Nodes[i].DistanceCm;
    }
    if (plan.TurnCount != 4u)
    {
        plan.Fault = WorldScaleApproachFault::MissingPhysicalRoad;
        return plan;
    }

    constexpr std::array<int, WorldScaleBoardDistancesPerTurn>
        DistancesMeters{{150, 100, 50}};
    for (unsigned turn = 0; turn < plan.TurnCount; ++turn)
    {
        const double corner = plan.TurnStartsCm[turn];
        for (int distanceMeters : DistancesMeters)
        {
            const double station = WrapWorldScaleApproachStation(
                corner - static_cast<double>(distanceMeters) * 100.0);
            const auto pose = WorldScaleCenterlineAt(station);
            if (!pose.Valid ||
                !IsOnWorldScaleRoad(pose.Pose.Position))
            {
                plan.Fault = WorldScaleApproachFault::MissingPhysicalRoad;
                return plan;
            }
            const double heading =
                pose.Pose.HeadingDegrees * WorldScalePaceRadPerDegree;
            const double nx = -std::sin(heading);
            const double ny = std::cos(heading);
            for (int side : {-1, 1})
            {
                const CircuitPoint position{
                    pose.Pose.Position.X +
                        side * nx * settings.BoardLateralOffsetCm,
                    pose.Pose.Position.Y +
                        side * ny * settings.BoardLateralOffsetCm
                };
                if (!std::isfinite(position.X) ||
                    !std::isfinite(position.Y) ||
                    std::abs(position.X) > 1000000.0 ||
                    std::abs(position.Y) > 1000000.0 ||
                    IsOnWorldScaleRoad(position))
                {
                    plan.Fault = WorldScaleApproachFault::UnsafePlacement;
                    return plan;
                }
                if (plan.BoardCount >= WorldScaleMaximumApproachBoards)
                {
                    plan.Fault = WorldScaleApproachFault::TooManyTurns;
                    return plan;
                }
                for (unsigned other = 0; other < plan.BoardCount; ++other)
                {
                    const auto& prior = plan.Boards[other];
                    const double dx =
                        prior.GroundLocation.X - position.X;
                    const double dy =
                        prior.GroundLocation.Y - position.Y;
                    if (std::hypot(dx, dy) <
                        settings.MinimumSeparateBoardDistanceCm)
                    {
                        plan.Fault =
                            WorldScaleApproachFault::UnsafePlacement;
                        return plan;
                    }
                }
                const std::uint64_t token =
                    100000u + static_cast<std::uint64_t>(turn) * 10u +
                    static_cast<std::uint64_t>(
                        distanceMeters == 150 ? 0 : distanceMeters == 100 ? 2 : 4) +
                    static_cast<std::uint64_t>(side + 1) / 2u;
                plan.Boards[plan.BoardCount++] = {
                    position,
                    RoundedRoadTopCm,
                    RoundedRoadTopCm + settings.BoardHeightCm,
                    std::fmod(pose.Pose.HeadingDegrees + 180.0, 360.0),
                    station, corner, turn, side, distanceMeters,
                    token, true
                };
            }
        }
    }
    if (plan.BoardCount != plan.TurnCount *
                WorldScaleBoardDistancesPerTurn *
                WorldScaleBoardSides)
    {
        plan.Fault = WorldScaleApproachFault::IncompleteBoardSequence;
        return plan;
    }
    plan.Fault = WorldScaleApproachFault::None;
    plan.Valid = true;
    return plan;
}

inline bool AuditWorldScaleApproachBoardPlan(
    const WorldScaleApproachBoardPlan& plan) noexcept
{
    if (!plan.Valid || plan.Fault != WorldScaleApproachFault::None ||
        !plan.NoPhysicalCollision || !plan.NoRaceControlAuthority ||
        plan.TurnCount != 4u ||
        plan.BoardCount != 24u)
    {
        return false;
    }
    std::array<unsigned, 4> perTurn{};
    for (unsigned i = 0; i < plan.BoardCount; ++i)
    {
        const auto& item = plan.Boards[i];
        if (!item.VisualOnly || item.TurnIndex >= plan.TurnCount ||
            !std::isfinite(item.StationCm) ||
            !std::isfinite(item.UpcomingCornerStationCm) ||
            !std::isfinite(item.GroundLocation.X) ||
            !std::isfinite(item.GroundLocation.Y) ||
            !std::isfinite(item.FacingDegrees) ||
            item.FacingDegrees < 0.0 ||
            item.FacingDegrees >= 360.0 ||
            item.RoadSide == 0 ||
            (item.DistanceToCornerMeters != 50 &&
             item.DistanceToCornerMeters != 100 &&
             item.DistanceToCornerMeters != 150) ||
            IsOnWorldScaleRoad(item.GroundLocation) ||
            item.BoardTopCm <= item.GroundTopCm)
        {
            return false;
        }
        for (unsigned j = 0; j < i; ++j)
        {
            if (plan.Boards[j].UniqueBoardToken ==
                    item.UniqueBoardToken ||
                (plan.Boards[j].TurnIndex == item.TurnIndex &&
                 plan.Boards[j].RoadSide == item.RoadSide &&
                 plan.Boards[j].DistanceToCornerMeters ==
                     item.DistanceToCornerMeters))
            {
                return false;
            }
        }
        ++perTurn[item.TurnIndex];
    }
    for (unsigned count : perTurn)
    {
        if (count != 6u) return false;
    }
    return true;
}
} // namespace latebrake
