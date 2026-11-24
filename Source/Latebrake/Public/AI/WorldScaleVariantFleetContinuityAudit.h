#pragma once

#include "Track/WorldScaleVariantRacePace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// PORTABLE SOURCE-ONLY fleet continuity audit for six measured cars on
// one of the six independently authored future circuit geometries.
// A trusted UE5.8 adapter would need to provide the six real Chaos
// actor sources, session/frame identities, physical road contacts,
// velocities and signed world-scale revolution counts. None are
// synthesized or queried here. This cannot award laps or standings.
inline constexpr unsigned WorldScaleVariantFleetCars = 6u;
inline constexpr double WorldScaleVariantFleetMaximumSpeedCmps=15000.0;
inline constexpr double WorldScaleVariantFleetMaximumFrameSeconds=0.10;
inline constexpr double WorldScaleVariantFleetMinimumFrameSeconds=0.001;
inline constexpr double WorldScaleVariantFleetMaxStationResidualCm=220.0;
inline constexpr double WorldScaleVariantFleetMinimumResidualSlackCm=5.0;
inline constexpr double WorldScaleVariantFleetMaximumClockSkewSeconds=0.02;

struct WorldScaleVariantFleetActor final
{
    int Slot=-1;
    std::uint64_t ActorToken=0;
    int SignedSpatialRevolution=0;
    double ReportedWrappedStationCm=0.0;
    CircuitPoint WorldPosition{};
    CircuitPoint WorldVelocityCmps{};
    bool OriginalRoadContactQualified=false;
};

struct WorldScaleVariantFleetFrame final
{
    std::uint64_t SessionToken=0;
    std::uint64_t FrameOrdinal=0;
    double DeltaSeconds=0.0;
    double ObservationClockSeconds=0.0;
    bool RaceGreen=false;
    bool PhysicsEngineQualified=false;
    bool SixRealActorsConfirmed=false;
    std::array<WorldScaleVariantFleetActor,
               WorldScaleVariantFleetCars> Actors{};
};

struct WorldScaleVariantFleetCarResult final
{
    int Slot=-1;
    std::uint64_t ActorToken=0;
    double MeasuredSpeedCmps=0.0;
    double SignedRouteSpeedCmps=0.0;
    double SourceLapStationCm=0.0;
    double UnwrappedProjectedStationCm=0.0;
    double UnwrappedReportedStationCm=0.0;
    double SignedLateralOffsetCm=0.0;
    double CenterlineResidualCm=0.0;
    bool WithinOriginalRoad=true;
};

struct WorldScaleVariantFleetReadout final
{
    bool Valid=false;
    bool MotionContinuous=false;
    WorldScaleVariantId TrackId=WorldScaleVariantId::OriginalLoop;
    std::uint64_t SourceSessionToken=0;
    std::uint64_t SourceFrameOrdinal=0;
    std::array<WorldScaleVariantFleetCarResult,
               WorldScaleVariantFleetCars> CarsBySlot{};
    std::array<unsigned,WorldScaleVariantFleetCars> AdvisoryOrder{};
    std::uint64_t AcceptedContinuousFrames=0;
    std::uint64_t RejectedFrames=0;
    double ContinuousPhysicalSeconds=0.0;
    double MaximumPositionPredictionErrorCm=0.0;
    double MaximumReportedStationErrorCm=0.0;
    bool NoOfficialLapCredit=true;
    bool DoesNotControlVehicles=true;
    bool AdvisoryOnly=true;
};

inline bool ValidWorldScaleVariantFleetActor(
    const WorldScaleVariantFleetActor& a,
    double lapLength) noexcept
{
    return a.Slot>=0 &&
        a.Slot<static_cast<int>(WorldScaleVariantFleetCars) &&
        a.ActorToken!=0 &&
        a.OriginalRoadContactQualified &&
        a.SignedSpatialRevolution>=-1000 &&
        a.SignedSpatialRevolution<=1000 &&
        std::isfinite(a.ReportedWrappedStationCm) &&
        a.ReportedWrappedStationCm>=0.0 &&
        a.ReportedWrappedStationCm<lapLength &&
        std::isfinite(a.WorldPosition.X) &&
        std::isfinite(a.WorldPosition.Y) &&
        std::abs(a.WorldPosition.X)<=1000000.0 &&
        std::abs(a.WorldPosition.Y)<=1000000.0 &&
        std::isfinite(a.WorldVelocityCmps.X) &&
        std::isfinite(a.WorldVelocityCmps.Y) &&
        std::hypot(a.WorldVelocityCmps.X,
                   a.WorldVelocityCmps.Y) <=
            WorldScaleVariantFleetMaximumSpeedCmps;
}

inline double WorldScaleVariantLapResidual(
    double reported,double projected,double length) noexcept
{
    if(!std::isfinite(reported) ||
       !std::isfinite(projected) ||
       !std::isfinite(length) || length<=0.0)
        return std::numeric_limits<double>::infinity();
    const double direct=std::abs(reported-projected);
    return std::min(direct,length-direct);
}

class WorldScaleVariantFleetContinuityAudit final
{
public:
    void Reset() noexcept
    {
        Session=0;
        LastFrame=0;
        LastObservationClockSeconds=0.0;
        Reference=nullptr;
        HavePrevious=false;
        Earlier={};
        EarlierReported={};
        EarlierSignedSpeed={};
        Accepted=0;
        Rejects=0;
        Elapsed=0.0;
        LastReadout={};
    }

    void Suspend() noexcept
    {
        ClearContinuity();
        LastReadout={};
        LastReadout.RejectedFrames=Rejects;
    }

    WorldScaleVariantFleetReadout Readout() const noexcept
    {
        return LastReadout;
    }

    WorldScaleVariantFleetReadout Observe(
        const WorldScaleVariantPacePlan& plan,
        const WorldScaleVariantFleetFrame& frame) noexcept
    {
        if(!plan.Valid ||
           !ValidWorldScaleVariantRecipe(plan.Track) ||
           !frame.RaceGreen ||
           !frame.PhysicsEngineQualified ||
           !frame.SixRealActorsConfirmed ||
           frame.SessionToken==0 ||
           frame.FrameOrdinal==0 ||
           !std::isfinite(frame.DeltaSeconds) ||
           frame.DeltaSeconds<
               WorldScaleVariantFleetMinimumFrameSeconds ||
           frame.DeltaSeconds>
               WorldScaleVariantFleetMaximumFrameSeconds ||
           !std::isfinite(frame.ObservationClockSeconds) ||
           frame.ObservationClockSeconds<0.0)
            return Reject();

        std::array<WorldScaleVariantFleetActor,
                   WorldScaleVariantFleetCars> ordered{};
        std::array<WorldScaleVariantFleetCarResult,
                   WorldScaleVariantFleetCars> reported{};
        std::array<bool,WorldScaleVariantFleetCars> seen{};
        std::array<double,WorldScaleVariantFleetCars> signedSpeed{};
        for(const auto& car:frame.Actors)
        {
            if(!ValidWorldScaleVariantFleetActor(
                car,plan.Track.LapLengthCm))
                return Reject();
            const auto index=static_cast<unsigned>(car.Slot);
            if(seen[index])return Reject();
            seen[index]=true;
            for(const auto& previous:frame.Actors)
            {
                if(&previous!=&car &&
                   previous.ActorToken==car.ActorToken)
                    return Reject();
            }
            if(!IsOnWorldScaleVariantRoad(
                plan.Track,car.WorldPosition))
                return Reject();

            const auto projection=ProjectWorldScaleVariantPace(
                plan,car.WorldPosition);
            if(!projection.Valid ||
               !projection.InsidePhysicalCandidateRoad ||
               !std::isfinite(projection.SignedLateralCm) ||
               !std::isfinite(projection.CentrelineSeparationCm) ||
               WorldScaleVariantLapResidual(
                   car.ReportedWrappedStationCm,
                   projection.LapDistanceCm,
                   plan.Track.LapLengthCm)>
                   WorldScaleVariantFleetMaxStationResidualCm)
                return Reject();

            const auto before=WorldScaleVariantCenterlineAt(
                plan.Track,projection.LapDistanceCm-90.0);
            const auto after=WorldScaleVariantCenterlineAt(
                plan.Track,projection.LapDistanceCm+90.0);
            if(!before.Valid || !after.Valid)return Reject();
            const double tangentX=after.Pose.Position.X-
                before.Pose.Position.X;
            const double tangentY=after.Pose.Position.Y-
                before.Pose.Position.Y;
            const double tangentLength=std::hypot(
                tangentX,tangentY);
            if(!std::isfinite(tangentLength) ||
               tangentLength<10.0)return Reject();
            const double progressSpeed=(
                car.WorldVelocityCmps.X*tangentX+
                car.WorldVelocityCmps.Y*tangentY)/tangentLength;
            if(!std::isfinite(progressSpeed))return Reject();
            const double lapBase=
                static_cast<double>(car.SignedSpatialRevolution)*
                    plan.Track.LapLengthCm;
            signedSpeed[index]=progressSpeed;
            ordered[index]=car;
            reported[index]={
                car.Slot,car.ActorToken,
                std::hypot(car.WorldVelocityCmps.X,
                           car.WorldVelocityCmps.Y),
                progressSpeed,
                projection.LapDistanceCm,
                lapBase+projection.LapDistanceCm,
                lapBase+car.ReportedWrappedStationCm,
                projection.SignedLateralCm,
                projection.CentrelineSeparationCm,
                true
            };
        }
        for(bool slot:seen) if(!slot)return Reject();

        const bool newSession=Session!=frame.SessionToken;
        if(newSession)
        {
            ClearContinuity();
            Session=frame.SessionToken;
            Reference=&plan;
        }
        // Same real race epoch is prohibited from switching track
        // geometry or from having two different pace-plan objects.
        if(!newSession && Reference!=&plan)
            return Reject();

        const bool successor=HavePrevious &&
            LastFrame!=std::numeric_limits<std::uint64_t>::max() &&
            frame.FrameOrdinal==LastFrame+1u;
        // A source adapter cannot present a strictly increasing frame
        // number while recycling a stale physics timestamp. Snapshot
        // time must advance by this same frame's measured delta.
        if(successor &&
           (!std::isfinite(LastObservationClockSeconds) ||
            frame.ObservationClockSeconds<=LastObservationClockSeconds ||
            std::abs(frame.ObservationClockSeconds-
                     LastObservationClockSeconds-frame.DeltaSeconds)>
                WorldScaleVariantFleetMaximumClockSkewSeconds))
            return Reject();
        double maxMotionError=0.0;
        double maxStationError=0.0;
        if(successor)
        {
            for(unsigned i=0;i<WorldScaleVariantFleetCars;++i)
            {
                const auto& old=Earlier[i];
                const auto& now=ordered[i];
                if(old.ActorToken!=now.ActorToken)
                    return Reject();
                const double expectedX=old.WorldPosition.X+
                    frame.DeltaSeconds*0.5*(
                        old.WorldVelocityCmps.X+
                        now.WorldVelocityCmps.X);
                const double expectedY=old.WorldPosition.Y+
                    frame.DeltaSeconds*0.5*(
                        old.WorldVelocityCmps.Y+
                        now.WorldVelocityCmps.Y);
                const double spatialError=std::hypot(
                    now.WorldPosition.X-expectedX,
                    now.WorldPosition.Y-expectedY);
                const double predictedStation=frame.DeltaSeconds*
                    0.5*(EarlierSignedSpeed[i]+signedSpeed[i]);
                const double measuredStation=
                    reported[i].UnwrappedReportedStationCm-
                    EarlierReported[i];
                const double estimateDistance=frame.DeltaSeconds*
                    0.5*(reported[i].MeasuredSpeedCmps+
                        std::hypot(old.WorldVelocityCmps.X,
                                   old.WorldVelocityCmps.Y));
                const double slack=std::max(
                    WorldScaleVariantFleetMinimumResidualSlackCm,
                    estimateDistance*0.15);
                const double stationError=std::abs(
                    measuredStation-predictedStation);
                if(!std::isfinite(spatialError) ||
                   !std::isfinite(stationError) ||
                   !std::isfinite(slack) ||
                   spatialError>slack ||
                   stationError>slack)
                    return Reject();
                maxMotionError=std::max(maxMotionError,spatialError);
                maxStationError=std::max(maxStationError,stationError);
            }
        }
        if(!successor)
        {
            Accepted=0;
            Elapsed=0.0;
        }
        else
        {
            ++Accepted;
            Elapsed+=frame.DeltaSeconds;
        }

        WorldScaleVariantFleetReadout result{};
        result.Valid=true;
        result.MotionContinuous=successor;
        result.TrackId=plan.Track.Id;
        result.SourceSessionToken=frame.SessionToken;
        result.SourceFrameOrdinal=frame.FrameOrdinal;
        result.CarsBySlot=reported;
        for(unsigned i=0;i<WorldScaleVariantFleetCars;++i)
            result.AdvisoryOrder[i]=i;
        std::sort(result.AdvisoryOrder.begin(),
                  result.AdvisoryOrder.end(),
            [&reported](unsigned a,unsigned b) noexcept
            {
                const double sa=
                    reported[a].UnwrappedProjectedStationCm;
                const double sb=
                    reported[b].UnwrappedProjectedStationCm;
                return sa>sb || (sa==sb && a<b);
            });
        result.AcceptedContinuousFrames=Accepted;
        result.RejectedFrames=Rejects;
        result.ContinuousPhysicalSeconds=Elapsed;
        result.MaximumPositionPredictionErrorCm=maxMotionError;
        result.MaximumReportedStationErrorCm=maxStationError;

        Earlier=ordered;
        for(unsigned i=0;i<WorldScaleVariantFleetCars;++i)
        {
            EarlierReported[i]=
                reported[i].UnwrappedReportedStationCm;
            EarlierSignedSpeed[i]=signedSpeed[i];
        }
        LastFrame=frame.FrameOrdinal;
        LastObservationClockSeconds=frame.ObservationClockSeconds;
        HavePrevious=true;
        LastReadout=result;
        return result;
    }

private:
    void ClearContinuity() noexcept
    {
        HavePrevious=false;
        LastFrame=0;
        LastObservationClockSeconds=0.0;
        Earlier={};
        EarlierReported={};
        EarlierSignedSpeed={};
        Accepted=0;
        Elapsed=0.0;
    }

    WorldScaleVariantFleetReadout Reject() noexcept
    {
        ++Rejects;
        ClearContinuity();
        LastReadout={};
        LastReadout.RejectedFrames=Rejects;
        return LastReadout;
    }

    std::uint64_t Session=0;
    std::uint64_t LastFrame=0;
    double LastObservationClockSeconds=0.0;
    const WorldScaleVariantPacePlan* Reference=nullptr;
    bool HavePrevious=false;
    std::array<WorldScaleVariantFleetActor,
               WorldScaleVariantFleetCars> Earlier{};
    std::array<double,WorldScaleVariantFleetCars> EarlierReported{};
    std::array<double,WorldScaleVariantFleetCars> EarlierSignedSpeed{};
    std::uint64_t Accepted=0;
    std::uint64_t Rejects=0;
    double Elapsed=0.0;
    WorldScaleVariantFleetReadout LastReadout{};
};
} // namespace latebrake
