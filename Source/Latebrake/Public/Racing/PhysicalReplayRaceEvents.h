#pragma once

#include "Racing/PhysicalSixCarReplayTimeline.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// A purely cosmetic broadcast-highlight detector for actual observed
// six-car replay frames. This is not track-ordered race classification,
// overtaking credit, a collision predictor or a steward's decision.
inline constexpr unsigned ReplayObservedPairCount=15u;
inline constexpr unsigned ReplayEventHistoryCapacity=64u;
inline constexpr double ReplayEventMaximumWorldGapCm=1800.0;
inline constexpr double ReplayEventMaximumSideGapCm=430.0;
inline constexpr double ReplayEventMinimumDirectionDot=0.86;

enum class ReplayObservedPairClass : std::uint8_t
{
    Unqualified,
    Distant,
    CloseFollowing,
    SideBySide
};

enum class ReplayBroadcastEventKind : std::uint8_t
{
    Unknown,
    CloseTraffic,
    SideBySide,
    ApparentPositionSwap,
    TrafficSeparated,
    StrongMeasuredBraking
};

struct ReplayEventParameters final
{
    double MaximumCloseDistanceCm=1800.0;
    double MaximumSameLaneWidthCm=320.0;
    double MaximumSideBySideLateralCm=430.0;
    double MaximumParallelYawErrorRadians=0.55;
    double SideBySideLongitudinalCm=270.0;
    double MinimumBrakingSpeedCmps=1000.0;
    double MinimumBrakingInput=0.68;
    double MinimumMeasurableBrakeDecelCmps2=200.0;
    double MaximumFrameSeconds=0.10;
    double MinimumFrameSeconds=0.001;
    bool RequireRealSixCarChaosSource=true;
    bool NeverAwardRacingPlace=true;
    bool NeverApplyActualBrake=true;
};

struct ReplayBroadcastEvent final
{
    bool Valid=false;
    ReplayBroadcastEventKind Kind=
        ReplayBroadcastEventKind::Unknown;
    std::uint64_t SourceSession=0;
    std::uint64_t SourceFrame=0;
    std::uint64_t PrimaryActor=0;
    std::uint64_t SecondaryActor=0;
    double ActiveClockSeconds=0.0;
    double WorldGapCm=0.0;
    double LongitudinalSeparationCm=0.0;
    double LateralSeparationCm=0.0;
    double ObservedSpeedDifferenceCmps=0.0;
    double MeasuredDecelCmps2=0.0;
    bool PhysicalSourceRequired=true;
    bool OfficialPassAwarded=false;
    bool GameplayAuthorityChanged=false;
};

struct ReplayEventReceipt final
{
    bool Valid=false;
    std::size_t StoredEvents=0u;
    unsigned NewEvents=0u;
    unsigned PairComparisons=0u;
    unsigned ParallelPairs=0u;
    std::uint64_t LastSourceFrame=0;
    bool OfficialPlaceUnchanged=true;
};

inline bool ValidReplayEventParameters(
    ReplayEventParameters p) noexcept
{
    const auto finite=[](double x) noexcept {
        return std::isfinite(x);
    };
    return finite(p.MaximumCloseDistanceCm) &&
        p.MaximumCloseDistanceCm>=700.0 &&
        p.MaximumCloseDistanceCm<=ReplayEventMaximumWorldGapCm &&
        finite(p.MaximumSameLaneWidthCm) &&
        p.MaximumSameLaneWidthCm>=150.0 &&
        p.MaximumSameLaneWidthCm<=ReplayEventMaximumSideGapCm &&
        finite(p.MaximumSideBySideLateralCm) &&
        p.MaximumSideBySideLateralCm>=p.MaximumSameLaneWidthCm &&
        p.MaximumSideBySideLateralCm<=ReplayEventMaximumSideGapCm &&
        finite(p.MaximumParallelYawErrorRadians) &&
        p.MaximumParallelYawErrorRadians>0.0 &&
        p.MaximumParallelYawErrorRadians<=0.60 &&
        finite(p.SideBySideLongitudinalCm) &&
        p.SideBySideLongitudinalCm>=200.0 &&
        p.SideBySideLongitudinalCm<=400.0 &&
        finite(p.MinimumBrakingSpeedCmps) &&
        p.MinimumBrakingSpeedCmps>=200.0 &&
        p.MinimumBrakingSpeedCmps<=5000.0 &&
        finite(p.MinimumBrakingInput) &&
        p.MinimumBrakingInput>=0.5 &&
        p.MinimumBrakingInput<=1.0 &&
        finite(p.MinimumMeasurableBrakeDecelCmps2) &&
        p.MinimumMeasurableBrakeDecelCmps2>=100.0 &&
        p.MinimumMeasurableBrakeDecelCmps2<=2500.0 &&
        finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=PhysicalReplayMaximumFrameSeconds &&
        finite(p.MinimumFrameSeconds) &&
        p.MinimumFrameSeconds>0.0 &&
        p.MinimumFrameSeconds<p.MaximumFrameSeconds &&
        p.RequireRealSixCarChaosSource &&
        p.NeverAwardRacingPlace &&
        p.NeverApplyActualBrake;
}

struct ReplayObservedPair final
{
    ReplayObservedPairClass Classification=
        ReplayObservedPairClass::Unqualified;
    double WorldGapCm=0.0;
    double AlongCm=0.0;
    double LateralCm=0.0;
    double SpeedDifferenceCmps=0.0;
    bool ParallelSourceDirections=false;
};

inline ReplayObservedPair ClassifyReplayPair(
    const PhysicalReplayCarSample& a,
    const PhysicalReplayCarSample& b,
    ReplayEventParameters settings={}) noexcept
{
    ReplayObservedPair result;
    if(!ValidReplayEventParameters(settings) ||
       !FinitePhysicalReplayCar(a) ||
       !FinitePhysicalReplayCar(b) ||
       a.Actor==b.Actor ||
       a.Session!=b.Session ||
       a.SessionRevision!=b.SessionRevision ||
       a.SourceFrame!=b.SourceFrame)
        return result;
    const double ax=std::cos(a.HeadingRadians);
    const double ay=std::sin(a.HeadingRadians);
    const double bx=std::cos(b.HeadingRadians);
    const double by=std::sin(b.HeadingRadians);
    const double dot=ax*bx+ay*by;
    const double angle=std::abs(std::remainder(
        a.HeadingRadians-b.HeadingRadians,
        2.0*3.1415926535897932384626433832795));
    const double dx=b.WorldXcm-a.WorldXcm;
    const double dy=b.WorldYcm-a.WorldYcm;
    const double dz=b.WorldZcm-a.WorldZcm;
    const double distance=std::hypot(std::hypot(dx,dy),dz);
    if(!std::isfinite(dot) || !std::isfinite(angle) ||
       !std::isfinite(distance))
        return result;
    result.WorldGapCm=distance;
    result.AlongCm=dx*ax+dy*ay;
    result.LateralCm=dx*(-ay)+dy*ax;
    result.SpeedDifferenceCmps=
        PhysicalReplaySpeed(b)-PhysicalReplaySpeed(a);
    result.ParallelSourceDirections=
        dot>=ReplayEventMinimumDirectionDot &&
        angle<=settings.MaximumParallelYawErrorRadians;
    result.Classification=ReplayObservedPairClass::Distant;
    if(!result.ParallelSourceDirections ||
       result.WorldGapCm>settings.MaximumCloseDistanceCm)
        return result;
    if(std::abs(result.AlongCm)<=
           settings.SideBySideLongitudinalCm &&
       std::abs(result.LateralCm)<=
           settings.MaximumSideBySideLateralCm)
    {
        result.Classification=ReplayObservedPairClass::SideBySide;
    }
    else if(std::abs(result.LateralCm)<=
                settings.MaximumSameLaneWidthCm)
    {
        result.Classification=
            ReplayObservedPairClass::CloseFollowing;
    }
    return result;
}

class PhysicalReplayRaceEventLog final
{
public:
    void Reset() noexcept
    {
        HavePrevious=false;
        Previous={};
        PairClasses.fill(ReplayObservedPairClass::Unqualified);
        EventCount=0u;
        NextEvent=0u;
    }

    std::size_t Size() const noexcept
    {
        return EventCount;
    }
    bool HasSource() const noexcept
    {
        return HavePrevious;
    }
    bool TryReadEvent(
        std::size_t recentOffset,
        ReplayBroadcastEvent& out) const noexcept
    {
        out={};
        if(recentOffset>=EventCount)return false;
        const std::size_t at=
            (NextEvent+ReplayEventHistoryCapacity-1u-
             recentOffset)%ReplayEventHistoryCapacity;
        out=Events[at];
        return true;
    }

    ReplayEventReceipt Observe(
        const PhysicalReplayFrame& frame,
        ReplayEventParameters settings={}) noexcept
    {
        ReplayEventReceipt receipt;
        if(!ValidReplayEventParameters(settings) ||
           ValidatePhysicalReplayFrame(frame)!=
                PhysicalReplayReject::None)
        {
            Reset();
            return receipt;
        }
        if(HavePrevious)
        {
            if(frame.Session!=Previous.Session ||
               frame.SessionRevision!=Previous.SessionRevision ||
               frame.SourceFrame<=Previous.SourceFrame ||
               frame.SourceFrame-Previous.SourceFrame>8u ||
               frame.ActiveClockSeconds<=Previous.ActiveClockSeconds)
            {
                Reset();
                return receipt;
            }
            const double dt=frame.ActiveClockSeconds-
                Previous.ActiveClockSeconds;
            if(!std::isfinite(dt) ||
               dt<settings.MinimumFrameSeconds ||
               dt>settings.MaximumFrameSeconds ||
               std::abs(dt-frame.SourceDeltaSeconds)>0.02)
            {
                Reset();
                return receipt;
            }
            for(std::size_t i=0u;i<PhysicalReplayCarCount;++i)
            {
                const auto& a=Previous.Cars[i];
                const auto& b=frame.Cars[i];
                if(a.Actor!=b.Actor)
                {
                    Reset();
                    return receipt;
                }
                const double moved=std::hypot(
                    std::hypot(b.WorldXcm-a.WorldXcm,
                               b.WorldYcm-a.WorldYcm),
                    b.WorldZcm-a.WorldZcm);
                const double maxMotion=dt*
                    std::max(PhysicalReplaySpeed(a),
                             PhysicalReplaySpeed(b))+
                    PhysicalReplayPositionResidualCm;
                if(!std::isfinite(moved) ||
                   !std::isfinite(maxMotion) ||
                   moved>maxMotion)
                {
                    Reset();
                    return receipt;
                }
            }
        }

        unsigned pairIndex=0u;
        for(std::size_t i=0u;i<PhysicalReplayCarCount;++i)
        {
            for(std::size_t j=i+1u;j<PhysicalReplayCarCount;++j)
            {
                ++receipt.PairComparisons;
                const auto observed=ClassifyReplayPair(
                    frame.Cars[i],frame.Cars[j],settings);
                if(observed.Classification==
                   ReplayObservedPairClass::Unqualified)
                {
                    Reset();
                    return {};
                }
                if(observed.ParallelSourceDirections)
                    ++receipt.ParallelPairs;
                const auto prior=PairClasses[pairIndex];
                if(HavePrevious)
                {
                    ReplayBroadcastEventKind event=
                        ReplayBroadcastEventKind::Unknown;
                    if(observed.Classification==
                           ReplayObservedPairClass::SideBySide &&
                       prior!=ReplayObservedPairClass::SideBySide)
                        event=ReplayBroadcastEventKind::SideBySide;
                    else if(observed.Classification==
                                ReplayObservedPairClass::CloseFollowing &&
                            prior==ReplayObservedPairClass::Distant)
                        event=ReplayBroadcastEventKind::CloseTraffic;
                    else if(observed.Classification==
                                ReplayObservedPairClass::Distant &&
                            (prior==ReplayObservedPairClass::SideBySide ||
                             prior==ReplayObservedPairClass::CloseFollowing))
                        event=ReplayBroadcastEventKind::TrafficSeparated;
                    if(prior==ReplayObservedPairClass::SideBySide &&
                       observed.Classification==
                           ReplayObservedPairClass::SideBySide)
                    {
                        const auto previous=ClassifyReplayPair(
                            Previous.Cars[i],
                            Previous.Cars[j],settings);
                        if(previous.Classification==
                               ReplayObservedPairClass::SideBySide &&
                           previous.AlongCm*observed.AlongCm<0.0 &&
                           std::abs(previous.AlongCm)>10.0 &&
                           std::abs(observed.AlongCm)>10.0)
                            event=ReplayBroadcastEventKind::ApparentPositionSwap;
                    }
                    if(event!=ReplayBroadcastEventKind::Unknown)
                    {
                        Append({true,event,frame.Session,
                            frame.SourceFrame,frame.Cars[i].Actor,
                            frame.Cars[j].Actor,
                            frame.ActiveClockSeconds,
                            observed.WorldGapCm,
                            observed.AlongCm,
                            observed.LateralCm,
                            observed.SpeedDifferenceCmps,
                            0.0,true,false,false});
                        ++receipt.NewEvents;
                    }
                }
                PairClasses[pairIndex++]=observed.Classification;
            }
        }

        if(HavePrevious)
        {
            const double dt=frame.ActiveClockSeconds-
                Previous.ActiveClockSeconds;
            for(std::size_t i=0u;i<PhysicalReplayCarCount;++i)
            {
                const auto& oldCar=Previous.Cars[i];
                const auto& newCar=frame.Cars[i];
                const double speed=PhysicalReplaySpeed(newCar);
                const double priorSpeed=PhysicalReplaySpeed(oldCar);
                const double decel=(priorSpeed-speed)/dt;
                if(std::isfinite(decel) &&
                   speed>=settings.MinimumBrakingSpeedCmps &&
                   decel>=settings.MinimumMeasurableBrakeDecelCmps2 &&
                   newCar.BrakeNormalized>=settings.MinimumBrakingInput &&
                   oldCar.BrakeNormalized<settings.MinimumBrakingInput)
                {
                    Append({true,
                        ReplayBroadcastEventKind::StrongMeasuredBraking,
                        frame.Session,frame.SourceFrame,newCar.Actor,0,
                        frame.ActiveClockSeconds,0.0,0.0,0.0,
                        speed-priorSpeed,decel,true,false,false});
                    ++receipt.NewEvents;
                }
            }
        }
        Previous=frame;
        HavePrevious=true;
        receipt.Valid=true;
        receipt.StoredEvents=EventCount;
        receipt.LastSourceFrame=frame.SourceFrame;
        return receipt;
    }

private:
    void Append(ReplayBroadcastEvent event) noexcept
    {
        Events[NextEvent]=event;
        NextEvent=(NextEvent+1u)%ReplayEventHistoryCapacity;
        if(EventCount<ReplayEventHistoryCapacity)++EventCount;
    }
    std::array<ReplayBroadcastEvent,
        ReplayEventHistoryCapacity> Events{};
    std::array<ReplayObservedPairClass,
        ReplayObservedPairCount> PairClasses{};
    PhysicalReplayFrame Previous{};
    std::size_t EventCount=0u;
    std::size_t NextEvent=0u;
    bool HavePrevious=false;
};
} // namespace latebrake
