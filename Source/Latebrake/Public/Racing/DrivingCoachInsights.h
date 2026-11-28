#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Human-readable coaching in LateBrake is exclusively a read-only observation
// of accepted chassis samples. In particular, it is not a tire model, race
// steward, lap timer, input assistant, physics filter, or rewards system.
inline constexpr std::size_t DrivingCoachSpeedBandCount=7;
inline constexpr std::size_t DrivingCoachEpisodeCapacity=32;
inline constexpr std::size_t DrivingCoachWindowCapacity=16;
inline constexpr double DrivingCoachMaximumActiveSeconds=10000000.0;
inline constexpr double DrivingCoachMaximumSpeedCmps=10000.0;
inline constexpr double DrivingCoachMaximumSampleSeconds=0.25;
inline constexpr double DrivingCoachWindowSeconds=5.0;
inline constexpr double DrivingCoachSlipThresholdCmps=500.0;
inline constexpr double DrivingCoachSlipMinimumSpeedCmps=900.0;
inline constexpr double DrivingCoachSlipMinimumFraction=0.35;

struct DrivingCoachSample final
{
    double DeltaSeconds=0.0;
    double PlanarSpeedCmps=0.0;
    double LateralSpeedCmps=0.0;
    bool Braking=false;
};

inline bool ValidDrivingCoachSample(DrivingCoachSample sample) noexcept
{
    return std::isfinite(sample.DeltaSeconds) &&
        sample.DeltaSeconds>0.0 &&
        sample.DeltaSeconds<=DrivingCoachMaximumSampleSeconds &&
        std::isfinite(sample.PlanarSpeedCmps) &&
        sample.PlanarSpeedCmps>=0.0 &&
        sample.PlanarSpeedCmps<=DrivingCoachMaximumSpeedCmps &&
        std::isfinite(sample.LateralSpeedCmps) &&
        std::abs(sample.LateralSpeedCmps)<=DrivingCoachMaximumSpeedCmps;
}

inline bool DrivingCoachHighSlip(DrivingCoachSample sample) noexcept
{
    return ValidDrivingCoachSample(sample) &&
        sample.PlanarSpeedCmps>=DrivingCoachSlipMinimumSpeedCmps &&
        std::abs(sample.LateralSpeedCmps)>=DrivingCoachSlipThresholdCmps &&
        std::abs(sample.LateralSpeedCmps)>=
            DrivingCoachSlipMinimumFraction*sample.PlanarSpeedCmps;
}

enum class DrivingCoachEvidence : std::uint8_t
{
    Unavailable,
    Limited,
    Useful,
    Broad
};

enum class DrivingCoachMessage : std::uint8_t
{
    CollectMoreSamples,
    HighLateralSlipObserved,
    FrequentBrakeReapplication,
    HeavyBrakingShare,
    StableObservedMotion,
    LowMotionCoverage
};

inline const char* DrivingCoachMessageLabel(DrivingCoachMessage message) noexcept
{
    switch(message)
    {
    case DrivingCoachMessage::CollectMoreSamples:
        return "MORE VERIFIED DRIVING SAMPLES NEEDED";
    case DrivingCoachMessage::HighLateralSlipObserved:
        return "HIGH LATERAL MOTION OBSERVED: REVIEW CORNER ENTRIES";
    case DrivingCoachMessage::FrequentBrakeReapplication:
        return "MANY SEPARATE BRAKE INPUTS: REVIEW SMOOTHNESS";
    case DrivingCoachMessage::HeavyBrakingShare:
        return "BRAKING OCCUPIED A LARGE SHARE OF OBSERVED DRIVING";
    case DrivingCoachMessage::StableObservedMotion:
        return "SAMPLED MOTION WAS MOSTLY STABLE";
    case DrivingCoachMessage::LowMotionCoverage:
        return "MOST SAMPLE TIME WAS NOT ACTIVE DRIVING";
    default:
        return "DRIVING COACH UNAVAILABLE";
    }
}

struct DrivingCoachSpeedBand final
{
    // The upper boundary is exclusive except for the final band.
    double LowKmh=0.0;
    double HighKmh=0.0;
    double Seconds=0.0;
    double DistanceKm=0.0;
    double BrakingSeconds=0.0;
    double HighSlipSeconds=0.0;
    std::uint64_t Samples=0;
};

struct DrivingCoachEpisode final
{
    bool Valid=false;
    bool Complete=false;
    double StartedSeconds=0.0;
    double EndedSeconds=0.0;
    double DurationSeconds=0.0;
    double DistanceKm=0.0;
    double EntrySpeedKmh=0.0;
    double MinimumSpeedKmh=0.0;
    double ExitSpeedKmh=0.0;
    double PeakSlipFraction=0.0;
    std::uint64_t Samples=0;
};

struct DrivingCoachWindow final
{
    bool Valid=false;
    bool Complete=false;
    std::uint64_t Sequence=0;
    std::uint64_t ContinuityEpoch=0;
    double StartedSeconds=0.0;
    double EndedSeconds=0.0;
    double ObservedSeconds=0.0;
    double DistanceKm=0.0;
    double MeanSpeedKmh=0.0;
    double BrakingFraction=0.0;
    double SlipFraction=0.0;
    std::uint64_t Samples=0;
};

struct DrivingCoachReadout final
{
    bool Valid=false;
    DrivingCoachEvidence Evidence=DrivingCoachEvidence::Unavailable;
    DrivingCoachMessage Message=DrivingCoachMessage::CollectMoreSamples;
    double ObservedSeconds=0.0;
    double DistanceKm=0.0;
    double MeanSpeedKmh=0.0;
    double PeakSpeedKmh=0.0;
    double BrakeSeconds=0.0;
    double HighSlipSeconds=0.0;
    double MovingSeconds=0.0;
    double StationarySeconds=0.0;
    double IntermediateSpeedSeconds=0.0;
    double MovingFraction=0.0;
    double PeakObservedSpeedChangeCmps2=0.0;
    double MeanObservedAbsoluteSpeedChangeCmps2=0.0;
    std::uint64_t ObservedStrongSpeedChanges=0;
    double BrakeFraction=0.0;
    double HighSlipFraction=0.0;
    double PeakSlipFraction=0.0;
    double MeanAbsoluteLateralFraction=0.0;
    std::uint64_t AcceptedSamples=0;
    std::uint64_t RefusedSamples=0;
    std::uint64_t Suspensions=0;
    std::uint64_t AcceptedBrakeEpisodes=0;
    std::uint64_t CompletedBrakeEpisodes=0;
    std::uint64_t AcceptedSlipEpisodes=0;
    std::uint64_t CompletedSlipEpisodes=0;
    std::array<DrivingCoachSpeedBand,DrivingCoachSpeedBandCount> SpeedBands{};
    std::array<DrivingCoachEpisode,DrivingCoachEpisodeCapacity> Brakes{};
    std::array<DrivingCoachEpisode,DrivingCoachEpisodeCapacity> Slips{};
    std::size_t RetainedBrakeEpisodes=0;
    std::size_t RetainedSlipEpisodes=0;
    std::array<DrivingCoachWindow,DrivingCoachWindowCapacity> Windows{};
    std::size_t RetainedWindows=0;
    bool NoGameplayAuthority=true;
    bool NoPersistentData=true;
    bool NoOfficialTiming=true;
};

// Ring backing storage is bounded and allocation-free. The most recent record
// has reverse offset zero; old records expire instead of inventing history.
template<class Entry,std::size_t Capacity>
class DrivingCoachRing final
{
public:
    static_assert(Capacity>0);

    void Reset() noexcept
    {
        Data={};
        Cursor=0;
        Size=0;
    }

    void Push(const Entry& item) noexcept
    {
        Data[Cursor]=item;
        Cursor=(Cursor+1)%Capacity;
        if(Size<Capacity) ++Size;
    }

    bool AtNewest(std::size_t offset,Entry& out) const noexcept
    {
        if(offset>=Size) return false;
        out=Data[(Cursor+Capacity-1-offset)%Capacity];
        return true;
    }

    std::size_t Count() const noexcept {return Size;}

private:
    std::array<Entry,Capacity> Data{};
    std::size_t Cursor=0;
    std::size_t Size=0;
};

class DrivingCoachAccumulator final
{
public:
    DrivingCoachAccumulator() noexcept {Reset();}

    void Reset() noexcept
    {
        ObservedSeconds=0.0;
        DistanceKm=0.0;
        BrakeSeconds=0.0;
        SlipSeconds=0.0;
        MovingSeconds=0.0;
        StationarySeconds=0.0;
        IntermediateSpeedSeconds=0.0;
        TotalObservedSpeedVariation=0.0;
        ObservedSpeedPairSeconds=0.0;
        PeakObservedSpeedChangeCmps2=0.0;
        ObservedStrongSpeedChanges=0;
        HavePreviousSpeed=false;
        PreviousSpeedCmps=0.0;
        WeightedSlipSeconds=0.0;
        PeakSpeedKmh=0.0;
        PeakSlipFraction=0.0;
        Accepted=0;
        Refused=0;
        Suspensions=0;
        TotalBrakeEpisodes=0;
        CompletedBrakeEpisodes=0;
        TotalSlipEpisodes=0;
        CompletedSlipEpisodes=0;
        BrakeActive=false;
        SlipActive=false;
        CurrentBrake={};
        CurrentSlip={};
        Bands={};
        constexpr std::array<double,DrivingCoachSpeedBandCount+1> edges{
            0.0,30.0,60.0,90.0,120.0,180.0,240.0,360.0};
        for(std::size_t i=0;i<Bands.size();++i)
        {
            Bands[i].LowKmh=edges[i];
            Bands[i].HighKmh=edges[i+1];
        }
        BrakeArchive.Reset();
        SlipArchive.Reset();
        WindowArchive.Reset();
        CurrentWindow={};
        NextWindowSequence=1;
        ContinuityEpoch=1;
    }

    void Suspend() noexcept
    {
        FinishBrake();
        FinishSlip();
        FlushWindow(false);
        HavePreviousSpeed=false;
        PreviousSpeedCmps=0.0;
        Increase(Suspensions);
        Increase(ContinuityEpoch);
    }

    bool Observe(DrivingCoachSample sample) noexcept
    {
        if(!ValidDrivingCoachSample(sample))
        {
            Increase(Refused);
            Suspend();
            return false;
        }
        const bool highSlip=DrivingCoachHighSlip(sample);
        const double dt=sample.DeltaSeconds;
        const double speedKmh=sample.PlanarSpeedCmps*0.036;
        const double traveled=sample.PlanarSpeedCmps*dt/100000.0;
        const double lateralFraction=sample.PlanarSpeedCmps>1.0 ?
            std::abs(sample.LateralSpeedCmps)/sample.PlanarSpeedCmps:0.0;
        const double previousClock=ObservedSeconds;
        ObservedSeconds=std::min(DrivingCoachMaximumActiveSeconds,
            ObservedSeconds+dt);
        DistanceKm=std::min(10000.0,DistanceKm+traveled);
        PeakSpeedKmh=std::max(PeakSpeedKmh,speedKmh);
        PeakSlipFraction=std::max(PeakSlipFraction,lateralFraction);
        WeightedSlipSeconds=std::min(DrivingCoachMaximumActiveSeconds,
            WeightedSlipSeconds+std::min(10.0,lateralFraction)*dt);
        Increase(Accepted);
        if(sample.Braking)
            BrakeSeconds=std::min(DrivingCoachMaximumActiveSeconds,
                BrakeSeconds+dt);
        if(highSlip)
            SlipSeconds=std::min(DrivingCoachMaximumActiveSeconds,
                SlipSeconds+dt);
        if(sample.PlanarSpeedCmps>=300.0)
            MovingSeconds+=dt;
        else if(sample.PlanarSpeedCmps<100.0)
            StationarySeconds+=dt;
        else
            IntermediateSpeedSeconds+=dt;
        if(HavePreviousSpeed)
        {
            const double observedRate=std::abs(sample.PlanarSpeedCmps-
                PreviousSpeedCmps)/dt;
            PeakObservedSpeedChangeCmps2=
                std::max(PeakObservedSpeedChangeCmps2,observedRate);
            TotalObservedSpeedVariation+=observedRate*dt;
            ObservedSpeedPairSeconds+=dt;
            if(observedRate>=2500.0)
                Increase(ObservedStrongSpeedChanges);
        }
        PreviousSpeedCmps=sample.PlanarSpeedCmps;
        HavePreviousSpeed=true;
        auto& band=Bands[BandFor(speedKmh)];
        band.Seconds=std::min(DrivingCoachMaximumActiveSeconds,band.Seconds+dt);
        band.DistanceKm=std::min(10000.0,band.DistanceKm+traveled);
        if(sample.Braking)band.BrakingSeconds+=dt;
        if(highSlip)band.HighSlipSeconds+=dt;
        Increase(band.Samples);
        UpdateEpisode(CurrentBrake,BrakeActive,TotalBrakeEpisodes,
            sample.Braking,previousClock,dt,speedKmh,traveled,lateralFraction,
            BrakeArchive,CompletedBrakeEpisodes);
        UpdateEpisode(CurrentSlip,SlipActive,TotalSlipEpisodes,
            highSlip,previousClock,dt,speedKmh,traveled,lateralFraction,
            SlipArchive,CompletedSlipEpisodes);
        AccumulateWindow(sample,highSlip,previousClock,traveled);
        return true;
    }

    DrivingCoachReadout Review() const noexcept
    {
        DrivingCoachReadout out;
        out.Valid=Accepted>0 && ObservedSeconds>0.0;
        if(!out.Valid) return out;
        out.Evidence=ObservedSeconds<5.0 || Accepted<20 ?
            DrivingCoachEvidence::Limited :
            ObservedSeconds>=60.0 && Accepted>=400 ?
                DrivingCoachEvidence::Broad : DrivingCoachEvidence::Useful;
        out.ObservedSeconds=ObservedSeconds;
        out.DistanceKm=DistanceKm;
        out.PeakSpeedKmh=PeakSpeedKmh;
        out.MeanSpeedKmh=DistanceKm*3600.0/ObservedSeconds;
        out.BrakeSeconds=BrakeSeconds;
        out.HighSlipSeconds=SlipSeconds;
        out.MovingSeconds=MovingSeconds;
        out.StationarySeconds=StationarySeconds;
        out.IntermediateSpeedSeconds=IntermediateSpeedSeconds;
        out.MovingFraction=std::clamp(MovingSeconds/ObservedSeconds,0.0,1.0);
        out.PeakObservedSpeedChangeCmps2=PeakObservedSpeedChangeCmps2;
        out.MeanObservedAbsoluteSpeedChangeCmps2=
            ObservedSpeedPairSeconds>0.0 ?
            TotalObservedSpeedVariation/ObservedSpeedPairSeconds:0.0;
        out.ObservedStrongSpeedChanges=ObservedStrongSpeedChanges;
        out.BrakeFraction=std::clamp(BrakeSeconds/ObservedSeconds,0.0,1.0);
        out.HighSlipFraction=std::clamp(SlipSeconds/ObservedSeconds,0.0,1.0);
        out.PeakSlipFraction=PeakSlipFraction;
        out.MeanAbsoluteLateralFraction=WeightedSlipSeconds/ObservedSeconds;
        out.AcceptedSamples=Accepted;
        out.RefusedSamples=Refused;
        out.Suspensions=Suspensions;
        out.AcceptedBrakeEpisodes=TotalBrakeEpisodes;
        out.CompletedBrakeEpisodes=CompletedBrakeEpisodes;
        out.AcceptedSlipEpisodes=TotalSlipEpisodes;
        out.CompletedSlipEpisodes=CompletedSlipEpisodes;
        out.SpeedBands=Bands;
        // An active event is the most recent observation. Put it first,
        // then include as many archived records as fit the fixed buffer.
        const std::size_t brakeStart=BrakeActive?1u:0u;
        if(BrakeActive)out.Brakes[0]=CurrentBrake;
        out.RetainedBrakeEpisodes=std::min(DrivingCoachEpisodeCapacity,
            BrakeArchive.Count()+brakeStart);
        for(std::size_t i=brakeStart;i<out.RetainedBrakeEpisodes;++i)
            BrakeArchive.AtNewest(i-brakeStart,out.Brakes[i]);
        const std::size_t slipStart=SlipActive?1u:0u;
        if(SlipActive)out.Slips[0]=CurrentSlip;
        out.RetainedSlipEpisodes=std::min(DrivingCoachEpisodeCapacity,
            SlipArchive.Count()+slipStart);
        for(std::size_t i=slipStart;i<out.RetainedSlipEpisodes;++i)
            SlipArchive.AtNewest(i-slipStart,out.Slips[i]);
        const std::size_t windowStart=CurrentWindow.Valid?1u:0u;
        if(CurrentWindow.Valid)out.Windows[0]=BuildWindow(CurrentWindow,false);
        out.RetainedWindows=std::min(DrivingCoachWindowCapacity,
            WindowArchive.Count()+windowStart);
        for(std::size_t i=windowStart;i<out.RetainedWindows;++i)
            WindowArchive.AtNewest(i-windowStart,out.Windows[i]);
        if(out.Evidence==DrivingCoachEvidence::Limited)
            out.Message=DrivingCoachMessage::CollectMoreSamples;
        else if(out.MovingFraction<0.15)
            out.Message=DrivingCoachMessage::LowMotionCoverage;
        else if(out.HighSlipFraction>=0.12 && out.HighSlipSeconds>=1.0)
            out.Message=DrivingCoachMessage::HighLateralSlipObserved;
        else if(out.AcceptedBrakeEpisodes>=12 &&
                static_cast<double>(out.AcceptedBrakeEpisodes)/ObservedSeconds>=0.20)
            out.Message=DrivingCoachMessage::FrequentBrakeReapplication;
        else if(out.BrakeFraction>=0.40 && out.BrakeSeconds>=3.0)
            out.Message=DrivingCoachMessage::HeavyBrakingShare;
        else
            out.Message=DrivingCoachMessage::StableObservedMotion;
        return out;
    }

private:
    struct RawWindow final
    {
        bool Valid=false;
        std::uint64_t Sequence=0;
        std::uint64_t Epoch=0;
        double Start=0.0;
        double End=0.0;
        double Seconds=0.0;
        double DistanceKm=0.0;
        double BrakeSeconds=0.0;
        double SlipSeconds=0.0;
        std::uint64_t Samples=0;
    };

    static void Increase(std::uint64_t& value) noexcept
    {
        if(value<std::numeric_limits<std::uint64_t>::max()) ++value;
    }

    static std::size_t BandFor(double kmh) noexcept
    {
        if(kmh<30.0) return 0;
        if(kmh<60.0) return 1;
        if(kmh<90.0) return 2;
        if(kmh<120.0) return 3;
        if(kmh<180.0) return 4;
        if(kmh<240.0) return 5;
        return 6;
    }

    template<std::size_t Capacity>
    static void UpdateEpisode(DrivingCoachEpisode& active,bool& running,
        std::uint64_t& count,bool condition,double now,double dt,double speed,
        double traveled,double lateralFraction,
        DrivingCoachRing<DrivingCoachEpisode,Capacity>& archive,
        std::uint64_t& completed) noexcept
    {
        if(condition)
        {
            if(!running)
            {
                running=true;
                active={};
                active.Valid=true;
                active.StartedSeconds=now;
                active.EntrySpeedKmh=speed;
                active.MinimumSpeedKmh=speed;
                Increase(count);
            }
            active.EndedSeconds=now+dt;
            active.DurationSeconds+=dt;
            active.DistanceKm+=traveled;
            active.ExitSpeedKmh=speed;
            active.MinimumSpeedKmh=std::min(active.MinimumSpeedKmh,speed);
            active.PeakSlipFraction=std::max(active.PeakSlipFraction,lateralFraction);
            Increase(active.Samples);
        }
        else if(running)
        {
            active.Complete=true;
            archive.Push(active);
            Increase(completed);
            running=false;
            active={};
        }
    }

    static DrivingCoachWindow BuildWindow(RawWindow raw,bool complete) noexcept
    {
        DrivingCoachWindow out;
        if(!raw.Valid || raw.Seconds<=0.0)return out;
        out.Valid=true;
        out.Complete=complete;
        out.Sequence=raw.Sequence;
        out.ContinuityEpoch=raw.Epoch;
        out.StartedSeconds=raw.Start;
        out.EndedSeconds=raw.End;
        out.ObservedSeconds=raw.Seconds;
        out.DistanceKm=raw.DistanceKm;
        out.MeanSpeedKmh=raw.DistanceKm*3600.0/raw.Seconds;
        out.BrakingFraction=std::clamp(raw.BrakeSeconds/raw.Seconds,0.0,1.0);
        out.SlipFraction=std::clamp(raw.SlipSeconds/raw.Seconds,0.0,1.0);
        out.Samples=raw.Samples;
        return out;
    }

    void FinishBrake() noexcept
    {
        if(BrakeActive)
        {
            CurrentBrake.Complete=true;
            BrakeArchive.Push(CurrentBrake);
            Increase(CompletedBrakeEpisodes);
            BrakeActive=false;
            CurrentBrake={};
        }
    }

    void FinishSlip() noexcept
    {
        if(SlipActive)
        {
            CurrentSlip.Complete=true;
            SlipArchive.Push(CurrentSlip);
            Increase(CompletedSlipEpisodes);
            SlipActive=false;
            CurrentSlip={};
        }
    }

    void FlushWindow(bool complete) noexcept
    {
        if(CurrentWindow.Valid && CurrentWindow.Seconds>0.0)
            WindowArchive.Push(BuildWindow(CurrentWindow,complete));
        CurrentWindow={};
    }

    void AccumulateWindow(DrivingCoachSample sample,bool highSlip,
        double previousClock,double traveled) noexcept
    {
        // Source samples are never split or resampled across artificial
        // boundaries. A new bucket begins after the last accepted sample.
        if(CurrentWindow.Valid && CurrentWindow.Seconds+
            sample.DeltaSeconds>DrivingCoachWindowSeconds+1.0e-9)
            FlushWindow(CurrentWindow.Seconds>=DrivingCoachWindowSeconds-0.25);
        if(!CurrentWindow.Valid)
        {
            CurrentWindow.Valid=true;
            CurrentWindow.Start=previousClock;
            CurrentWindow.Sequence=NextWindowSequence;
            CurrentWindow.Epoch=ContinuityEpoch;
            Increase(NextWindowSequence);
        }
        CurrentWindow.End=previousClock+sample.DeltaSeconds;
        CurrentWindow.Seconds+=sample.DeltaSeconds;
        CurrentWindow.DistanceKm+=traveled;
        if(sample.Braking)CurrentWindow.BrakeSeconds+=sample.DeltaSeconds;
        if(highSlip)CurrentWindow.SlipSeconds+=sample.DeltaSeconds;
        Increase(CurrentWindow.Samples);
        if(CurrentWindow.Seconds>=DrivingCoachWindowSeconds-1.0e-9)
            FlushWindow(true);
    }

    double ObservedSeconds=0.0;
    double DistanceKm=0.0;
    double BrakeSeconds=0.0;
    double SlipSeconds=0.0;
    double MovingSeconds=0.0;
    double StationarySeconds=0.0;
    double IntermediateSpeedSeconds=0.0;
    double TotalObservedSpeedVariation=0.0;
    double ObservedSpeedPairSeconds=0.0;
    double PeakObservedSpeedChangeCmps2=0.0;
    std::uint64_t ObservedStrongSpeedChanges=0;
    bool HavePreviousSpeed=false;
    double PreviousSpeedCmps=0.0;
    double WeightedSlipSeconds=0.0;
    double PeakSpeedKmh=0.0;
    double PeakSlipFraction=0.0;
    std::uint64_t Accepted=0;
    std::uint64_t Refused=0;
    std::uint64_t Suspensions=0;
    std::uint64_t TotalBrakeEpisodes=0;
    std::uint64_t CompletedBrakeEpisodes=0;
    std::uint64_t TotalSlipEpisodes=0;
    std::uint64_t CompletedSlipEpisodes=0;
    bool BrakeActive=false;
    bool SlipActive=false;
    DrivingCoachEpisode CurrentBrake{};
    DrivingCoachEpisode CurrentSlip{};
    std::array<DrivingCoachSpeedBand,DrivingCoachSpeedBandCount> Bands{};
    DrivingCoachRing<DrivingCoachEpisode,DrivingCoachEpisodeCapacity> BrakeArchive;
    DrivingCoachRing<DrivingCoachEpisode,DrivingCoachEpisodeCapacity> SlipArchive;
    DrivingCoachRing<DrivingCoachWindow,DrivingCoachWindowCapacity> WindowArchive;
    RawWindow CurrentWindow{};
    std::uint64_t NextWindowSequence=1;
    std::uint64_t ContinuityEpoch=1;
};
} // namespace latebrake
