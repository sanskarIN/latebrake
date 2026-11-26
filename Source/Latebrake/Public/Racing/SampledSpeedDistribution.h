#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Fixed speed bands of accepted chassis samples, weighted by elapsed
// sample time rather than frame count. They are estimates from sampled
// planar speed, not measured odometer distance or official timing.
enum class SampledSpeedBand : std::size_t
{
    Below20, From20To60, From60To100, From100To160, From160To220,
    From220To280, From280To360, Count
};

inline constexpr std::size_t SampledSpeedBandCount =
    static_cast<std::size_t>(SampledSpeedBand::Count);
inline constexpr std::array<double, SampledSpeedBandCount+1>
    SampledSpeedEdgesKmH{0.0,20.0,60.0,100.0,160.0,220.0,280.0,360.0};

struct SampledSpeedBandRow final
{
    double Seconds = 0.0;
    double DistanceCm = 0.0;
    double BrakingSeconds = 0.0;
    int Frames = 0;
};

struct SampledSpeedDistribution final
{
    bool Valid = false;
    std::array<SampledSpeedBandRow,SampledSpeedBandCount> Rows{};
    double TotalSeconds = 0.0;
    double TotalDistanceCm = 0.0;
    double TotalBrakingSeconds = 0.0;
    int TotalFrames = 0;
    double AverageSpeedKmH = 0.0;
    // Time-weighted quantiles are intervals, not interpolated exact
    // speed readings. Empty bands are never selected as quantiles.
    SampledSpeedBand MedianBand = SampledSpeedBand::Below20;
    SampledSpeedBand P90Band = SampledSpeedBand::Below20;
};

inline bool ClassifySampledSpeed(double kmh, SampledSpeedBand& band) noexcept
{
    if (!std::isfinite(kmh) || kmh < 0.0 || kmh > 360.0)
        return false;
    for (std::size_t i=0;i<SampledSpeedBandCount;++i)
    {
        if (kmh < SampledSpeedEdgesKmH[i+1] ||
            i+1==SampledSpeedBandCount)
        {
            band=static_cast<SampledSpeedBand>(i);
            return true;
        }
    }
    return false;
}

class SampledSpeedAccumulator final
{
public:
    static constexpr double MaximumSeconds = 10000000.0;
    static constexpr double MaximumDistanceCm = 1000000000.0;

    void Reset() noexcept
    {
        Rows.fill({});
        Seconds=Distance=Braking=0.0;
        Frames=0;
        Complete=true;
    }

    // Only call for an already accepted DrivingSessionStats sample.
    // On saturation, withhold the distribution instead of independently
    // clamping bins and presenting a false partition of session totals.
    bool Observe(double seconds,double cmPerSecond,bool braking) noexcept
    {
        if (!Complete) return false;
        SampledSpeedBand band{};
        if (!std::isfinite(seconds) || seconds<=0.0 || seconds>0.25 ||
            !std::isfinite(cmPerSecond) || cmPerSecond<0.0 ||
            cmPerSecond>10000.0 ||
            !ClassifySampledSpeed(cmPerSecond*0.036,band) ||
            Frames==std::numeric_limits<int>::max())
        {
            Complete=false;
            return false;
        }
        const double distance=cmPerSecond*seconds;
        if (seconds>MaximumSeconds-Seconds ||
            distance>MaximumDistanceCm-Distance ||
            (braking && seconds>MaximumSeconds-Braking))
        {
            Complete=false;
            return false;
        }
        auto& row=Rows[static_cast<std::size_t>(band)];
        row.Seconds+=seconds;
        row.DistanceCm+=distance;
        if (braking) row.BrakingSeconds+=seconds;
        ++row.Frames;
        Seconds+=seconds;
        Distance+=distance;
        if (braking) Braking+=seconds;
        ++Frames;
        return true;
    }

    SampledSpeedDistribution Review(int acceptedFrames,double sampledSeconds,
        double distanceCm,double brakingSeconds) const noexcept
    {
        if (!Complete || Frames<2 || Frames!=acceptedFrames ||
            Seconds<0.25 || !Matches(Seconds,sampledSeconds) ||
            !Matches(Distance,distanceCm) || !Matches(Braking,brakingSeconds))
            return {};
        SampledSpeedDistribution result;
        int count=0;
        double time=0.0,distance=0.0,braking=0.0;
        for (std::size_t i=0;i<Rows.size();++i)
        {
            const auto& row=Rows[i];
            if (row.Frames<0 || row.Frames>Frames-count ||
                !std::isfinite(row.Seconds) || row.Seconds<0.0 ||
                !std::isfinite(row.DistanceCm) || row.DistanceCm<0.0 ||
                !std::isfinite(row.BrakingSeconds) || row.BrakingSeconds<0.0 ||
                row.BrakingSeconds>row.Seconds ||
                ((row.Frames==0)!=(row.Seconds==0.0)) ||
                (row.Frames==0 && row.DistanceCm!=0.0))
                return {};
            count+=row.Frames;
            time+=row.Seconds;
            distance+=row.DistanceCm;
            braking+=row.BrakingSeconds;
        }
        if (count!=Frames || !Matches(time,Seconds) ||
            !Matches(distance,Distance) || !Matches(braking,Braking))
            return {};
        const double average=Distance/Seconds*0.036;
        if (!std::isfinite(average) || average<0.0 || average>360.0+1e-9)
            return {};
        result.Rows=Rows;
        result.TotalSeconds=Seconds;
        result.TotalDistanceCm=Distance;
        result.TotalBrakingSeconds=Braking;
        result.TotalFrames=Frames;
        result.AverageSpeedKmH=std::min(360.0,average);
        result.MedianBand=QuantileBand(0.5);
        result.P90Band=QuantileBand(0.9);
        result.Valid=true;
        return result;
    }

private:
    static bool Matches(double measured,double expected) noexcept
    {
        // Floating-point accumulation order differs between total and
        // bins. A relative rounding tolerance is not extra time credit.
        return std::isfinite(expected) && expected>=0.0 &&
            std::abs(measured-expected)<=1e-8*std::max(1.0,measured);
    }

    SampledSpeedBand QuantileBand(double fraction) const noexcept
    {
        double cumulative=0.0;
        auto last=SampledSpeedBand::Below20;
        for (std::size_t i=0;i<Rows.size();++i)
        {
            const auto seconds=Rows[i].Seconds;
            if (seconds<=0.0) continue;
            last=static_cast<SampledSpeedBand>(i);
            cumulative+=seconds;
            if (cumulative>=fraction*Seconds)
                return last;
        }
        return last;
    }

    std::array<SampledSpeedBandRow,SampledSpeedBandCount> Rows{};
    double Seconds=0.0;
    double Distance=0.0;
    double Braking=0.0;
    int Frames=0;
    bool Complete=true;
};
} // namespace latebrake
