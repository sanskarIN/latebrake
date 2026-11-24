#pragma once

#include "Track/WorldScaleVariantReferenceTiming.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Immutable-by-interface cache for repeated reference queries. The cache
// owns a copy of the validated source nodes, not a mutable caller pointer.
// It does not read an engine clock or create real race/ghost authority.
struct VariantReferenceClockSample final
{
    bool Valid=false;
    double StationCm=0.0;
    double Seconds=0.0;
    double SpeedCmps=0.0;
    double AccelerationCmps2=0.0;
    unsigned Segment=0u;
    CircuitPose Pose{};
    bool AdvisoryOnly=true;
};
struct VariantReferenceClockWindow final
{
    bool Valid=false;
    VariantReferenceClockSample Entry{};
    VariantReferenceClockSample Exit{};
    double DistanceCm=0.0;
    double DurationSeconds=0.0;
    double MeanSpeedCmps=0.0;
    bool CrossesLapSeam=false;
};

class VariantReferenceClock final
{
public:
    static constexpr double MaximumAbsoluteStationCm=1e9;
    static constexpr double MaximumAbsoluteTimeSeconds=1e6;

    // Strong replacement guarantee: invalid input does not erase a
    // previously built cache or leave a half-constructed usable object.
    bool Build(const WorldScaleVariantPacePlan& plan) noexcept
    {
        if(!ValidWorldScaleVariantPaceMetadata(plan))return false;
        std::array<double,WorldScalePaceStations+1u> times{};
        std::array<double,WorldScalePaceStations> speeds{};
        for(unsigned i=0;i<WorldScalePaceStations;++i)
        {
            if(!ValidWorldScaleVariantPaceEdge(plan,i))return false;
            const auto pose=WorldScaleVariantCenterlineAt(
                plan.Track,i*plan.StepCm);
            if(!pose.Valid)return false;
            const auto& pos=plan.Nodes[i].Position;
            if(std::hypot(pos.X-pose.Pose.Position.X,
                          pos.Y-pose.Pose.Position.Y)>1e-5)return false;
            speeds[i]=plan.Nodes[i].TargetCmps;
            const unsigned next=(i+1u)%WorldScalePaceStations;
            const double seconds=VariantReferenceSegmentSeconds(
                plan.StepCm,speeds[i],plan.Nodes[next].TargetCmps);
            if(!std::isfinite(seconds)||seconds<=0.0)return false;
            times[i+1u]=times[i]+seconds;
            if(!std::isfinite(times[i+1u])||times[i+1u]<=times[i])
                return false;
        }
        if(std::abs(times.back()-plan.EstimatedLapSeconds)>
           std::max(1e-8,plan.EstimatedLapSeconds*1e-10))return false;
        // Name is not used for physics, but avoid retaining a caller's
        // potentially temporary C string in the cached recipe.
        auto track=plan.Track;
        track.Name=WorldScaleVariantAt(static_cast<unsigned>(track.Id)).Name;
        track_=track;
        step_=plan.StepCm;
        times_=times;
        speeds_=speeds;
        ready_=true;
        return true;
    }

    void Reset() noexcept
    {
        *this=VariantReferenceClock{};
    }

    bool Ready() const noexcept { return ready_; }
    double LapSeconds() const noexcept { return ready_?times_.back():0.0; }
    double LapDistanceCm() const noexcept { return ready_?track_.LapLengthCm:0.0; }
    WorldScaleVariantId TrackId() const noexcept { return track_.Id; }

    // Signed unwrapped station and time are preserved; a negative station
    // is a point before the current lap origin, not an official lap count.
    VariantReferenceClockSample AtStation(double station) const noexcept
    {
        if(!ready_||!std::isfinite(station)||
           std::abs(station)>MaximumAbsoluteStationCm)return {};
        const double lap=std::floor(station/track_.LapLengthCm);
        double wrapped=station-lap*track_.LapLengthCm;
        double adjustedLap=lap;
        if(wrapped<0.0)
        {
            wrapped+=track_.LapLengthCm;
            adjustedLap-=1.0;
        }
        if(wrapped>=track_.LapLengthCm)
        {
            wrapped-=track_.LapLengthCm;
            adjustedLap+=1.0;
        }
        if(!std::isfinite(wrapped)||wrapped<0.0||
           wrapped>=track_.LapLengthCm)return {};
        const double index=wrapped/step_;
        if(!std::isfinite(index)||index<0.0||
           index>WorldScalePaceStations)return {};
        const unsigned i=std::min(static_cast<unsigned>(index),
            WorldScalePaceStations-1u);
        const double offset=std::clamp(wrapped-i*step_,0.0,step_);
        const double a=Acceleration(i);
        const double v=std::sqrt(std::max(0.0,
            speeds_[i]*speeds_[i]+2.0*a*offset));
        const double local=VariantReferenceSegmentSeconds(offset,speeds_[i],v);
        if(local<0.0||!std::isfinite(local))return {};
        const double time=adjustedLap*times_.back()+times_[i]+local;
        return MakeSample(station,time,v,a,i,wrapped);
    }

    // O(log N) inversion of monotonically increasing prefix times.
    // Constant acceleration in each segment gives distance =
    // t*(v0 + 0.5*a*t); no division by a near-zero acceleration.
    VariantReferenceClockSample AtTime(double seconds) const noexcept
    {
        if(!ready_||!std::isfinite(seconds)||
           std::abs(seconds)>MaximumAbsoluteTimeSeconds)return {};
        double lap=std::floor(seconds/times_.back());
        double wrapped=seconds-lap*times_.back();
        if(wrapped<0.0)
        {
            wrapped+=times_.back();
            lap-=1.0;
        }
        if(wrapped>=times_.back())
        {
            wrapped-=times_.back();
            lap+=1.0;
        }
        if(!std::isfinite(wrapped)||wrapped<0.0||wrapped>=times_.back())
            return {};
        const auto upper=std::upper_bound(times_.begin(),times_.end(),wrapped);
        if(upper==times_.begin()||upper==times_.end())return {};
        const unsigned i=static_cast<unsigned>(upper-times_.begin()-1);
        const double local=wrapped-times_[i];
        const double a=Acceleration(i);
        const double v=speeds_[i]+a*local;
        const double offset=std::clamp(
            local*(speeds_[i]+0.5*a*local),0.0,step_);
        const double wrappedStation=i*step_+offset;
        const double station=lap*track_.LapLengthCm+wrappedStation;
        if(!std::isfinite(station)||std::abs(station)>MaximumAbsoluteStationCm)
            return {};
        return MakeSample(station,seconds,v,a,i,wrappedStation);
    }

    VariantReferenceClockWindow WindowByDistance(
        double start,double distance) const noexcept
    {
        if(!ready_||!std::isfinite(start)||!std::isfinite(distance)||
           distance<=0.0||distance>track_.LapLengthCm)return {};
        const auto a=AtStation(start);
        const auto b=AtStation(start+distance);
        if(!a.Valid||!b.Valid||b.StationCm<=a.StationCm)return {};
        const double elapsed=b.Seconds-a.Seconds;
        // Near cancellation must not produce bogus instantaneous speed.
        const double tolerance=64.0*std::numeric_limits<double>::epsilon()*
            std::max({1.0,std::abs(a.Seconds),std::abs(b.Seconds)});
        if(!std::isfinite(elapsed)||elapsed<=tolerance)return {};
        return MakeWindow(a,b,b.StationCm-a.StationCm,elapsed);
    }

    VariantReferenceClockWindow WindowByTime(
        double start,double duration) const noexcept
    {
        if(!ready_||!std::isfinite(start)||!std::isfinite(duration)||
           duration<=0.0||duration>times_.back())return {};
        const auto a=AtTime(start);
        const auto b=AtTime(start+duration);
        if(!a.Valid||!b.Valid||b.Seconds<=a.Seconds)return {};
        const double distance=b.StationCm-a.StationCm;
        const double tolerance=64.0*std::numeric_limits<double>::epsilon()*
            std::max({1.0,std::abs(a.StationCm),std::abs(b.StationCm)});
        if(!std::isfinite(distance)||distance<=tolerance)return {};
        return MakeWindow(a,b,distance,b.Seconds-a.Seconds);
    }

private:
    double Acceleration(unsigned i) const noexcept
    {
        const double v0=speeds_[i];
        const double v1=speeds_[(i+1u)%WorldScalePaceStations];
        return (v1*v1-v0*v0)/(2.0*step_);
    }

    VariantReferenceClockSample MakeSample(
        double station,double seconds,double speed,double acceleration,
        unsigned segment,double wrapped) const noexcept
    {
        if(!std::isfinite(seconds)||std::abs(seconds)>MaximumAbsoluteTimeSeconds||
           !std::isfinite(speed)||speed<WorldScalePaceMinSpeedCmps-1e-6||
           speed>WorldScalePaceMaxSpeedCmps+1e-6||
           !std::isfinite(acceleration)||
           acceleration>WorldScalePaceDriveAccelCmps2+1e-4||
           acceleration< -WorldScalePaceBrakeAccelCmps2-1e-4)return {};
        const auto pose=WorldScaleVariantCenterlineAt(track_,wrapped);
        if(!pose.Valid)return {};
        return {true,station,seconds,speed,acceleration,segment,pose.Pose,true};
    }

    VariantReferenceClockWindow MakeWindow(
        VariantReferenceClockSample a,VariantReferenceClockSample b,
        double distance,double duration) const noexcept
    {
        if(!std::isfinite(distance)||distance<=0.0||
           !std::isfinite(duration)||duration<=0.0)return {};
        const double mean=distance/duration;
        // Large signed origins may lose a little precision; reject an
        // implausible quotient rather than silently clamping it.
        if(!std::isfinite(mean)||mean<WorldScalePaceMinSpeedCmps-1e-3||
           mean>WorldScalePaceMaxSpeedCmps+1e-3)return {};
        return {true,a,b,distance,duration,mean,
            std::floor(a.StationCm/track_.LapLengthCm)!=
            std::floor(b.StationCm/track_.LapLengthCm)};
    }

    bool ready_=false;
    WorldScaleVariantRecipe track_{};
    double step_=0.0;
    std::array<double,WorldScalePaceStations+1u> times_{};
    std::array<double,WorldScalePaceStations> speeds_{};
};
} // namespace latebrake
