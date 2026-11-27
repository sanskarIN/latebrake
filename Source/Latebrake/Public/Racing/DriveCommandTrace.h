#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// A bounded observation of the game's normalized command pipeline.
// The trace is not a force sensor, actual wheel slip measurement,
// race clock, input authority, persistent record or network packet.
inline constexpr double DriveTraceWindowSeconds=4.0;
inline constexpr std::size_t DriveTraceBinCount=32;
inline constexpr std::size_t DriveTraceSampleCapacity=1024;
inline constexpr double DriveTraceMaximumDelta=0.10;
inline constexpr double DriveTraceBinSeconds=DriveTraceWindowSeconds/DriveTraceBinCount;

enum class DriveCommandPath : std::uint8_t
{
    NoGroundSupport,
    Coast,
    MotorCommand,
    OpposingThrottle,
    BrakeCommand
};

constexpr const char* DriveCommandPathName(DriveCommandPath path) noexcept
{
    switch(path)
    {
    case DriveCommandPath::NoGroundSupport:return "NO GROUND SUPPORT";
    case DriveCommandPath::Coast:return "COAST";
    case DriveCommandPath::MotorCommand:return "MOTOR COMMAND";
    case DriveCommandPath::OpposingThrottle:return "OPPOSING THROTTLE";
    case DriveCommandPath::BrakeCommand:return "BRAKE COMMAND";
    default:return "UNAVAILABLE";
    }
}

struct DriveCommandSample final
{
    std::uint64_t Frame=0;
    std::uint64_t Session=0;
    std::uint64_t Teleport=0;
    std::uint64_t Controller=0;
    double DeltaSeconds=0.0;
    double FilteredThrottle=0.0;
    double ModeratedThrottle=0.0;
    double FilteredSteering=0.0;
    double ModeratedSteering=0.0;
    bool ServiceBrake=false;
    bool Handbrake=false;
    bool GroundSupported=false;
    bool MotorBranch=false;
};

inline bool ValidDriveCommandAxis(double axis) noexcept
{
    return std::isfinite(axis) && axis>=-1.0 && axis<=1.0;
}

inline bool ValidDriveCommandSample(const DriveCommandSample& s) noexcept
{
    if(!s.Frame || !s.Controller ||
        !std::isfinite(s.DeltaSeconds) || s.DeltaSeconds<=0.0 ||
        s.DeltaSeconds>DriveTraceMaximumDelta ||
        !ValidDriveCommandAxis(s.FilteredThrottle) ||
        !ValidDriveCommandAxis(s.ModeratedThrottle) ||
        !ValidDriveCommandAxis(s.FilteredSteering) ||
        !ValidDriveCommandAxis(s.ModeratedSteering))
        return false;
    if(s.MotorBranch && (!s.GroundSupported || s.ServiceBrake || s.Handbrake))
        return false;
    return !s.Handbrake || std::abs(s.ModeratedThrottle)<=1e-6;
}

inline DriveCommandPath ClassifyDriveCommandPath(const DriveCommandSample& s) noexcept
{
    if(!s.GroundSupported)return DriveCommandPath::NoGroundSupport;
    if(s.ServiceBrake || s.Handbrake)return DriveCommandPath::BrakeCommand;
    if(s.MotorBranch)
        return std::abs(s.ModeratedThrottle)>=0.01?
            DriveCommandPath::MotorCommand:DriveCommandPath::Coast;
    return DriveCommandPath::OpposingThrottle;
}

struct DriveTraceBin final
{
    double ObservedSeconds=0.0;
    double FilteredThrottle=0.0;
    double ModeratedThrottle=0.0;
    double FilteredSteering=0.0;
    double ModeratedSteering=0.0;
    double ServiceBrakeFraction=0.0;
    double HandbrakeFraction=0.0;
    double GroundFraction=0.0;
};

struct DriveCommandReview final
{
    bool Valid=false;
    DriveCommandSample Latest{};
    DriveCommandPath Path=DriveCommandPath::NoGroundSupport;
    double ObservedSeconds=0.0;
    double ServiceBrakeSeconds=0.0;
    double HandbrakeSeconds=0.0;
    double GroundSeconds=0.0;
    double MotorSeconds=0.0;
    double OpposingThrottleSeconds=0.0;
    double CoastSeconds=0.0;
    double BrakeCommandSeconds=0.0;
    double UnsupportedSeconds=0.0;
    double MeanThrottleModeration=0.0;
    double MeanSteeringModeration=0.0;
    std::array<DriveTraceBin,DriveTraceBinCount> Bins{};
};

class DriveCommandTrace final
{
public:
    void Suspend() noexcept
    {
        Count=0;
        Next=0;
        PendingFrame=0;
    }

    // Every pawn tick opens a receipt. A skipped, invalid or aborted
    // tick is not silently joined to the previous accepted history.
    void BeginFrame(std::uint64_t frame) noexcept
    {
        if(!frame || PendingFrame || (Count &&
            (Newest().Frame==UINT64_MAX || frame!=Newest().Frame+1)))
            Suspend();
        PendingFrame=frame;
    }

    bool Observe(const DriveCommandSample& s) noexcept
    {
        if(!PendingFrame || PendingFrame!=s.Frame || !ValidDriveCommandSample(s))
        {
            Suspend();
            return false;
        }
        if(Count && (s.Session!=Newest().Session ||
            s.Teleport!=Newest().Teleport || s.Controller!=Newest().Controller))
        {
            Count=0;
            Next=0;
        }
        Samples[Next]=s;
        Next=(Next+1)%Samples.size();
        Count=std::min(Count+1,Samples.size());
        PendingFrame=0;
        return true;
    }

    DriveCommandReview Read(std::uint64_t frame,std::uint64_t session,
        std::uint64_t teleport,std::uint64_t controller,bool eligible) const noexcept
    {
        DriveCommandReview out;
        if(!eligible || PendingFrame || !Count || !controller)
            return out;
        const auto& latest=Newest();
        if(frame<latest.Frame || frame-latest.Frame>1 ||
            session!=latest.Session || teleport!=latest.Teleport ||
            controller!=latest.Controller)
            return out;
        out.Latest=latest;
        out.Path=ClassifyDriveCommandPath(latest);
        double age=0.0;
        // Walk backwards from the latest complete frame, clipping only at
        // the requested window boundary. Missing history stays empty;
        // there is no extrapolation to fill four seconds after a reset.
        for(std::size_t offset=0;offset<Count && age<DriveTraceWindowSeconds;++offset)
        {
            const auto index=(Next+Samples.size()-1-offset)%Samples.size();
            const auto& sample=Samples[index];
            const double end=std::min(DriveTraceWindowSeconds,age+sample.DeltaSeconds);
            const double duration=end-age;
            if(duration<=0.0)break;
            Accumulate(out,sample,duration);
            for(std::size_t bin=0;bin<DriveTraceBinCount;++bin)
            {
                const double startAge=(DriveTraceBinCount-1-bin)*DriveTraceBinSeconds;
                const double endAge=startAge+DriveTraceBinSeconds;
                const double overlap=std::min(end,endAge)-std::max(age,startAge);
                if(overlap>0.0)AccumulateBin(out.Bins[bin],sample,overlap);
            }
            age=end;
        }
        if(out.ObservedSeconds<=0.0)return {};
        for(auto& bin:out.Bins)
        {
            if(bin.ObservedSeconds<=0.0)continue;
            const double divisor=bin.ObservedSeconds;
            bin.FilteredThrottle/=divisor;
            bin.ModeratedThrottle/=divisor;
            bin.FilteredSteering/=divisor;
            bin.ModeratedSteering/=divisor;
            bin.ServiceBrakeFraction/=divisor;
            bin.HandbrakeFraction/=divisor;
            bin.GroundFraction/=divisor;
        }
        out.MeanThrottleModeration/=out.ObservedSeconds;
        out.MeanSteeringModeration/=out.ObservedSeconds;
        out.Valid=true;
        return out;
    }

private:
    static void Accumulate(DriveCommandReview& out,const DriveCommandSample& s,
        double seconds) noexcept
    {
        out.ObservedSeconds+=seconds;
        if(s.ServiceBrake)out.ServiceBrakeSeconds+=seconds;
        if(s.Handbrake)out.HandbrakeSeconds+=seconds;
        if(s.GroundSupported)out.GroundSeconds+=seconds;
        switch(ClassifyDriveCommandPath(s))
        {
        case DriveCommandPath::NoGroundSupport:out.UnsupportedSeconds+=seconds;break;
        case DriveCommandPath::Coast:out.CoastSeconds+=seconds;break;
        case DriveCommandPath::MotorCommand:out.MotorSeconds+=seconds;break;
        case DriveCommandPath::OpposingThrottle:out.OpposingThrottleSeconds+=seconds;break;
        case DriveCommandPath::BrakeCommand:out.BrakeCommandSeconds+=seconds;break;
        default:break;
        }
        out.MeanThrottleModeration+=
            std::abs(s.FilteredThrottle-s.ModeratedThrottle)*seconds;
        out.MeanSteeringModeration+=
            std::abs(s.FilteredSteering-s.ModeratedSteering)*seconds;
    }

    static void AccumulateBin(DriveTraceBin& bin,const DriveCommandSample& s,
        double seconds) noexcept
    {
        bin.ObservedSeconds+=seconds;
        bin.FilteredThrottle+=s.FilteredThrottle*seconds;
        bin.ModeratedThrottle+=s.ModeratedThrottle*seconds;
        bin.FilteredSteering+=s.FilteredSteering*seconds;
        bin.ModeratedSteering+=s.ModeratedSteering*seconds;
        if(s.ServiceBrake)bin.ServiceBrakeFraction+=seconds;
        if(s.Handbrake)bin.HandbrakeFraction+=seconds;
        if(s.GroundSupported)bin.GroundFraction+=seconds;
    }

    const DriveCommandSample& Newest() const noexcept
    {
        return Samples[(Next+Samples.size()-1)%Samples.size()];
    }

    std::array<DriveCommandSample,DriveTraceSampleCapacity> Samples{};
    std::size_t Count=0;
    std::size_t Next=0;
    std::uint64_t PendingFrame=0;
};
} // namespace latebrake
