#pragma once
#include "Racecraft/CircuitGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr int SmoothLinePieceCount = 8;

struct SmoothLineParameters final
{
    double CornerRadiusCm = 480.0;
    double MaximumLegalLateralOffsetCm = 250.0;
    double ChassisHalfWidthCm = 95.0;
    double MarginFromPavementCm = 85.0;
    double MaximumVehicleHalfLengthCm = 240.0;
    double MinimumSampleSpacingCm = 20.0;
    double MaximumSampleSpacingCm = 200.0;
    int MaximumRouteSamples = 512;
    bool RequireAsphaltGeometryProof = true;
    bool RejectCornerClip = true;
    bool CounterClockwise = true;
};

enum class SmoothLinePieceKind : std::uint8_t
{
    Straight, QuarterCircle
};

struct SmoothLinePiece final
{
    SmoothLinePieceKind Kind = SmoothLinePieceKind::Straight;
    Vec2 Start{};
    Vec2 End{};
    Vec2 ArcCenter{};
    double StartAngleRadians = 0.0;
    double RadiusCm = 0.0;
    double StartDistanceCm = 0.0;
    double LengthCm = 0.0;
    double StartHeadingRadians = 0.0;
    double EndHeadingRadians = 0.0;
    int OriginalSegment = -1;
    bool Qualified = false;
};

struct SmoothLineSample final
{
    Vec2 PositionCm{};
    Vec2 Tangent{};
    Vec2 LeftNormal{};
    double HeadingRadians = 0.0;
    double CurvaturePerCm = 0.0;
    double DistanceCm = 0.0;
    double NormalizedFraction = 0.0;
    double DistanceToNextCornerCm = 0.0;
    double SourceRoadMarginCm = 0.0;
    double SafeVehicleMarginCm = 0.0;
    int PieceIndex = -1;
    bool Curved = false;
    bool InsideAuthoredRoad = false;
    bool ChassisCenterQualified = false;
    bool SourceGeometryOnly = true;
    bool Valid = false;
};

struct SmoothLineProjection final
{
    SmoothLineSample Target{};
    double NearestDistanceCm = 0.0;
    double TrackDistanceCm = 0.0;
    double SignedLateralCm = 0.0;
    double HeadingErrorRadians = 0.0;
    double ProgressFraction = 0.0;
    double SourceRoadClearanceCm = 0.0;
    int PieceIndex = -1;
    bool MotionAligned = false;
    bool OnPavement = false;
    bool Qualified = false;
};

struct SmoothLineGeometry final
{
    std::array<SmoothLinePiece, SmoothLinePieceCount> Pieces{};
    double TotalLengthCm = 0.0;
    double StraightDistanceCm = 0.0;
    double CurveDistanceCm = 0.0;
    double MaximumAbsCurvaturePerCm = 0.0;
    double MinimumRoadMarginCm = 0.0;
    double MaximumLateralDeviationCm = 0.0;
    double CornerRadiusCm = 0.0;
    bool Continuous = false;
    bool Closed = false;
    bool CounterClockwise = true;
    bool SourceGeometryOnly = true;
    bool Valid = false;
};

inline bool ValidSmoothLineParameters(
    SmoothLineParameters s) noexcept
{
    return Finite(s.CornerRadiusCm) &&
        s.CornerRadiusCm >= 250.0 &&
        s.CornerRadiusCm <= 750.0 &&
        Finite(s.MaximumLegalLateralOffsetCm) &&
        s.MaximumLegalLateralOffsetCm >= 0.0 &&
        s.MaximumLegalLateralOffsetCm < RoadHalfWidth &&
        Finite(s.ChassisHalfWidthCm) &&
        s.ChassisHalfWidthCm > 0.0 &&
        s.ChassisHalfWidthCm <= 240.0 &&
        Finite(s.MarginFromPavementCm) &&
        s.MarginFromPavementCm >= 0.0 &&
        Finite(s.MaximumVehicleHalfLengthCm) &&
        s.MaximumVehicleHalfLengthCm > 0.0 &&
        Finite(s.MinimumSampleSpacingCm) &&
        s.MinimumSampleSpacingCm > 0.0 &&
        Finite(s.MaximumSampleSpacingCm) &&
        s.MaximumSampleSpacingCm >= s.MinimumSampleSpacingCm &&
        s.MaximumRouteSamples >= 32 &&
        s.MaximumRouteSamples <= 4096 &&
        s.ChassisHalfWidthCm + s.MarginFromPavementCm +
        s.MaximumLegalLateralOffsetCm < RoadHalfWidth;
}

inline SmoothLinePiece MakeStraightPiece(
    Vec2 start, Vec2 end, double distanceCm, int original) noexcept
{
    SmoothLinePiece piece;
    piece.Kind = SmoothLinePieceKind::Straight;
    piece.Start = start;
    piece.End = end;
    piece.LengthCm = Distance(start, end);
    piece.StartDistanceCm = distanceCm;
    piece.StartHeadingRadians = AngleOf(end - start);
    piece.EndHeadingRadians = piece.StartHeadingRadians;
    piece.OriginalSegment = original;
    piece.Qualified =
        Finite(piece.LengthCm) && piece.LengthCm > 0.0;
    return piece;
}

inline SmoothLinePiece MakeCornerPiece(
    Vec2 center, double radiusCm,
    double startingAngleRadians,
    double distanceCm, int original) noexcept
{
    SmoothLinePiece piece;
    piece.Kind = SmoothLinePieceKind::QuarterCircle;
    piece.ArcCenter = center;
    piece.RadiusCm = radiusCm;
    piece.StartAngleRadians = startingAngleRadians;
    piece.Start = center + HeadingVector(
        startingAngleRadians) * radiusCm;
    piece.End = center + HeadingVector(
        startingAngleRadians + Pi * 0.5) * radiusCm;
    piece.StartDistanceCm = distanceCm;
    piece.LengthCm = radiusCm * Pi * 0.5;
    piece.StartHeadingRadians =
        WrappedAngle(startingAngleRadians + Pi * 0.5);
    piece.EndHeadingRadians =
        WrappedAngle(startingAngleRadians + Pi);
    piece.OriginalSegment = original;
    piece.Qualified =
        Finite(piece.Start) && Finite(piece.End) &&
        Finite(piece.LengthCm) && piece.LengthCm > 0.0;
    return piece;
}

inline SmoothLineSample SampleSmoothLinePiece(
    SmoothLinePiece piece, double localFraction) noexcept
{
    SmoothLineSample sample;
    if (!piece.Qualified ||
        !Finite(localFraction)) return sample;
    const double t = Clamp01(localFraction);
    sample.PieceIndex = piece.OriginalSegment;
    sample.DistanceCm =
        piece.StartDistanceCm + piece.LengthCm * t;
    sample.NormalizedFraction = t;
    sample.Curved =
        piece.Kind == SmoothLinePieceKind::QuarterCircle;
    if (sample.Curved)
    {
        const double angle =
            piece.StartAngleRadians + Pi * 0.5 * t;
        sample.PositionCm =
            piece.ArcCenter +
            HeadingVector(angle) * piece.RadiusCm;
        sample.HeadingRadians =
            WrappedAngle(angle + Pi * 0.5);
        sample.CurvaturePerCm =
            SafeDivision(1.0, piece.RadiusCm);
    }
    else
    {
        sample.PositionCm =
            Lerp(piece.Start, piece.End, t);
        sample.HeadingRadians =
            piece.StartHeadingRadians;
    }
    sample.Tangent =
        HeadingVector(sample.HeadingRadians);
    sample.LeftNormal =
        PerpendicularLeft(sample.Tangent);
    sample.InsideAuthoredRoad =
        CenterOnOriginalRoad(sample.PositionCm);
    sample.ChassisCenterQualified =
        sample.InsideAuthoredRoad;
    sample.SourceRoadMarginCm =
        DistanceToNearestSlabEdge(sample.PositionCm);
    sample.SafeVehicleMarginCm =
        sample.SourceRoadMarginCm;
    sample.Valid =
        Finite(sample.PositionCm) &&
        Finite(sample.HeadingRadians);
    return sample;
}

inline SmoothLineGeometry BuildOriginalSmoothLine(
    SmoothLineParameters params = {}) noexcept
{
    SmoothLineGeometry result;
    if (!ValidSmoothLineParameters(params))
        return result;
    const double r = params.CornerRadiusCm;
    const double east = EastCenterX;
    const double west = WestCenterX;
    const double north = NorthCenterY;
    const double south = SouthCenterY;
    const std::array<SmoothLinePiece, SmoothLinePieceCount> pieces{{
        MakeStraightPiece({west+r,south}, {east-r,south},0.0,0),
        MakeCornerPiece({east-r,south+r},r,-Pi*0.5,0.0,0),
        MakeStraightPiece({east,south+r},{east,north-r},0.0,1),
        MakeCornerPiece({east-r,north-r},r,0.0,0.0,1),
        MakeStraightPiece({east-r,north},{west+r,north},0.0,2),
        MakeCornerPiece({west+r,north-r},r,Pi*0.5,0.0,2),
        MakeStraightPiece({west,north-r},{west,south+r},0.0,3),
        MakeCornerPiece({west+r,south+r},r,Pi,0.0,3)
    }};
    double offset = 0.0;
    result.MinimumRoadMarginCm =
        std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < pieces.size(); ++i)
    {
        SmoothLinePiece p = pieces[i];
        p.StartDistanceCm = offset;
        result.Pieces[i] = p;
        offset += p.LengthCm;
        if (p.Kind == SmoothLinePieceKind::Straight)
            result.StraightDistanceCm += p.LengthCm;
        else
        {
            result.CurveDistanceCm += p.LengthCm;
            result.MaximumAbsCurvaturePerCm = std::max(
                result.MaximumAbsCurvaturePerCm,
                SafeDivision(1.0, r));
        }
    }
    result.TotalLengthCm = offset;
    result.CornerRadiusCm = r;
    result.CounterClockwise = params.CounterClockwise;
    result.SourceGeometryOnly = true;
    result.Continuous = true;
    result.Closed = true;
    for (std::size_t i = 0; i < pieces.size(); ++i)
    {
        const SmoothLinePiece& a = result.Pieces[i];
        const SmoothLinePiece& b =
            result.Pieces[(i+1)%pieces.size()];
        if (!a.Qualified || Distance(a.End,b.Start)>1.0e-6 ||
            std::abs(AngleDifference(
                a.EndHeadingRadians,b.StartHeadingRadians))>1.0e-6)
            result.Continuous = false;
        const SmoothLineSample midpoint =
            SampleSmoothLinePiece(a,0.5);
        if (!midpoint.InsideAuthoredRoad)
            result.Closed = false;
        result.MinimumRoadMarginCm = std::min(
            result.MinimumRoadMarginCm,
            midpoint.SourceRoadMarginCm);
    }
    result.Valid = result.Closed &&
        result.Continuous &&
        result.MinimumRoadMarginCm >
            params.ChassisHalfWidthCm +
            params.MarginFromPavementCm;
    return result;
}

inline SmoothLineSample SampleSmoothRacingLine(
    SmoothLineGeometry geometry,
    double distanceCm) noexcept
{
    SmoothLineSample result;
    if (!geometry.Valid || !Finite(distanceCm))
        return result;
    const double s =
        WrapDistance(distanceCm,geometry.TotalLengthCm);
    for (std::size_t i=0;i<geometry.Pieces.size();++i)
    {
        const SmoothLinePiece& piece=geometry.Pieces[i];
        if (s >= piece.StartDistanceCm &&
            s < piece.StartDistanceCm+piece.LengthCm)
        {
            result=SampleSmoothLinePiece(piece,
                SafeDivision(s-piece.StartDistanceCm,
                             piece.LengthCm));
            result.PieceIndex=static_cast<int>(i);
            if (!result.Curved)
            {
                const int next =
                    static_cast<int>((i+1)%geometry.Pieces.size());
                result.DistanceToNextCornerCm =
                    geometry.Pieces[static_cast<std::size_t>(next)].Kind ==
                    SmoothLinePieceKind::QuarterCircle ?
                    piece.StartDistanceCm+piece.LengthCm-s :
                    0.0;
            }
            result.DistanceCm=s;
            return result;
        }
    }
    return result;
}

inline SmoothLineProjection ProjectToSmoothRacingLine(
    SmoothLineGeometry geometry, Vec2 world,
    double yawRadians=0.0,
    bool requireForward=true) noexcept
{
    SmoothLineProjection result;
    if (!geometry.Valid || !Finite(world) ||
        !Finite(yawRadians)) return result;
    double bestError =
        std::numeric_limits<double>::infinity();
    for (std::size_t i=0;i<geometry.Pieces.size();++i)
    {
        const auto& piece=geometry.Pieces[i];
        double t=0.0;
        if (piece.Kind==SmoothLinePieceKind::Straight)
        {
            const SegmentProjection p=ProjectToSegment(
                world,{piece.Start,piece.End});
            if (!p.Valid) continue;
            t=p.AlongFraction;
        }
        else
        {
            const double pointAngle=AngleOf(
                world-piece.ArcCenter);
            const double angleDelta=
                WrapDistance(pointAngle-piece.StartAngleRadians,
                             TwoPi);
            t=Clamp01(angleDelta/(Pi*0.5));
            if (angleDelta > Pi)
                t=0.0;
            else if (angleDelta>Pi*0.5)
                t=1.0;
        }
        SmoothLineSample candidate=
            SampleSmoothLinePiece(piece,t);
        candidate.PieceIndex=static_cast<int>(i);
        const double headingError=AngleDifference(
            yawRadians,candidate.HeadingRadians);
        const double headingPenalty =
            requireForward && std::cos(headingError)<0.0 ?
            25000.0:0.0;
        const double error=LengthSquared(
            world-candidate.PositionCm)+headingPenalty;
        if (error >= bestError) continue;
        bestError=error;
        result.Target=candidate;
        result.NearestDistanceCm=Distance(
            world,candidate.PositionCm);
        result.TrackDistanceCm=candidate.DistanceCm;
        result.SignedLateralCm=Cross(
            candidate.Tangent,world-candidate.PositionCm);
        result.HeadingErrorRadians=headingError;
        result.ProgressFraction=SafeDivision(
            candidate.DistanceCm,geometry.TotalLengthCm);
        result.SourceRoadClearanceCm=
            DistanceToNearestSlabEdge(world);
        result.PieceIndex=static_cast<int>(i);
        result.MotionAligned=std::cos(headingError)>=0.0;
        result.OnPavement=CenterOnOriginalRoad(world);
        result.Qualified=true;
    }
    return result;
}

inline double SmoothLineSignedProgress(
    SmoothLineGeometry geometry,
    double fromCm,double toCm) noexcept
{
    return geometry.Valid ?
        SignedShortestLapDistance(
            fromCm,toCm,geometry.TotalLengthCm):0.0;
}

inline SmoothLineSample LookaheadOnSmoothLine(
    SmoothLineGeometry geometry,
    SmoothLineProjection projection,
    double currentSpeedCmS,double seconds) noexcept
{
    if (!projection.Qualified ||
        !Finite(currentSpeedCmS) ||
        !Finite(seconds) ||
        currentSpeedCmS<0.0 || seconds<0.0)
        return {};
    return SampleSmoothRacingLine(
        geometry,projection.TrackDistanceCm+
        std::clamp(currentSpeedCmS*seconds,150.0,2800.0));
}

inline double SmoothLineRoadClearanceCm(
    SmoothLineGeometry geometry,double distanceCm,
    double bodyHalfWidthCm) noexcept
{
    if (!Finite(bodyHalfWidthCm) ||
        bodyHalfWidthCm<=0.0) return -1.0;
    const auto sample=SampleSmoothRacingLine(
        geometry,distanceCm);
    return sample.Valid ?
        sample.SourceRoadMarginCm-bodyHalfWidthCm:-1.0;
}

struct SmoothRouteQuality final
{
    double LengthCm=0.0;
    double PercentCurve=0.0;
    double AverageCurvaturePerCm=0.0;
    double HighestCurvaturePerCm=0.0;
    double SmallestRoadClearanceCm=0.0;
    double MaximumHeadingStepRadians=0.0;
    int PavedSampleCount=0;
    int RejectedSampleCount=0;
    int Samples=0;
    bool FullyInsideOriginalAsphalt=false;
    bool Continuous=false;
    bool Closed=false;
    bool Valid=false;
};

inline SmoothRouteQuality InspectSmoothRoute(
    SmoothLineGeometry geometry,
    double spacingCm=60.0,
    double carHalfWidthCm=95.0) noexcept
{
    SmoothRouteQuality result;
    if (!geometry.Valid ||
        !Finite(spacingCm) ||
        !Finite(carHalfWidthCm) ||
        spacingCm<5.0 || carHalfWidthCm<0.0)
        return result;
    const int count=std::clamp(
        static_cast<int>(std::ceil(
            geometry.TotalLengthCm/spacingCm)),16,4096);
    result.LengthCm=geometry.TotalLengthCm;
    result.PercentCurve=SafeDivision(
        geometry.CurveDistanceCm,
        geometry.TotalLengthCm);
    result.HighestCurvaturePerCm=
        geometry.MaximumAbsCurvaturePerCm;
    result.SmallestRoadClearanceCm=
        std::numeric_limits<double>::infinity();
    SmoothLineSample previous;
    for (int i=0;i<count;++i)
    {
        const auto sample=SampleSmoothRacingLine(
            geometry,geometry.TotalLengthCm*
                static_cast<double>(i)/count);
        ++result.Samples;
        if (!sample.Valid ||
            !sample.InsideAuthoredRoad ||
            sample.SourceRoadMarginCm<carHalfWidthCm)
        {
            ++result.RejectedSampleCount;
            continue;
        }
        ++result.PavedSampleCount;
        result.AverageCurvaturePerCm+=
            std::abs(sample.CurvaturePerCm);
        result.SmallestRoadClearanceCm=std::min(
            result.SmallestRoadClearanceCm,
            sample.SourceRoadMarginCm-carHalfWidthCm);
        if (previous.Valid)
            result.MaximumHeadingStepRadians=std::max(
                result.MaximumHeadingStepRadians,
                std::abs(AngleDifference(
                    previous.HeadingRadians,
                    sample.HeadingRadians)));
        previous=sample;
    }
    result.AverageCurvaturePerCm=SafeDivision(
        result.AverageCurvaturePerCm,result.PavedSampleCount);
    result.Continuous=geometry.Continuous;
    result.Closed=geometry.Closed;
    result.FullyInsideOriginalAsphalt=
        result.RejectedSampleCount==0;
    result.Valid=result.FullyInsideOriginalAsphalt &&
        result.Closed && result.Continuous;
    return result;
}

class SmoothLineTracker final
{
public:
    bool Configure(SmoothLineParameters settings) noexcept
    {
        const auto built=BuildOriginalSmoothLine(settings);
        if (!built.Valid) return false;
        Settings=settings;
        Geometry=built;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Accepted=0;
        Rejected=0;
        ReverseObservations=0;
        OffTrackObservations=0;
        TravelCm=0.0;
        LargestLateralCm=0.0;
        HasPrevious=false;
    }
    SmoothLineProjection Observe(
        Vec2 carPosition,double yawRadians,
        double deltaSeconds) noexcept
    {
        const auto sample=ProjectToSmoothRacingLine(
            Geometry,carPosition,yawRadians,true);
        if (!sample.Qualified ||
            !ValidFrameDelta(deltaSeconds))
        {
            ++Rejected;
            HasPrevious=false;
            return {};
        }
        if (HasPrevious)
        {
            const double forward=SmoothLineSignedProgress(
                Geometry,Last.TrackDistanceCm,
                sample.TrackDistanceCm);
            if (forward < -20.0)
                ++ReverseObservations;
            if (forward > 0.0 &&
                forward<MaximumPhysicalSpeedCmS*deltaSeconds+200.0)
                TravelCm+=forward;
        }
        if (!sample.OnPavement) ++OffTrackObservations;
        LargestLateralCm=std::max(
            LargestLateralCm,std::abs(sample.SignedLateralCm));
        Last=sample;
        HasPrevious=true;
        ++Accepted;
        return sample;
    }
    SmoothLineProjection Snapshot() const noexcept
    {
        return Last;
    }
    SmoothLineGeometry AuthoredLine() const noexcept
    {
        return Geometry;
    }
    std::uint64_t QualifiedFrames() const noexcept
    {
        return Accepted;
    }
    std::uint64_t RejectedFrames() const noexcept
    {
        return Rejected;
    }
    double DistanceAdvancedCm() const noexcept
    {
        return TravelCm;
    }
private:
    SmoothLineParameters Settings{};
    SmoothLineGeometry Geometry{};
    SmoothLineProjection Last{};
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t ReverseObservations=0;
    std::uint64_t OffTrackObservations=0;
    double TravelCm=0.0;
    double LargestLateralCm=0.0;
    bool HasPrevious=false;
};
} // namespace latebrake::racecraft
