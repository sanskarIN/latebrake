#pragma once

#include "Track/WorldScaleRoundedCircuit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace latebrake
{
// SOURCE ONLY: render-only roadside Armco-placement proposals for a
// future 1.7km circuit. Not a cooked collision mesh, not a safety
// barrier and not part of current real Chaos driving. A future
// native owner must separately author collision-bearing barriers.
inline constexpr std::size_t WorldScaleGuardrailLevels = 3u;
inline constexpr std::size_t WorldScaleGuardrailMaximumSegments = 768u;
inline constexpr std::size_t WorldScaleGuardrailMaximumPanels =
    WorldScaleGuardrailMaximumSegments * WorldScaleGuardrailLevels * 2u;

enum class WorldScaleGuardrailFault : std::uint8_t
{
    None, InvalidSettings, InvalidPhysicalRoad,
    InvalidOutsidePlacement, ImpossibleSpan,
    TooManySections, IncompleteInventory
};

struct WorldScaleGuardrailSettings final
{
    double RequestedPanelSpacingCm = 400.0;
    double MaximumAllowedPanelSpacingCm = 450.0;
    double LateralDistanceFromCenterCm = 990.0;
    double MaximumWorldCoordinateCm = 150000.0;
    double RailFaceHeightCm = 18.0;
    double RailBaseCm = 47.0;
    double VerticalRailSpacingCm = 43.0;
    double PostHeightCm = 180.0;
    double PostSquareWidthCm = 20.0;
    bool MustRemainOutsideOriginalRoad = true;
    bool MustKeepRoadCollisionUnchanged = true;
    bool MustNotModifyRaceControl = true;
    bool ExplicitlyVisualOnly = true;
};

struct WorldScaleGuardrailPoint final
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

struct WorldScaleGuardrailPanel final
{
    std::array<WorldScaleGuardrailPoint, 4> Corners{};
    double StartStationCm = 0.0;
    double EndStationCm = 0.0;
    double NormalizedYawDegrees = 0.0;
    std::uint64_t RenderToken = 0;
    unsigned SourceSection = 0;
    unsigned RailLayer = 0;
    int Side = 0;
    bool VisualOnly = true;
};

struct WorldScaleGuardrailPost final
{
    WorldScaleGuardrailPoint Base{};
    WorldScaleGuardrailPoint Top{};
    double WidthCm = 0.0;
    double FacingDegrees = 0.0;
    std::uint64_t RenderToken = 0;
    unsigned SourceSection = 0;
    int Side = 0;
    bool VisualOnly = true;
};

struct WorldScaleGuardrailPlan final
{
    std::vector<WorldScaleGuardrailPanel> Panels{};
    std::vector<WorldScaleGuardrailPost> Posts{};
    WorldScaleGuardrailFault Fault =
        WorldScaleGuardrailFault::InvalidSettings;
    std::array<std::size_t, WorldScaleGuardrailLevels> LayerPanels{};
    unsigned Sections = 0;
    unsigned PhysicalSectionMidpointsChecked = 0;
    double MaximumPhysicalPanelChordCm = 0.0;
    bool IsCollisionBearing = false;
    bool OriginalPhysicalRoadUnchanged = true;
    bool OriginalRaceRulesUnchanged = true;
    bool Valid = false;
};

inline bool ValidWorldScaleGuardrailSettings(
    const WorldScaleGuardrailSettings& s) noexcept
{
    return std::isfinite(s.RequestedPanelSpacingCm) &&
        s.RequestedPanelSpacingCm >= 220.0 &&
        s.RequestedPanelSpacingCm <= 440.0 &&
        std::isfinite(s.MaximumAllowedPanelSpacingCm) &&
        s.MaximumAllowedPanelSpacingCm >=
            s.RequestedPanelSpacingCm &&
        s.MaximumAllowedPanelSpacingCm <= 460.0 &&
        std::isfinite(s.LateralDistanceFromCenterCm) &&
        s.LateralDistanceFromCenterCm >= 850.0 &&
        s.LateralDistanceFromCenterCm <= 1500.0 &&
        std::isfinite(s.MaximumWorldCoordinateCm) &&
        s.MaximumWorldCoordinateCm >= 100000.0 &&
        s.MaximumWorldCoordinateCm <= 1000000.0 &&
        std::isfinite(s.RailFaceHeightCm) &&
        s.RailFaceHeightCm >= 10.0 &&
        s.RailFaceHeightCm <= 30.0 &&
        std::isfinite(s.RailBaseCm) &&
        s.RailBaseCm >= 25.0 &&
        s.RailBaseCm <= 70.0 &&
        std::isfinite(s.VerticalRailSpacingCm) &&
        s.VerticalRailSpacingCm >= 30.0 &&
        s.VerticalRailSpacingCm <= 55.0 &&
        std::isfinite(s.PostHeightCm) &&
        s.PostHeightCm >= 150.0 &&
        s.PostHeightCm <= 220.0 &&
        s.RailBaseCm +
            (WorldScaleGuardrailLevels - 1u) *
                s.VerticalRailSpacingCm + s.RailFaceHeightCm <=
            s.PostHeightCm &&
        std::isfinite(s.PostSquareWidthCm) &&
        s.PostSquareWidthCm >= 12.0 &&
        s.PostSquareWidthCm <= 32.0 &&
        s.MustRemainOutsideOriginalRoad &&
        s.MustKeepRoadCollisionUnchanged &&
        s.MustNotModifyRaceControl &&
        s.ExplicitlyVisualOnly;
}

inline WorldScaleGuardrailPoint WorldScaleGuardrailLocation(
    RoundedCenterlinePose source,
    double lateralOffset,
    double altitude) noexcept
{
    const double rad =
        source.Pose.HeadingDegrees * RoundedPi / 180.0;
    return {
        source.Pose.Position.X - std::sin(rad) * lateralOffset,
        source.Pose.Position.Y + std::cos(rad) * lateralOffset,
        altitude
    };
}

inline bool WorldScaleGuardrailOutsidePhysicalRoad(
    WorldScaleGuardrailPoint p,
    double maxCoordinate) noexcept
{
    return std::isfinite(p.X) &&
        std::isfinite(p.Y) && std::isfinite(p.Z) &&
        std::abs(p.X) < maxCoordinate &&
        std::abs(p.Y) < maxCoordinate &&
        !IsOnWorldScaleRoad({p.X, p.Y});
}

inline WorldScaleGuardrailPlan BuildWorldScaleGuardrailScene(
    WorldScaleGuardrailSettings s = {})
{
    WorldScaleGuardrailPlan plan{};
    if (!ValidWorldScaleGuardrailSettings(s))
        return plan;
    const unsigned sections = static_cast<unsigned>(
        std::ceil(WorldScaleCircuitLapCm / s.RequestedPanelSpacingCm));
    if (sections < 64u ||
        sections > WorldScaleGuardrailMaximumSegments)
    {
        plan.Fault = WorldScaleGuardrailFault::TooManySections;
        return plan;
    }
    const double step = WorldScaleCircuitLapCm / sections;
    if (step > s.MaximumAllowedPanelSpacingCm)
    {
        plan.Fault = WorldScaleGuardrailFault::ImpossibleSpan;
        return plan;
    }

    plan.Panels.reserve(
        static_cast<std::size_t>(sections) *
            WorldScaleGuardrailLevels * 2u);
    plan.Posts.reserve(static_cast<std::size_t>(sections) * 2u);
    for (unsigned section = 0; section < sections; ++section)
    {
        const double station = section * step;
        const double nextStation = (section + 1u) * step;
        const auto start = WorldScaleCenterlineAt(station);
        const auto end = WorldScaleCenterlineAt(nextStation);
        const auto midpoint = WorldScaleCenterlineAt(
            0.5 * (station + nextStation));
        if (!start.Valid || !end.Valid || !midpoint.Valid ||
            !IsOnWorldScaleRoad(start.Pose.Position) ||
            !IsOnWorldScaleRoad(end.Pose.Position) ||
            !IsOnWorldScaleRoad(midpoint.Pose.Position))
        {
            plan.Fault = WorldScaleGuardrailFault::InvalidPhysicalRoad;
            return {};
        }

        for (int side : {-1,1})
        {
            const double distance =
                side * s.LateralDistanceFromCenterCm;
            const auto a = WorldScaleGuardrailLocation(
                start, distance, RoundedRoadTopCm);
            const auto b = WorldScaleGuardrailLocation(
                end, distance, RoundedRoadTopCm);
            const auto m = WorldScaleGuardrailLocation(
                midpoint, distance, RoundedRoadTopCm);
            if (!WorldScaleGuardrailOutsidePhysicalRoad(
                    a,s.MaximumWorldCoordinateCm) ||
                !WorldScaleGuardrailOutsidePhysicalRoad(
                    b,s.MaximumWorldCoordinateCm) ||
                !WorldScaleGuardrailOutsidePhysicalRoad(
                    m,s.MaximumWorldCoordinateCm))
            {
                plan.Fault =
                    WorldScaleGuardrailFault::InvalidOutsidePlacement;
                return {};
            }
            const double chord = std::hypot(
                b.X - a.X, b.Y - a.Y);
            if (!std::isfinite(chord) || chord < 20.0 ||
                chord > s.MaximumAllowedPanelSpacingCm * 1.2)
            {
                plan.Fault = WorldScaleGuardrailFault::ImpossibleSpan;
                return {};
            }
            plan.MaximumPhysicalPanelChordCm = std::max(
                plan.MaximumPhysicalPanelChordCm,chord);
            ++plan.PhysicalSectionMidpointsChecked;
            const std::uint64_t token =
                100000u + static_cast<std::uint64_t>(section) * 16u +
                static_cast<std::uint64_t>(side + 1) / 2u * 8u;
            WorldScaleGuardrailPost post{};
            post.Base = a;
            post.Top = {a.X,a.Y,RoundedRoadTopCm+s.PostHeightCm};
            post.WidthCm = s.PostSquareWidthCm;
            post.FacingDegrees = start.Pose.HeadingDegrees;
            post.RenderToken = token + 7u;
            post.SourceSection = section;
            post.Side = side;
            plan.Posts.push_back(post);

            for (unsigned layer = 0;
                 layer < WorldScaleGuardrailLevels; ++layer)
            {
                const double bottom = RoundedRoadTopCm +
                    s.RailBaseCm +
                    layer * s.VerticalRailSpacingCm;
                const double top = bottom + s.RailFaceHeightCm;
                WorldScaleGuardrailPanel panel{};
                panel.Corners = {{
                    {a.X,a.Y,bottom},
                    {b.X,b.Y,bottom},
                    {b.X,b.Y,top},
                    {a.X,a.Y,top}
                }};
                panel.StartStationCm = station;
                panel.EndStationCm = nextStation;
                panel.NormalizedYawDegrees =
                    std::fmod(start.Pose.HeadingDegrees + 360.0,360.0);
                panel.RenderToken = token + layer;
                panel.SourceSection = section;
                panel.RailLayer = layer;
                panel.Side = side;
                plan.Panels.push_back(panel);
                ++plan.LayerPanels[layer];
            }
        }
    }
    plan.Sections = sections;
    if (plan.Panels.size() !=
            static_cast<std::size_t>(sections) *
                WorldScaleGuardrailLevels * 2u ||
        plan.Posts.size() !=
            static_cast<std::size_t>(sections) * 2u ||
        plan.PhysicalSectionMidpointsChecked != sections * 2u ||
        plan.MaximumPhysicalPanelChordCm >
            s.MaximumAllowedPanelSpacingCm * 1.2)
    {
        plan.Fault = WorldScaleGuardrailFault::IncompleteInventory;
        return {};
    }
    plan.Fault = WorldScaleGuardrailFault::None;
    plan.Valid = true;
    return plan;
}

inline bool AuditWorldScaleGuardrailScene(
    const WorldScaleGuardrailPlan& plan,
    double maxWorldCoordinate = 150000.0) noexcept
{
    if (!plan.Valid ||
        plan.Fault != WorldScaleGuardrailFault::None ||
        plan.IsCollisionBearing ||
        !plan.OriginalPhysicalRoadUnchanged ||
        !plan.OriginalRaceRulesUnchanged ||
        plan.Sections < 64u ||
        plan.Sections > WorldScaleGuardrailMaximumSegments ||
        plan.Panels.size() !=
            static_cast<std::size_t>(plan.Sections) *
                WorldScaleGuardrailLevels * 2u ||
        plan.Posts.size() !=
            static_cast<std::size_t>(plan.Sections) * 2u)
    {
        return false;
    }
    std::array<std::size_t,WorldScaleGuardrailLevels> counts{};
    for (const auto& panel : plan.Panels)
    {
        if (!panel.VisualOnly ||
            panel.SourceSection >= plan.Sections ||
            panel.RailLayer >= WorldScaleGuardrailLevels ||
            (panel.Side != 1 && panel.Side != -1) ||
            panel.EndStationCm <= panel.StartStationCm ||
            !std::isfinite(panel.NormalizedYawDegrees) ||
            panel.NormalizedYawDegrees < 0.0 ||
            panel.NormalizedYawDegrees >= 360.0)
        {
            return false;
        }
        for (const auto& corner:panel.Corners)
        {
            if (!WorldScaleGuardrailOutsidePhysicalRoad(
                    corner,maxWorldCoordinate))
                return false;
        }
        const auto& a = panel.Corners[0];
        const auto& b = panel.Corners[1];
        const auto& c = panel.Corners[2];
        const double dx = b.X - a.X;
        const double dy = b.Y - a.Y;
        const double dz = c.Z - b.Z;
        if (!std::isfinite(dx) || !std::isfinite(dy) ||
            !std::isfinite(dz) ||
            std::hypot(dx,dy) < 20.0 || dz < 10.0)
            return false;
        ++counts[panel.RailLayer];
    }
    if (counts != plan.LayerPanels) return false;
    for (const auto& post:plan.Posts)
    {
        if (!post.VisualOnly ||
            post.SourceSection >= plan.Sections ||
            (post.Side != -1 && post.Side != 1) ||
            post.Top.Z <= post.Base.Z ||
            !WorldScaleGuardrailOutsidePhysicalRoad(
                post.Base,maxWorldCoordinate) ||
            !WorldScaleGuardrailOutsidePhysicalRoad(
                post.Top,maxWorldCoordinate))
        {
            return false;
        }
    }
    return true;
}
} // namespace latebrake
