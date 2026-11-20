#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Bounded, original, C++20 source-only SIX-CAR OBSERVATION REPLAY.
// No real Unreal actor is spawned, moved or authenticated here; the
// eventual native adapter must provide all true Chaos body observations.
// Playback is cosmetic: never replays collisions, awards lap credits,
// changes the race clock, overwrites records, or reports official ranking.
inline constexpr std::size_t PhysicalReplayCarCount=6u;
inline constexpr std::size_t PhysicalReplayFrameCapacity=480u;
inline constexpr double PhysicalReplayMaximumWorldCm=1000000.0;
inline constexpr double PhysicalReplayMaximumSpeedCmps=15000.0;
inline constexpr double PhysicalReplayMaximumFrameSeconds=0.10;
inline constexpr double PhysicalReplayPositionResidualCm=175.0;
inline constexpr double PhysicalReplayClockEpsilon=1e-8;

enum class PhysicalReplayReject : std::uint8_t
{
    None,
    BadSource,
    BadRaceEpoch,
    WrongRoster,
    TimeRollback,
    FrameDiscontinuity,
    InvalidChassis,
    ImpossibleMotion,
    MissingRoadEvidence,
    UnsupportedInterpolation
};

struct PhysicalReplayCarSample final
{
    std::uint64_t Actor=0;
    std::uint64_t Session=0;
    std::uint64_t SessionRevision=0;
    std::uint64_t SourceFrame=0;
    double WorldXcm=0.0;
    double WorldYcm=0.0;
    double WorldZcm=0.0;
    double VelocityXcmps=0.0;
    double VelocityYcmps=0.0;
    double VelocityZcmps=0.0;
    double HeadingRadians=0.0;
    double SteeringNormalized=0.0;
    double ThrottleNormalized=0.0;
    double BrakeNormalized=0.0;
    double RoadContactConfidence=1.0;
    bool ActualChaosBodyObserved=false;
    bool OriginalRoadPhysicalContact=false;
    bool EngineClockRunning=false;
};

struct PhysicalReplayFrame final
{
    std::uint64_t Session=0;
    std::uint64_t SessionRevision=0;
    std::uint64_t SourceFrame=0;
    double ActiveClockSeconds=0.0;
    double SourceDeltaSeconds=0.0;
    std::array<PhysicalReplayCarSample,PhysicalReplayCarCount> Cars{};
    bool AllSixEngineActorsObserved=false;
    bool SourceOnly=true;
};

struct PhysicalReplayReceipt final
{
    bool Accepted=false;
    PhysicalReplayReject Rejection=PhysicalReplayReject::BadSource;
    std::size_t StoredFrames=0u;
    std::uint64_t AcceptedSourceFrame=0;
    std::uint64_t Session=0;
    std::uint64_t SessionRevision=0;
    double LatestClockSeconds=0.0;
    bool GameplayAuthorityChanged=false;
};

struct PhysicalReplayPlaybackCar final
{
    std::uint64_t Actor=0;
    double Xcm=0.0;
    double Ycm=0.0;
    double Zcm=0.0;
    double HeadingRadians=0.0;
    double ObservedSpeedCmps=0.0;
    double SteeringNormalized=0.0;
    double ThrottleNormalized=0.0;
    double BrakeNormalized=0.0;
    bool ProvenanceMatched=false;
};

struct PhysicalReplayPlaybackFrame final
{
    bool Valid=false;
    std::uint64_t Session=0;
    std::uint64_t SessionRevision=0;
    std::uint64_t LowerSourceFrame=0;
    std::uint64_t UpperSourceFrame=0;
    double ActiveClockSeconds=0.0;
    double InterpolationFraction=0.0;
    std::array<PhysicalReplayPlaybackCar,PhysicalReplayCarCount> Cars{};
    bool SourceOnly=true;
    bool NoCollisionAuthority=true;
    bool NoOfficialTimingAuthority=true;
};

inline bool FinitePhysicalReplayCar(
    const PhysicalReplayCarSample& car) noexcept
{
    const auto finite=[](double x) noexcept {
        return std::isfinite(x);
    };
    if(car.Actor==0 || car.Session==0 ||
       car.SessionRevision==0 ||
       car.SourceFrame==0 ||
       !car.ActualChaosBodyObserved ||
       !car.OriginalRoadPhysicalContact ||
       !car.EngineClockRunning ||
       !finite(car.WorldXcm) ||
       !finite(car.WorldYcm) ||
       !finite(car.WorldZcm) ||
       !finite(car.VelocityXcmps) ||
       !finite(car.VelocityYcmps) ||
       !finite(car.VelocityZcmps) ||
       !finite(car.HeadingRadians) ||
       !finite(car.SteeringNormalized) ||
       !finite(car.ThrottleNormalized) ||
       !finite(car.BrakeNormalized) ||
       !finite(car.RoadContactConfidence) ||
       std::abs(car.WorldXcm)>PhysicalReplayMaximumWorldCm ||
       std::abs(car.WorldYcm)>PhysicalReplayMaximumWorldCm ||
       std::abs(car.WorldZcm)>PhysicalReplayMaximumWorldCm ||
       std::abs(car.HeadingRadians)>1000.0 ||
       std::abs(car.SteeringNormalized)>1.0 ||
       car.ThrottleNormalized<0.0 ||
       car.ThrottleNormalized>1.0 ||
       car.BrakeNormalized<0.0 ||
       car.BrakeNormalized>1.0 ||
       car.RoadContactConfidence<0.8 ||
       car.RoadContactConfidence>1.0)
        return false;
    const double speed=std::hypot(
        std::hypot(car.VelocityXcmps,
                   car.VelocityYcmps),car.VelocityZcmps);
    return finite(speed) && speed<=PhysicalReplayMaximumSpeedCmps;
}

inline PhysicalReplayReject ValidatePhysicalReplayFrame(
    const PhysicalReplayFrame& frame) noexcept
{
    if(!frame.SourceOnly ||
       !frame.AllSixEngineActorsObserved ||
       frame.Session==0 ||
       frame.SessionRevision==0 ||
       frame.SourceFrame==0 ||
       !std::isfinite(frame.ActiveClockSeconds) ||
       !std::isfinite(frame.SourceDeltaSeconds) ||
       frame.ActiveClockSeconds<=0.0 ||
       frame.ActiveClockSeconds>86400.0 ||
       frame.SourceDeltaSeconds<=0.0 ||
       frame.SourceDeltaSeconds>PhysicalReplayMaximumFrameSeconds)
        return PhysicalReplayReject::BadSource;
    for(std::size_t i=0;i<PhysicalReplayCarCount;++i)
    {
        const auto& car=frame.Cars[i];
        if(!FinitePhysicalReplayCar(car))
            return car.OriginalRoadPhysicalContact ?
                PhysicalReplayReject::InvalidChassis :
                PhysicalReplayReject::MissingRoadEvidence;
        if(car.Session!=frame.Session ||
           car.SessionRevision!=frame.SessionRevision ||
           car.SourceFrame!=frame.SourceFrame)
            return PhysicalReplayReject::BadRaceEpoch;
        for(std::size_t j=0;j<i;++j)
            if(frame.Cars[j].Actor==car.Actor)
                return PhysicalReplayReject::WrongRoster;
    }
    return PhysicalReplayReject::None;
}

inline double PhysicalReplaySpeed(
    const PhysicalReplayCarSample& sample) noexcept
{
    return std::hypot(std::hypot(
        sample.VelocityXcmps,sample.VelocityYcmps),
        sample.VelocityZcmps);
}

class PhysicalSixCarReplayTimeline final
{
public:
    void Reset() noexcept
    {
        Size=0u;
        Next=0u;
        Session=0;
        Revision=0;
        LastReject=PhysicalReplayReject::None;
        ActorRoster.fill(0);
    }

    PhysicalReplayReceipt Record(
        const PhysicalReplayFrame& frame) noexcept
    {
        PhysicalReplayReceipt out;
        out.Rejection=ValidatePhysicalReplayFrame(frame);
        out.StoredFrames=Size;
        if(out.Rejection!=PhysicalReplayReject::None)
        {
            LastReject=out.Rejection;
            return out;
        }
        if(Size>0u)
        {
            const auto& previous=FrameAt(Size-1u);
            if(frame.Session!=Session ||
               frame.SessionRevision!=Revision)
            {
                out.Rejection=PhysicalReplayReject::BadRaceEpoch;
                LastReject=out.Rejection;
                return out;
            }
            if(frame.SourceFrame<=previous.SourceFrame ||
               frame.SourceFrame-previous.SourceFrame>8u)
            {
                out.Rejection=
                    PhysicalReplayReject::FrameDiscontinuity;
                LastReject=out.Rejection;
                return out;
            }
            const double dt=frame.ActiveClockSeconds-
                previous.ActiveClockSeconds;
            if(!std::isfinite(dt) ||
               dt<=PhysicalReplayClockEpsilon ||
               dt>PhysicalReplayMaximumFrameSeconds ||
               std::abs(dt-frame.SourceDeltaSeconds)>0.02)
            {
                out.Rejection=PhysicalReplayReject::TimeRollback;
                LastReject=out.Rejection;
                return out;
            }
            for(std::size_t i=0u;i<PhysicalReplayCarCount;++i)
            {
                const auto& now=frame.Cars[i];
                const auto& prior=previous.Cars[i];
                if(now.Actor!=ActorRoster[i] ||
                   prior.Actor!=ActorRoster[i])
                {
                    out.Rejection=PhysicalReplayReject::WrongRoster;
                    LastReject=out.Rejection;
                    return out;
                }
                const double dx=now.WorldXcm-prior.WorldXcm;
                const double dy=now.WorldYcm-prior.WorldYcm;
                const double dz=now.WorldZcm-prior.WorldZcm;
                const double motion=std::hypot(
                    std::hypot(dx,dy),dz);
                const double maximum=dt*std::max(
                    PhysicalReplaySpeed(now),
                    PhysicalReplaySpeed(prior))+
                    PhysicalReplayPositionResidualCm;
                if(!std::isfinite(motion) ||
                   !std::isfinite(maximum) ||
                   motion>maximum)
                {
                    out.Rejection=PhysicalReplayReject::ImpossibleMotion;
                    LastReject=out.Rejection;
                    return out;
                }
            }
        }
        else
        {
            Session=frame.Session;
            Revision=frame.SessionRevision;
            for(std::size_t i=0u;i<PhysicalReplayCarCount;++i)
                ActorRoster[i]=frame.Cars[i].Actor;
        }
        Frames[Next]=frame;
        Next=(Next+1u)%PhysicalReplayFrameCapacity;
        if(Size<PhysicalReplayFrameCapacity)++Size;
        LastReject=PhysicalReplayReject::None;
        out.Accepted=true;
        out.Rejection=PhysicalReplayReject::None;
        out.StoredFrames=Size;
        out.Session=Session;
        out.SessionRevision=Revision;
        out.AcceptedSourceFrame=frame.SourceFrame;
        out.LatestClockSeconds=frame.ActiveClockSeconds;
        out.GameplayAuthorityChanged=false;
        return out;
    }

    bool HasReplay() const noexcept
    {
        return Size>=2u;
    }
    std::size_t FrameCount() const noexcept
    {
        return Size;
    }
    PhysicalReplayReject LastRejection() const noexcept
    {
        return LastReject;
    }
    double EarliestClockSeconds() const noexcept
    {
        return Size==0u ? 0.0:
            FrameAt(0u).ActiveClockSeconds;
    }
    double LatestClockSeconds() const noexcept
    {
        return Size==0u ? 0.0:
            FrameAt(Size-1u).ActiveClockSeconds;
    }
    bool TryReadFrame(
        std::size_t chronologicalIndex,
        PhysicalReplayFrame& out) const noexcept
    {
        out={};
        if(chronologicalIndex>=Size)return false;
        out=FrameAt(chronologicalIndex);
        return true;
    }

    // Binary search is performed in LOGICAL chronological order over the
    // fixed ring. No allocations, identity guessing or extrapolation.
    PhysicalReplayPlaybackFrame Evaluate(
        double activeClockSeconds) const noexcept
    {
        PhysicalReplayPlaybackFrame out;
        if(!HasReplay() ||
           !std::isfinite(activeClockSeconds) ||
           activeClockSeconds<EarliestClockSeconds() ||
           activeClockSeconds>LatestClockSeconds())
            return out;
        std::size_t low=0u;
        std::size_t high=Size-1u;
        while(low<high)
        {
            const std::size_t mid=low+(high-low)/2u;
            if(FrameAt(mid).ActiveClockSeconds<
               activeClockSeconds)
                low=mid+1u;
            else
                high=mid;
        }
        const std::size_t upper=low;
        const std::size_t lower=upper==0u?0u:upper-1u;
        const auto& a=FrameAt(lower);
        const auto& b=FrameAt(upper);
        const double span=b.ActiveClockSeconds-
            a.ActiveClockSeconds;
        double alpha=0.0;
        if(lower!=upper)
        {
            if(!std::isfinite(span) ||
               span<=PhysicalReplayClockEpsilon ||
               span>PhysicalReplayMaximumFrameSeconds)
                return {};
            alpha=(activeClockSeconds-a.ActiveClockSeconds)/span;
            if(!std::isfinite(alpha) ||
               alpha<0.0 || alpha>1.0)
                return {};
        }
        for(std::size_t i=0u;i<PhysicalReplayCarCount;++i)
        {
            const auto& first=a.Cars[i];
            const auto& last=b.Cars[i];
            if(first.Actor==0 ||
               first.Actor!=last.Actor ||
               first.Actor!=ActorRoster[i] ||
               first.Session!=last.Session ||
               first.SessionRevision!=last.SessionRevision ||
               first.SourceFrame!=a.SourceFrame ||
               last.SourceFrame!=b.SourceFrame)
                return {};
            PhysicalReplayPlaybackCar car;
            car.Actor=first.Actor;
            car.Xcm=first.WorldXcm+
                alpha*(last.WorldXcm-first.WorldXcm);
            car.Ycm=first.WorldYcm+
                alpha*(last.WorldYcm-first.WorldYcm);
            car.Zcm=first.WorldZcm+
                alpha*(last.WorldZcm-first.WorldZcm);
            const double turn=std::remainder(
                last.HeadingRadians-first.HeadingRadians,
                2.0*3.1415926535897932384626433832795);
            car.HeadingRadians=std::remainder(
                first.HeadingRadians+alpha*turn,
                2.0*3.1415926535897932384626433832795);
            car.ObservedSpeedCmps=(1.0-alpha)*
                PhysicalReplaySpeed(first)+alpha*
                PhysicalReplaySpeed(last);
            car.SteeringNormalized=first.SteeringNormalized+
                alpha*(last.SteeringNormalized-
                       first.SteeringNormalized);
            car.ThrottleNormalized=first.ThrottleNormalized+
                alpha*(last.ThrottleNormalized-
                       first.ThrottleNormalized);
            car.BrakeNormalized=first.BrakeNormalized+
                alpha*(last.BrakeNormalized-
                       first.BrakeNormalized);
            if(!std::isfinite(car.Xcm) ||
               !std::isfinite(car.Ycm) ||
               !std::isfinite(car.Zcm) ||
               !std::isfinite(car.HeadingRadians) ||
               !std::isfinite(car.ObservedSpeedCmps))
                return {};
            car.ProvenanceMatched=true;
            out.Cars[i]=car;
        }
        out.Valid=true;
        out.Session=Session;
        out.SessionRevision=Revision;
        out.LowerSourceFrame=a.SourceFrame;
        out.UpperSourceFrame=b.SourceFrame;
        out.ActiveClockSeconds=activeClockSeconds;
        out.InterpolationFraction=alpha;
        return out;
    }

private:
    const PhysicalReplayFrame& FrameAt(
        std::size_t index) const noexcept
    {
        const std::size_t oldest=
            (Next+PhysicalReplayFrameCapacity-Size)%
            PhysicalReplayFrameCapacity;
        return Frames[(oldest+index)%PhysicalReplayFrameCapacity];
    }
    std::array<PhysicalReplayFrame,PhysicalReplayFrameCapacity>
        Frames{};
    std::array<std::uint64_t,PhysicalReplayCarCount> ActorRoster{};
    std::size_t Size=0u;
    std::size_t Next=0u;
    std::uint64_t Session=0;
    std::uint64_t Revision=0;
    PhysicalReplayReject LastReject=PhysicalReplayReject::None;
};
} // namespace latebrake
