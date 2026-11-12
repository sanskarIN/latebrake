#pragma once

#include "Telemetry/TelemetryMath.h"
#include "Telemetry/TelemetryTypes.h"
#include "Track/CircuitLayout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
struct TrackPoint final
{
    Vector2 PositionCm{};
    double RoadHalfWidthCm = 800.0;
    double TargetSpeedCmS = 0.0;
    int SectorIndex = 0;
    bool ApexMarker = false;
    bool PitEntry = false;
    bool PitExit = false;
};

struct TrackProjection final
{
    Vector2 ClosestPointCm{};
    Vector2 Tangent{};
    Vector2 Normal{};
    double DistanceAlongCm = 0.0;
    double SignedLateralCm = 0.0;
    double RoadHalfWidthCm = 0.0;
    double SegmentFraction = 0.0;
    double SegmentLengthCm = 0.0;
    double CurvaturePerCm = 0.0;
    double ForwardHeadingRadians = 0.0;
    double HeadingErrorRadians = 0.0;
    double SegmentCurvature = 0.0;
    double TargetSpeedCmS = 0.0;
    double DistanceToNextPointCm = 0.0;
    int Segment = -1;
    int SectorIndex = -1;
    bool OffRoad = false;
    bool OnPavedCenter = false;
    bool InPitTransition = false;
    bool Valid = false;
};

struct TrackProjectionSettings final
{
    double SearchPenaltyPerCentimeter = 0.1;
    double BacktrackPenaltyCm = 650.0;
    double BacktrackThresholdCm = 300.0;
    double AllowableLateralCm = 250.0;
    double MaximumProjectionErrorCm = 3000.0;
    double HeadingPenaltyCm = 550.0;
    int LocalSearchRadius = 6;
    bool PreferForwardProgress = true;
    bool LockToPreviousSegment = false;
};

inline bool ValidTrackProjectionSettings(
    const TrackProjectionSettings& s) noexcept
{
    return Finite(s.SearchPenaltyPerCentimeter) &&
           s.SearchPenaltyPerCentimeter >= 0.0 &&
           Finite(s.BacktrackPenaltyCm) &&
           s.BacktrackPenaltyCm >= 0.0 &&
           Finite(s.BacktrackThresholdCm) &&
           s.BacktrackThresholdCm >= 0.0 &&
           Finite(s.AllowableLateralCm) &&
           s.AllowableLateralCm >= 0.0 &&
           Finite(s.MaximumProjectionErrorCm) &&
           s.MaximumProjectionErrorCm >= 10.0 &&
           Finite(s.HeadingPenaltyCm) &&
           s.HeadingPenaltyCm >= 0.0 &&
           s.LocalSearchRadius >= 0 &&
           s.LocalSearchRadius <= MaximumTelemetryTrackNodes;
}

class TrackCenterline final
{
public:
    void Reset() noexcept
    {
        Nodes.fill({});
        Accumulated.fill(0.0);
        SegmentLengths.fill(0.0);
        NodeCount = 0;
        TotalLengthCm = 0.0;
        Closed = false;
        Ready = false;
    }

    bool Configure(
        const TrackPoint* points, int count, bool closed) noexcept
    {
        Reset();
        if (!points || count < 2 ||
            count > MaximumTelemetryTrackNodes ||
            (closed && count < 3))
        {
            return false;
        }
        for (int i = 0; i < count; ++i)
        {
            const TrackPoint& p = points[i];
            if (!Finite(p.PositionCm) ||
                !Finite(p.RoadHalfWidthCm) ||
                p.RoadHalfWidthCm <= 10.0 ||
                p.RoadHalfWidthCm > 10000.0 ||
                !Finite(p.TargetSpeedCmS) ||
                p.TargetSpeedCmS < 0.0 ||
                p.TargetSpeedCmS > MaximumPlanarSpeedCmS ||
                p.SectorIndex < 0 ||
                p.SectorIndex >= MaximumTelemetrySectors)
            {
                Reset();
                return false;
            }
            Nodes[static_cast<std::size_t>(i)] = p;
        }
        for (int i = 0; i < (closed ? count : count - 1); ++i)
        {
            const Vector2 begin =
                Nodes[static_cast<std::size_t>(i)].PositionCm;
            const Vector2 end =
                Nodes[static_cast<std::size_t>((i + 1) % count)]
                    .PositionCm;
            const double length = Distance(begin, end);
            if (!Finite(length) || length < 10.0 ||
                length > 1000000.0)
            {
                Reset();
                return false;
            }
            SegmentLengths[static_cast<std::size_t>(i)] = length;
            Accumulated[static_cast<std::size_t>(i)] = TotalLengthCm;
            TotalLengthCm += length;
        }
        if (!Finite(TotalLengthCm) || TotalLengthCm <= 0.0)
        {
            Reset();
            return false;
        }
        NodeCount = count;
        Closed = closed;
        Ready = true;
        return true;
    }

    bool ConfigureOriginalCircuit() noexcept
    {
        const std::array<TrackPoint, 4> p{{
            {{latebrake::EastX, latebrake::SouthY}, 700.0, 1200.0, 0, true},
            {{latebrake::EastX, latebrake::NorthY}, 700.0, 1200.0, 1, true},
            {{latebrake::WestX, latebrake::NorthY}, 700.0, 1200.0, 2, true},
            {{latebrake::WestX, latebrake::SouthY}, 700.0, 1200.0, 3, true}
        }};
        return Configure(p.data(), static_cast<int>(p.size()), true);
    }

    bool IsReady() const noexcept { return Ready; }
    bool IsClosed() const noexcept { return Closed; }
    int Points() const noexcept { return NodeCount; }
    int Segments() const noexcept
    {
        return Ready ? (Closed ? NodeCount : NodeCount - 1) : 0;
    }
    double LengthCm() const noexcept { return TotalLengthCm; }

    TrackPoint Point(int index) const noexcept
    {
        return Ready && index >= 0 && index < NodeCount ?
            Nodes[static_cast<std::size_t>(index)] : TrackPoint{};
    }

    double SegmentLength(int segment) const noexcept
    {
        return Ready && segment >= 0 && segment < Segments() ?
            SegmentLengths[static_cast<std::size_t>(segment)] : 0.0;
    }

    double BeginningOf(int segment) const noexcept
    {
        return Ready && segment >= 0 && segment < Segments() ?
            Accumulated[static_cast<std::size_t>(segment)] : 0.0;
    }

    double NormalizeDistance(double distanceCm) const noexcept
    {
        if (!Ready || !Finite(distanceCm))
        {
            return 0.0;
        }
        if (!Closed)
        {
            return std::clamp(distanceCm, 0.0, TotalLengthCm);
        }
        double wrapped = std::fmod(distanceCm, TotalLengthCm);
        if (wrapped < 0.0)
        {
            wrapped += TotalLengthCm;
        }
        return wrapped;
    }

    int SegmentAt(double distanceCm) const noexcept
    {
        if (!Ready || !Finite(distanceCm))
        {
            return -1;
        }
        const double normalized = NormalizeDistance(distanceCm);
        for (int i = Segments() - 1; i >= 0; --i)
        {
            if (normalized >= Accumulated[static_cast<std::size_t>(i)])
            {
                return i;
            }
        }
        return 0;
    }

    Vector2 PositionAt(double distanceCm) const noexcept
    {
        const int segment = SegmentAt(distanceCm);
        if (segment < 0)
        {
            return {};
        }
        const int next = (segment + 1) % NodeCount;
        const double t = Clamp01(
            SafeRatio(NormalizeDistance(distanceCm) -
                      BeginningOf(segment), SegmentLength(segment)));
        return Lerp(Nodes[static_cast<std::size_t>(segment)].PositionCm,
                    Nodes[static_cast<std::size_t>(next)].PositionCm, t);
    }

    Vector2 TangentAt(double distanceCm) const noexcept
    {
        const int segment = SegmentAt(distanceCm);
        if (segment < 0)
        {
            return {};
        }
        return NormalizeOrZero(
            Nodes[static_cast<std::size_t>((segment + 1) % NodeCount)]
                .PositionCm -
            Nodes[static_cast<std::size_t>(segment)].PositionCm);
    }

    double HalfWidthAt(double distanceCm) const noexcept
    {
        const int segment = SegmentAt(distanceCm);
        if (segment < 0)
        {
            return 0.0;
        }
        const int next = (segment + 1) % NodeCount;
        const double t = Clamp01(
            SafeRatio(NormalizeDistance(distanceCm) -
                      BeginningOf(segment), SegmentLength(segment)));
        return Lerp(Nodes[static_cast<std::size_t>(segment)].RoadHalfWidthCm,
                    Nodes[static_cast<std::size_t>(next)].RoadHalfWidthCm, t);
    }

    double TargetSpeedAt(double distanceCm) const noexcept
    {
        const int segment = SegmentAt(distanceCm);
        if (segment < 0)
        {
            return 0.0;
        }
        const int next = (segment + 1) % NodeCount;
        const double t = Clamp01(
            SafeRatio(NormalizeDistance(distanceCm) -
                      BeginningOf(segment), SegmentLength(segment)));
        const double a = Nodes[static_cast<std::size_t>(segment)]
            .TargetSpeedCmS;
        const double b = Nodes[static_cast<std::size_t>(next)]
            .TargetSpeedCmS;
        return Lerp(a, b, t);
    }

    double DistanceForward(double from, double to) const noexcept
    {
        if (!Ready || !Finite(from) || !Finite(to))
        {
            return 0.0;
        }
        const double begin = NormalizeDistance(from);
        const double finish = NormalizeDistance(to);
        if (!Closed)
        {
            return std::max(0.0, finish - begin);
        }
        const double raw = finish - begin;
        return raw >= 0.0 ? raw : raw + TotalLengthCm;
    }

    double SignedShortestTrackDistance(
        double from, double to) const noexcept
    {
        if (!Ready || !Finite(from) || !Finite(to))
        {
            return 0.0;
        }
        const double forward = DistanceForward(from, to);
        if (!Closed)
        {
            return NormalizeDistance(to) - NormalizeDistance(from);
        }
        return forward <= TotalLengthCm * 0.5 ?
            forward : forward - TotalLengthCm;
    }

    double LocalCurvature(int segment) const noexcept
    {
        if (!Ready || segment < 0 || segment >= Segments())
        {
            return 0.0;
        }
        if (!Closed && (segment == 0 || segment >= NodeCount - 2))
        {
            return 0.0;
        }
        const int previous = (segment + NodeCount - 1) % NodeCount;
        const int next = (segment + 1) % NodeCount;
        return Curvature(
            Nodes[static_cast<std::size_t>(previous)].PositionCm,
            Nodes[static_cast<std::size_t>(segment)].PositionCm,
            Nodes[static_cast<std::size_t>(next)].PositionCm);
    }

    TrackProjection Project(
        Vector2 position, double heading = 0.0,
        int preferredSegment = -1,
        TrackProjectionSettings settings = {}) const noexcept
    {
        TrackProjection result;
        if (!Ready || !Finite(position) || !Finite(heading) ||
            !ValidTrackProjectionSettings(settings))
        {
            return result;
        }
        double bestCost = std::numeric_limits<double>::infinity();
        const Vector2 headingUnit = HeadingUnit(heading);
        for (int i = 0; i < Segments(); ++i)
        {
            if (settings.LockToPreviousSegment && preferredSegment >= 0)
            {
                const int difference = std::abs(i - preferredSegment);
                const int wrappedDifference = Closed ?
                    std::min(difference, Segments() - difference) :
                    difference;
                if (wrappedDifference > settings.LocalSearchRadius)
                {
                    continue;
                }
            }
            const Vector2 begin =
                Nodes[static_cast<std::size_t>(i)].PositionCm;
            const Vector2 end =
                Nodes[static_cast<std::size_t>((i + 1) % NodeCount)]
                    .PositionCm;
            const Vector2 edge = end - begin;
            const double length = SegmentLength(i);
            const Vector2 tangent = edge / length;
            const double fraction = Clamp01(
                Dot(position - begin, edge) / (length * length));
            const Vector2 nearest = begin + edge * fraction;
            const double lateral = Cross(tangent, position - nearest);
            const double error = std::abs(lateral);
            const double headingCost =
                (1.0 - std::clamp(Dot(tangent, headingUnit), -1.0, 1.0)) *
                settings.HeadingPenaltyCm;
            double continuity = 0.0;
            if (preferredSegment >= 0 &&
                preferredSegment < Segments())
            {
                const int difference = std::abs(i - preferredSegment);
                const int wrappedDifference = Closed ?
                    std::min(difference, Segments() - difference) :
                    difference;
                continuity = static_cast<double>(wrappedDifference) *
                    settings.SearchPenaltyPerCentimeter;
            }
            const double cost = error + headingCost + continuity;
            if (cost >= bestCost)
            {
                continue;
            }
            bestCost = cost;
            const int next = (i + 1) % NodeCount;
            const TrackPoint& p = Nodes[static_cast<std::size_t>(i)];
            const TrackPoint& q = Nodes[static_cast<std::size_t>(next)];
            result.ClosestPointCm = nearest;
            result.Tangent = tangent;
            result.Normal = PerpendicularLeft(tangent);
            result.DistanceAlongCm = BeginningOf(i) + fraction * length;
            result.SignedLateralCm = lateral;
            result.RoadHalfWidthCm = Lerp(
                p.RoadHalfWidthCm, q.RoadHalfWidthCm, fraction);
            result.SegmentFraction = fraction;
            result.SegmentLengthCm = length;
            result.CurvaturePerCm = LocalCurvature(i);
            result.ForwardHeadingRadians =
                std::atan2(tangent.Y, tangent.X);
            result.HeadingErrorRadians = AngleDifference(
                heading, result.ForwardHeadingRadians);
            result.SegmentCurvature = result.CurvaturePerCm;
            result.TargetSpeedCmS = Lerp(
                p.TargetSpeedCmS, q.TargetSpeedCmS, fraction);
            result.DistanceToNextPointCm = (1.0 - fraction) * length;
            result.Segment = i;
            result.SectorIndex = p.SectorIndex;
            result.InPitTransition = p.PitEntry || p.PitExit ||
                                     q.PitEntry || q.PitExit;
            result.OffRoad = error > result.RoadHalfWidthCm;
            result.OnPavedCenter = !result.OffRoad;
            result.Valid = true;
        }
        if (result.Valid &&
            std::abs(result.SignedLateralCm) >
                settings.MaximumProjectionErrorCm)
        {
            return {};
        }
        return result;
    }

    TrackProjection ProjectNear(
        Vector2 position, double heading, int previousSegment,
        double previousDistanceCm,
        TrackProjectionSettings settings = {}) const noexcept
    {
        TrackProjection result =
            Project(position, heading, previousSegment, settings);
        if (!result.Valid || previousSegment < 0 ||
            !settings.PreferForwardProgress)
        {
            return result;
        }
        const double delta = SignedShortestTrackDistance(
            previousDistanceCm, result.DistanceAlongCm);
        if (delta < -settings.BacktrackThresholdCm &&
            std::abs(result.SignedLateralCm) >
                settings.AllowableLateralCm)
        {
            const TrackProjection locally =
                Project(position, heading, previousSegment, {
                    settings.SearchPenaltyPerCentimeter,
                    settings.BacktrackPenaltyCm,
                    settings.BacktrackThresholdCm,
                    settings.AllowableLateralCm,
                    settings.MaximumProjectionErrorCm,
                    settings.HeadingPenaltyCm,
                    std::max(1, settings.LocalSearchRadius),
                    settings.PreferForwardProgress,
                    true
                });
            if (locally.Valid &&
                SignedShortestTrackDistance(
                    previousDistanceCm, locally.DistanceAlongCm) >=
                    -settings.BacktrackThresholdCm)
            {
                return locally;
            }
        }
        return result;
    }

    bool ContainsCenter(Vector2 position) const noexcept
    {
        const TrackProjection p = Project(position);
        return p.Valid && !p.OffRoad;
    }

    double MeanRoadWidthCm() const noexcept
    {
        if (!Ready)
        {
            return 0.0;
        }
        double sum = 0.0;
        for (int i = 0; i < NodeCount; ++i)
        {
            sum += 2.0 * Nodes[static_cast<std::size_t>(i)]
                .RoadHalfWidthCm;
        }
        return sum / static_cast<double>(NodeCount);
    }

    bool MarkPitEntry(int segment) noexcept
    {
        if (!Ready || segment < 0 || segment >= NodeCount)
        {
            return false;
        }
        Nodes[static_cast<std::size_t>(segment)].PitEntry = true;
        return true;
    }

    bool MarkPitExit(int segment) noexcept
    {
        if (!Ready || segment < 0 || segment >= NodeCount)
        {
            return false;
        }
        Nodes[static_cast<std::size_t>(segment)].PitExit = true;
        return true;
    }

private:
    std::array<TrackPoint, MaximumTelemetryTrackNodes> Nodes{};
    std::array<double, MaximumTelemetryTrackNodes> Accumulated{};
    std::array<double, MaximumTelemetryTrackNodes> SegmentLengths{};
    int NodeCount = 0;
    double TotalLengthCm = 0.0;
    bool Closed = false;
    bool Ready = false;
};

struct TrackProgressObservation final
{
    TrackProjection Projection{};
    double UnwrappedDistanceCm = 0.0;
    double SignedTravelCm = 0.0;
    double CrossTrackErrorCm = 0.0;
    double CrossTrackErrorIntegral = 0.0;
    double MaximumCrossTrackErrorCm = 0.0;
    double WrongWaySeconds = 0.0;
    int CompletedTrackWraps = 0;
    bool WrongWay = false;
    bool TeleportSuspected = false;
    bool Accepted = false;
};

class TrackProgressEstimator final
{
public:
    void Reset() noexcept
    {
        Previous = {};
        HasPrevious = false;
        UnwrappedDistanceCm = 0.0;
        ErrorIntegral = 0.0;
        MaximumError = 0.0;
        WrongWayDuration = 0.0;
        Wraps = 0;
        Last = {};
    }

    bool Configure(
        const TrackCenterline* track,
        TrackProjectionSettings settings = {}) noexcept
    {
        if (!track || !track->IsReady() ||
            !ValidTrackProjectionSettings(settings))
        {
            return false;
        }
        Track = track;
        Settings = settings;
        Reset();
        return true;
    }

    TrackProgressObservation Observe(
        Vector2 position, double heading, double dt,
        double expectedMaxTravelCm = 900.0) noexcept
    {
        TrackProgressObservation out;
        if (!Track || !Track->IsReady() ||
            !Finite(position) || !Finite(heading) ||
            !Finite(dt) || dt <= 0.0 || dt > 0.5 ||
            !Finite(expectedMaxTravelCm) ||
            expectedMaxTravelCm <= 0.0)
        {
            Last = out;
            return out;
        }
        const TrackProjection result = HasPrevious ?
            Track->ProjectNear(position, heading,
                Previous.Segment, Previous.DistanceAlongCm, Settings) :
            Track->Project(position, heading);
        if (!result.Valid)
        {
            Last = out;
            return out;
        }
        out.Projection = result;
        out.CrossTrackErrorCm =
            std::abs(result.SignedLateralCm);
        MaximumError = std::max(MaximumError, out.CrossTrackErrorCm);
        ErrorIntegral += out.CrossTrackErrorCm * dt;
        out.CrossTrackErrorIntegral = ErrorIntegral;
        out.MaximumCrossTrackErrorCm = MaximumError;
        out.Accepted = true;
        if (HasPrevious)
        {
            const double travel = Track->SignedShortestTrackDistance(
                Previous.DistanceAlongCm, result.DistanceAlongCm);
            out.SignedTravelCm = travel;
            if (std::abs(travel) > expectedMaxTravelCm)
            {
                out.TeleportSuspected = true;
                out.Accepted = false;
                HasPrevious = false;
                Last = out;
                return out;
            }
            UnwrappedDistanceCm += travel;
            if (Track->IsClosed())
            {
                const double before = Previous.DistanceAlongCm;
                const double after = result.DistanceAlongCm;
                const double half = Track->LengthCm() / 2.0;
                if (before > half && after < half && travel > 0.0)
                {
                    ++Wraps;
                }
                else if (before < half && after > half && travel < 0.0)
                {
                    --Wraps;
                }
            }
            out.WrongWay = travel < -25.0 &&
                std::abs(result.HeadingErrorRadians) > Pi * 0.5;
            WrongWayDuration = out.WrongWay ?
                WrongWayDuration + dt : std::max(0.0, WrongWayDuration - dt);
        }
        else
        {
            UnwrappedDistanceCm = result.DistanceAlongCm;
        }
        Previous = result;
        HasPrevious = true;
        out.UnwrappedDistanceCm = UnwrappedDistanceCm;
        out.WrongWaySeconds = WrongWayDuration;
        out.CompletedTrackWraps = Wraps;
        Last = out;
        return out;
    }

    TrackProgressObservation Snapshot() const noexcept
    {
        return Last;
    }

    bool Ready() const noexcept { return HasPrevious; }
private:
    const TrackCenterline* Track = nullptr;
    TrackProjectionSettings Settings{};
    TrackProjection Previous{};
    double UnwrappedDistanceCm = 0.0;
    double ErrorIntegral = 0.0;
    double MaximumError = 0.0;
    double WrongWayDuration = 0.0;
    int Wraps = 0;
    bool HasPrevious = false;
    TrackProgressObservation Last{};
};
} // namespace latebrake::engineering
