#pragma once

#include "AI/FleetPassingLane.h"
#include "AI/FleetQueueDeferral.h"
#include "AI/RivalFleetRiskSupervisor.h"
#include "AI/RivalPilot.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Observed AI command requests from the actual control pipeline. These
// are not measured tire forces, simulated counterfactual laps, official
// race classification, or proof that an intervention avoided a collision.
enum class FleetDecisionStage : std::uint8_t
{
    Route,
    Corner,
    Difficulty,
    Grip,
    SurfaceDamage,
    EngineHeat,
    HumanFollowing,
    Passing,
    ImmediateTraffic,
    PredictiveTraffic,
    Yaw,
    Spin,
    Count
};

inline constexpr std::size_t FleetDecisionStageCount =
    static_cast<std::size_t>(FleetDecisionStage::Count);
inline constexpr double FleetDecisionCommandEpsilon=1e-6;
inline constexpr double FleetDecisionClockTolerance=1e-5;
inline constexpr double FleetDecisionMaxSeconds=604800.0;
inline constexpr double FleetDecisionMaxFrameSeconds=0.10;
inline constexpr double FleetDecisionStationaryCmS=80.0;
inline constexpr double FleetDecisionNoDriveThreshold=0.01;
using FleetDecisionIdentity=std::array<std::uint64_t,StartGridCapacity>;

inline std::size_t FleetDecisionIndex(FleetDecisionStage stage) noexcept
{
    return static_cast<std::size_t>(stage);
}

inline const char* FleetDecisionStageLabel(FleetDecisionStage stage) noexcept
{
    switch(stage)
    {
        case FleetDecisionStage::Route: return "ROUTE";
        case FleetDecisionStage::Corner: return "CORNER";
        case FleetDecisionStage::Difficulty: return "DIFFICULTY";
        case FleetDecisionStage::Grip: return "GRIP";
        case FleetDecisionStage::SurfaceDamage: return "SURFACE/DAMAGE";
        case FleetDecisionStage::EngineHeat: return "ENGINE HEAT";
        case FleetDecisionStage::HumanFollowing: return "FOLLOWING";
        case FleetDecisionStage::Passing: return "PASSING";
        case FleetDecisionStage::ImmediateTraffic: return "LOCAL TRAFFIC";
        case FleetDecisionStage::PredictiveTraffic: return "TRAFFIC FORECAST";
        case FleetDecisionStage::Yaw: return "YAW";
        case FleetDecisionStage::Spin: return "SPIN";
        default: return "NONE";
    }
}

struct FleetDecisionAdmission final
{
    std::uint64_t Attempt=0;
    std::uint64_t Frame=0;
    std::uint64_t EngineFrame=0;
    std::uint64_t Epoch=0;
    double ClockSeconds=0.0;
    double AdvancedSeconds=0.0;
};

// A command trace is valid only when every pipeline stage was explicitly
// recorded once in order. A default PilotCommand is a legitimate command,
// but is not evidence that its stage actually executed.
class FleetDecisionTrace final
{
public:
    using CommandsType=std::array<PilotCommand,FleetDecisionStageCount>;

    void Record(FleetDecisionStage stage,PilotCommand command) noexcept
    {
        if(Invalid) return;
        const auto index=FleetDecisionIndex(stage);
        if(index>=Values.size() || index!=Next)
        {
            Invalid=true;
            return;
        }
        Values[Next++]=command;
    }

    bool Complete() const noexcept {return !Invalid && Next==Values.size();}
    const CommandsType& Commands() const noexcept {return Values;}

private:
    CommandsType Values{};
    std::size_t Next=0;
    bool Invalid=false;
};

struct FleetDecisionSample final
{
    std::uint64_t Attempt=0;
    std::uint64_t Frame=0;
    std::uint64_t EngineFrame=0;
    std::uint64_t Epoch=0;
    std::uint64_t Actor=0;
    std::uint64_t TeleportRevision=0;
    int Slot=-1;
    int GateOrdinal=-1;
    double SpeedCmS=0.0;
    bool RoadQualified=false;
    bool TrafficObserved=false;
    bool TrafficQualified=false;
    bool BrakeHold=false;
    bool RoadSpeedExceeded=false;
    int OffroadBlockers=0;
    int LimitingSlot=-1;
    FleetRiskKind Risk=FleetRiskKind::Unqualified;
    FleetQueueStatus Queue=FleetQueueStatus::Inactive;
    FleetPassPhase Passing=FleetPassPhase::Idle;
    FleetDecisionTrace Trace{};

    void Set(FleetDecisionStage stage,PilotCommand command) noexcept
    {
        Trace.Record(stage,command);
    }
};

struct FleetDecisionStageEffect final
{
    bool ThrottleReduced=false;
    bool ThrottleRaised=false;
    bool BrakeAsserted=false;
    bool BrakeReleased=false;
    bool SteeringChanged=false;
    double Reduction=0.0;
};

struct FleetDecisionEndpoint final
{
    bool Valid=false;
    int Slot=-1;
    int GateOrdinal=-1;
    double SpeedCmS=0.0;
    PilotCommand Final{};
    FleetDecisionStage BrakeOrigin=FleetDecisionStage::Count;
    FleetDecisionStage LargestReduction=FleetDecisionStage::Count;
    double LargestThrottleReduction=0.0;
    bool RoadQualified=false;
    bool TrafficObserved=false;
    bool TrafficQualified=false;
    bool BrakeHold=false;
    bool RoadSpeedExceeded=false;
    bool StationaryNoDrive=false;
    int OffroadBlockers=0;
    int LimitingSlot=-1;
    FleetRiskKind Risk=FleetRiskKind::Unqualified;
    FleetQueueStatus Queue=FleetQueueStatus::Inactive;
    FleetPassPhase Passing=FleetPassPhase::Idle;
    std::array<FleetDecisionStageEffect,FleetDecisionStageCount> Effects{};
};

inline bool ValidFleetDecisionIdentity(const FleetDecisionIdentity& ids) noexcept
{
    for(std::size_t i=0;i<ids.size();++i)
    {
        if(!ids[i]) return false;
        for(std::size_t j=0;j<i;++j)
            if(ids[i]==ids[j]) return false;
    }
    return true;
}

inline bool ValidFleetDecisionAdmission(const FleetDecisionAdmission& a) noexcept
{
    return a.Attempt!=0 && a.Frame!=0 && a.EngineFrame!=0 &&
        std::isfinite(a.ClockSeconds) && a.ClockSeconds>0.0 &&
        a.ClockSeconds<=FleetDecisionMaxSeconds &&
        std::isfinite(a.AdvancedSeconds) && a.AdvancedSeconds>0.0 &&
        a.AdvancedSeconds<=FleetDecisionMaxFrameSeconds &&
        a.ClockSeconds+FleetDecisionClockTolerance>=a.AdvancedSeconds;
}

inline bool FreshFleetDecisionAdmission(const FleetDecisionAdmission& a,
    const FleetDecisionAdmission& b) noexcept
{
    return a.Attempt==b.Attempt && b.Frame>a.Frame &&
        b.EngineFrame>a.EngineFrame && b.Epoch>=a.Epoch &&
        b.ClockSeconds>a.ClockSeconds;
}

inline bool AdjacentFleetDecisionAdmission(const FleetDecisionAdmission& a,
    const FleetDecisionAdmission& b) noexcept
{
    if(!FreshFleetDecisionAdmission(a,b) || b.Frame-a.Frame!=1 ||
        b.EngineFrame-a.EngineFrame!=1 || b.Epoch!=a.Epoch)
        return false;
    const double dt=b.ClockSeconds-a.ClockSeconds;
    return dt<=FleetDecisionMaxFrameSeconds+FleetDecisionClockTolerance &&
        std::abs(dt-b.AdvancedSeconds)<=FleetDecisionClockTolerance;
}

inline bool ValidFleetDecisionRisk(FleetRiskKind kind) noexcept
{
    switch(kind)
    {
        case FleetRiskKind::Unqualified:
        case FleetRiskKind::Clear:
        case FleetRiskKind::Following:
        case FleetRiskKind::StoppingDistance:
        case FleetRiskKind::SweptContact:
        case FleetRiskKind::Touching: return true;
        default: return false;
    }
}

inline bool ValidFleetDecisionQueue(FleetQueueStatus value) noexcept
{
    switch(value)
    {
        case FleetQueueStatus::Inactive:
        case FleetQueueStatus::NoVerifiedQueue:
        case FleetQueueStatus::Waiting:
        case FleetQueueStatus::BudgetExhausted:
        case FleetQueueStatus::InvalidRoute: return true;
        default: return false;
    }
}

inline bool ValidFleetDecisionPass(FleetPassPhase value) noexcept
{
    switch(value)
    {
        case FleetPassPhase::Idle:
        case FleetPassPhase::Confirming:
        case FleetPassPhase::Shifting:
        case FleetPassPhase::Holding:
        case FleetPassPhase::Retired: return true;
        default: return false;
    }
}

inline bool ValidFleetDecisionCommand(const PilotCommand& c) noexcept
{
    return std::isfinite(c.Throttle) && c.Throttle>=0.0 && c.Throttle<=1.0 &&
        std::isfinite(c.Steering) && std::abs(c.Steering)<=1.0;
}

inline bool FleetDecisionPassingActive(FleetPassPhase phase) noexcept
{
    return phase==FleetPassPhase::Shifting || phase==FleetPassPhase::Holding;
}

inline bool FleetDecisionEmergency(FleetRiskKind risk) noexcept
{
    return risk==FleetRiskKind::StoppingDistance ||
        risk==FleetRiskKind::SweptContact || risk==FleetRiskKind::Touching;
}

inline FleetDecisionEndpoint AnalyzeFleetDecision(const FleetDecisionSample& sample) noexcept
{
    FleetDecisionEndpoint out;
    if(sample.Slot<=0 || sample.Slot>=StartGridCapacity ||
        sample.Actor==0 || sample.GateOrdinal<0 || sample.GateOrdinal>=25600 ||
        !std::isfinite(sample.SpeedCmS) || sample.SpeedCmS<0.0 ||
        sample.SpeedCmS>FleetTrafficMaxSpeedCmS ||
        !ValidFleetDecisionRisk(sample.Risk) || !ValidFleetDecisionQueue(sample.Queue) ||
        !ValidFleetDecisionPass(sample.Passing) ||
        sample.LimitingSlot< -1 || sample.LimitingSlot>=StartGridCapacity ||
        sample.LimitingSlot==sample.Slot || sample.OffroadBlockers<0 ||
        sample.OffroadBlockers>MaxRecoveryObstacles ||
        (sample.TrafficQualified && !sample.TrafficObserved) ||
        (sample.TrafficQualified!=(sample.Risk!=FleetRiskKind::Unqualified)) ||
        (!sample.TrafficObserved && (sample.LimitingSlot!=-1 ||
            sample.OffroadBlockers!=0 || sample.RoadSpeedExceeded || sample.BrakeHold)))
        return out;
    if(!sample.Trace.Complete()) return out;
    const auto& stages=sample.Trace.Commands();
    for(const auto& command:stages)
        if(!ValidFleetDecisionCommand(command)) return out;

    out.Valid=true;
    out.Slot=sample.Slot;
    out.GateOrdinal=sample.GateOrdinal;
    out.SpeedCmS=sample.SpeedCmS;
    out.Final=stages.back();
    out.RoadQualified=sample.RoadQualified;
    out.TrafficObserved=sample.TrafficObserved;
    out.TrafficQualified=sample.TrafficQualified;
    out.BrakeHold=sample.BrakeHold;
    out.RoadSpeedExceeded=sample.RoadSpeedExceeded;
    out.OffroadBlockers=sample.OffroadBlockers;
    out.LimitingSlot=sample.LimitingSlot;
    out.Risk=sample.Risk;
    out.Queue=sample.Queue;
    out.Passing=sample.Passing;
    if(stages.front().Brake)
    {
        out.BrakeOrigin=FleetDecisionStage::Route;
        out.Effects[0].BrakeAsserted=true;
    }
    // Attribute actual changes between adjacent pipeline outputs.
    // If a stage releases braking, a later assertion gets its own
    // origin rather than attributing the final brake to a stale stage.
    for(std::size_t i=1;i<stages.size();++i)
    {
        const auto& before=stages[i-1];
        const auto& after=stages[i];
        auto& effect=out.Effects[i];
        const auto stage=static_cast<FleetDecisionStage>(i);
        effect.Reduction=std::max(0.0,before.Throttle-after.Throttle);
        effect.ThrottleReduced=effect.Reduction>FleetDecisionCommandEpsilon;
        effect.ThrottleRaised=after.Throttle-before.Throttle>FleetDecisionCommandEpsilon;
        effect.BrakeAsserted=!before.Brake && after.Brake;
        effect.BrakeReleased=before.Brake && !after.Brake;
        effect.SteeringChanged=std::abs(before.Steering-after.Steering)>FleetDecisionCommandEpsilon;
        if(effect.BrakeAsserted) out.BrakeOrigin=stage;
        if(effect.BrakeReleased) out.BrakeOrigin=FleetDecisionStage::Count;
        if(effect.ThrottleReduced &&
            effect.Reduction>out.LargestThrottleReduction+FleetDecisionCommandEpsilon)
        {
            out.LargestReduction=stage;
            out.LargestThrottleReduction=effect.Reduction;
        }
    }
    out.StationaryNoDrive=sample.SpeedCmS<FleetDecisionStationaryCmS &&
        (out.Final.Brake || out.Final.Throttle<=FleetDecisionNoDriveThreshold);
    return out;
}

struct FleetDecisionStageSummary final
{
    FleetDecisionStage Stage=FleetDecisionStage::Count;
    std::uint64_t ThrottleReductionSamples=0;
    std::uint64_t ThrottleIncreaseSamples=0;
    std::uint64_t BrakeAssertionSamples=0;
    std::uint64_t BrakeReleaseSamples=0;
    std::uint64_t SteeringChangeSamples=0;
    double ReductionSeconds=0.0;
    double IncreaseSeconds=0.0;
    double BrakeOriginSeconds=0.0;
    double SteeringChangeSeconds=0.0;
    double MaxReduction=0.0;
};

struct FleetDecisionEpisode final
{
    bool Valid=false;
    double StartSeconds=0.0;
    double EndSeconds=0.0;
    FleetDecisionStage BrakeOrigin=FleetDecisionStage::Count;
    bool Stationary=false;
    double Duration() const noexcept {return Valid?EndSeconds-StartSeconds:0.0;}
};

// Exclusive observed contexts, not a causal diagnosis of why a body
// stopped moving. Mixed endpoint contexts receive a separate bucket.
enum class FleetNoDriveContext : std::uint8_t
{
    None,
    RoadUnavailable,
    TrafficUnavailable,
    TrafficEmergency,
    TrafficHold,
    Queue,
    OtherBrake,
    LowThrottle,
    Transition,
    Count
};

inline constexpr std::size_t FleetNoDriveContextCount=
    static_cast<std::size_t>(FleetNoDriveContext::Count);

inline FleetNoDriveContext ClassifyFleetNoDrive(const FleetDecisionEndpoint& e) noexcept
{
    if(!e.Valid || !e.StationaryNoDrive) return FleetNoDriveContext::None;
    if(!e.RoadQualified) return FleetNoDriveContext::RoadUnavailable;
    if(!e.TrafficQualified) return FleetNoDriveContext::TrafficUnavailable;
    if(FleetDecisionEmergency(e.Risk) || e.RoadSpeedExceeded)
        return FleetNoDriveContext::TrafficEmergency;
    if(e.BrakeHold) return FleetNoDriveContext::TrafficHold;
    if(e.Queue==FleetQueueStatus::Waiting) return FleetNoDriveContext::Queue;
    return e.Final.Brake ? FleetNoDriveContext::OtherBrake : FleetNoDriveContext::LowThrottle;
}

inline const char* FleetNoDriveContextName(FleetNoDriveContext context) noexcept
{
    switch(context)
    {
        case FleetNoDriveContext::RoadUnavailable: return "road unqualified";
        case FleetNoDriveContext::TrafficUnavailable: return "traffic unqualified";
        case FleetNoDriveContext::TrafficEmergency: return "traffic forecast";
        case FleetNoDriveContext::TrafficHold: return "traffic hold";
        case FleetNoDriveContext::Queue: return "queue";
        case FleetNoDriveContext::OtherBrake: return "other brake";
        case FleetNoDriveContext::LowThrottle: return "low throttle";
        case FleetNoDriveContext::Transition: return "mixed context";
        default: return "unavailable";
    }
}

struct FleetNoDriveBreakdown final
{
    std::array<double,FleetNoDriveContextCount> Seconds{};
    double Total() const noexcept
    {
        double total=0.0;
        for(const double seconds:Seconds) total+=seconds;
        return total;
    }
    FleetNoDriveContext Dominant() const noexcept
    {
        double largest=0.0;
        auto result=FleetNoDriveContext::None;
        for(std::size_t i=1;i<Seconds.size();++i)
            if(Seconds[i]>largest)
            {
                largest=Seconds[i];
                result=static_cast<FleetNoDriveContext>(i);
            }
        return result;
    }
};

struct FleetDecisionSlotSummary final
{
    bool HasSamples=false;
    int Slot=-1;
    int LastGateOrdinal=-1;
    std::uint64_t Samples=0;
    std::uint64_t Intervals=0;
    std::uint64_t Breaks=0;
    std::uint64_t Refused=0;
    double FirstClockSeconds=0.0;
    double LastClockSeconds=0.0;
    double ObservedSeconds=0.0;
    double BrakingSeconds=0.0;
    double StationarySeconds=0.0;
    double NoDriveSeconds=0.0;
    FleetNoDriveBreakdown NoDriveContexts{};
    double RoadUnqualifiedSeconds=0.0;
    double TrafficUnqualifiedSeconds=0.0;
    double EmergencyForecastSeconds=0.0;
    double TrafficHoldSeconds=0.0;
    double QueueSeconds=0.0;
    double PassingSeconds=0.0;
    double DegradedRoadSeconds=0.0;
    double MaxSpeedCmS=0.0;
    double IntegratedSpeed=0.0;
    double IntegratedThrottle=0.0;
    FleetDecisionEpisode LongestBrake{};
    FleetDecisionEpisode LongestStationary{};
    std::array<FleetDecisionStageSummary,FleetDecisionStageCount> Stages{};

    bool HasDuration() const noexcept {return HasSamples && ObservedSeconds>0.0;}
    // Coverage of the elapsed race window, not a claim of time spent
    // driving. Finished rivals and missing samples both leave gaps.
    double RaceWindowCoverage(double activeClock) const noexcept
    {
        if(!HasSamples || !std::isfinite(activeClock) || activeClock<=0.0 ||
            activeClock>FleetDecisionMaxSeconds ||
            LastClockSeconds>activeClock || !std::isfinite(ObservedSeconds) ||
            ObservedSeconds<0.0 || ObservedSeconds>activeClock+FleetDecisionClockTolerance)
            return 0.0;
        return std::clamp(ObservedSeconds/activeClock,0.0,1.0);
    }
    double MeanSpeedKmh() const noexcept
    {
        return HasDuration()?0.036*IntegratedSpeed/ObservedSeconds:0.0;
    }
    double MeanThrottle() const noexcept
    {
        return HasDuration()?IntegratedThrottle/ObservedSeconds:0.0;
    }
    FleetDecisionStage DominantBrakeOrigin() const noexcept
    {
        FleetDecisionStage out=FleetDecisionStage::Count;
        double largest=0.0;
        for(const auto& stage:Stages)
        {
            if(stage.BrakeOriginSeconds>largest)
            {
                largest=stage.BrakeOriginSeconds;
                out=stage.Stage;
            }
        }
        return out;
    }
};

struct FleetDecisionRouteEvidence final
{
    bool Verified=false;
    int AcceptedGates=0;
};

// Fixed capacity, attempt-local observation. A sector is the route segment
// ending at the next accepted gate. This does not infer gate-crossing time,
// road length, official sector duration or responsibility for a delay.
inline constexpr int FleetDecisionMaxSectors=256;
inline constexpr double FleetSectorMinimumComparisonSeconds=0.5;

struct FleetDecisionSectorBucket final
{
    std::uint64_t Intervals=0;
    double ObservedSeconds=0.0;
    double BrakingSeconds=0.0;
    double NoDriveSeconds=0.0;
    double QueueSeconds=0.0;
    double TrafficUnavailableSeconds=0.0;
    double SpeedIntegral=0.0;

    bool Comparable() const noexcept
    {
        return ObservedSeconds+FleetDecisionClockTolerance>=FleetSectorMinimumComparisonSeconds;
    }

    double NoDriveFraction() const noexcept
    {
        return ObservedSeconds>0.0?std::clamp(NoDriveSeconds/ObservedSeconds,0.0,1.0):0.0;
    }

    double MeanSpeedKmh() const noexcept
    {
        return ObservedSeconds>0.0?0.036*SpeedIntegral/ObservedSeconds:0.0;
    }
};

struct FleetDecisionSectorReview final
{
    bool Valid=false;
    int SectorCount=0;
    int ObservedSectors=0;
    int ComparableSectors=0;
    double AttributedSeconds=0.0;
    double TransitionSeconds=0.0;
    int MostNoDriveSector=0;
    FleetDecisionSectorBucket MostNoDrive{};
    int MostBrakingSector=0;
    FleetDecisionSectorBucket MostBraking{};
};

class FleetDecisionSectorHistory final
{
public:
    void Reset() noexcept
    {
        Sectors=0;
        TotalGates=0;
        Buckets={};
        TransitionSeconds=0.0;
    }

    bool Configure(int sectors,int totalGates) noexcept
    {
        if(Sectors || sectors<1 || sectors>FleetDecisionMaxSectors ||
            totalGates<sectors || totalGates>25600 || totalGates%sectors!=0)
            return false;
        Sectors=sectors;
        TotalGates=totalGates;
        return true;
    }

    // Called exclusively for already-admitted adjacent native samples.
    // Both endpoints must name the same ordinal, not just the same
    // modulo sector (a multi-gate jump cannot fabricate attribution).
    bool Observe(const FleetDecisionEndpoint& a,const FleetDecisionEndpoint& b,
        double dt) noexcept
    {
        if(!Sectors || !a.Valid || !b.Valid ||
            a.GateOrdinal<0 || b.GateOrdinal<a.GateOrdinal ||
            b.GateOrdinal>=TotalGates || !std::isfinite(dt) || dt<=0.0 ||
            dt>FleetDecisionMaxFrameSeconds+FleetDecisionClockTolerance ||
            !std::isfinite(a.SpeedCmS) || !std::isfinite(b.SpeedCmS) ||
            a.SpeedCmS<0.0 || b.SpeedCmS<0.0 ||
            a.SpeedCmS>FleetTrafficMaxSpeedCmS || b.SpeedCmS>FleetTrafficMaxSpeedCmS)
            return false;
        if(a.GateOrdinal!=b.GateOrdinal)
        {
            TransitionSeconds=std::min(FleetDecisionMaxSeconds,TransitionSeconds+dt);
            return true;
        }
        auto& s=Buckets[static_cast<std::size_t>(a.GateOrdinal%Sectors)];
        if(s.Intervals!=std::numeric_limits<std::uint64_t>::max()) ++s.Intervals;
        s.ObservedSeconds+=dt;
        if(a.Final.Brake && b.Final.Brake) s.BrakingSeconds+=dt;
        if(a.StationaryNoDrive && b.StationaryNoDrive) s.NoDriveSeconds+=dt;
        if(a.Queue==FleetQueueStatus::Waiting && b.Queue==FleetQueueStatus::Waiting)
            s.QueueSeconds+=dt;
        if(!a.TrafficQualified && !b.TrafficQualified) s.TrafficUnavailableSeconds+=dt;
        s.SpeedIntegral+=(a.SpeedCmS+b.SpeedCmS)*0.5*dt;
        return true;
    }

    bool TrySector(int gate,FleetDecisionSectorBucket& out) const noexcept
    {
        out={};
        if(gate<1 || gate>Sectors) return false;
        out=Buckets[static_cast<std::size_t>(gate-1)];
        return out.Intervals>0;
    }

    FleetDecisionSectorReview Review(double observedSeconds) const noexcept
    {
        FleetDecisionSectorReview out;
        if(!Sectors || !std::isfinite(observedSeconds) || observedSeconds<0.0 ||
            observedSeconds>FleetDecisionMaxSeconds)
            return out;
        out.SectorCount=Sectors;
        out.TransitionSeconds=TransitionSeconds;
        for(int gate=1;gate<=Sectors;++gate)
        {
            FleetDecisionSectorBucket s;
            if(!TrySector(gate,s)) continue;
            ++out.ObservedSectors;
            out.AttributedSeconds+=s.ObservedSeconds;
            if(!s.Comparable()) continue;
            ++out.ComparableSectors;
            // Select by total sampled exposure, not an extrapolated
            // fraction of a complete lap. Stable gate order breaks ties.
            if(s.NoDriveSeconds>out.MostNoDrive.NoDriveSeconds+FleetDecisionClockTolerance)
            {
                out.MostNoDriveSector=gate;
                out.MostNoDrive=s;
            }
            if(s.BrakingSeconds>out.MostBraking.BrakingSeconds+FleetDecisionClockTolerance)
            {
                out.MostBrakingSector=gate;
                out.MostBraking=s;
            }
        }
        // Every observed interval belongs to exactly one bucket or
        // the unattributed gate-transition bucket.
        if(std::abs(out.AttributedSeconds+out.TransitionSeconds-observedSeconds)>
            FleetDecisionClockTolerance*std::max(1.0,observedSeconds))
            return {};
        out.Valid=true;
        return out;
    }

private:
    int Sectors=0;
    int TotalGates=0;
    std::array<FleetDecisionSectorBucket,FleetDecisionMaxSectors> Buckets{};
    double TransitionSeconds=0.0;
};

struct FleetDecisionRivalReview final
{
    int Slot=-1;
    bool Valid=false;
    bool LiveValid=false;
    FleetDecisionEndpoint Live{};
    FleetDecisionSlotSummary History{};
    FleetDecisionSectorReview Sectors{};
};

// Coverage describes the sampled portion of the race clock, not the
// quality of driving or a missing car's behavior.
struct FleetDecisionCoverage final
{
    bool Valid=false;
    int VerifiedSlots=0;
    int DurationSlots=0;
    double ActiveSeconds=0.0;
    double ObservedCarSeconds=0.0;
    double ExpectedCarSeconds=0.0;
    double Fraction() const noexcept
    {
        return Valid && ExpectedCarSeconds>0.0 ?
            std::clamp(ObservedCarSeconds/ExpectedCarSeconds,0.0,1.0) : 0.0;
    }
};

struct FleetDecisionReadout final
{
    bool Valid=false;
    int ObservedRivals=0;
    int CurrentRivals=0;
    int CurrentBraking=0;
    int CurrentTrafficHold=0;
    int CurrentPassing=0;
    FleetDecisionCoverage Coverage{};
    std::array<FleetDecisionRivalReview,StartGridCapacity-1> Rivals{};
};

class FleetDecisionJournal final
{
public:
    void Reset() noexcept
    {
        Attempt=0;
        Identity={};
        TotalGates=0;
        Latest={};
        Open=false;
        Slots={};
    }

    bool Configure(std::uint64_t attempt,const FleetDecisionIdentity& ids,
        int totalGates,int sectorsPerLap) noexcept
    {
        if(Attempt || !attempt || !ValidFleetDecisionIdentity(ids) ||
            totalGates<1 || totalGates>25600 ||
            sectorsPerLap<1 || sectorsPerLap>FleetDecisionMaxSectors ||
            totalGates%sectorsPerLap!=0) return false;
        Attempt=attempt;
        Identity=ids;
        TotalGates=totalGates;
        for(int slot=1;slot<StartGridCapacity;++slot)
        {
            auto& state=Slots[static_cast<std::size_t>(slot-1)];
            state.Sectors.Configure(sectorsPerLap,totalGates);
            auto& summary=state.History;
            summary.Slot=slot;
            for(std::size_t i=0;i<summary.Stages.size();++i)
                summary.Stages[i].Stage=static_cast<FleetDecisionStage>(i);
        }
        return true;
    }

    void Suspend() noexcept
    {
        Open=false;
        for(auto& slot:Slots) Cut(slot);
    }

    bool BeginFrame(const FleetDecisionAdmission& admission) noexcept
    {
        Open=false;
        if(!Attempt || admission.Attempt!=Attempt || !ValidFleetDecisionAdmission(admission) ||
            (Latest.Frame!=0 && !FreshFleetDecisionAdmission(Latest,admission)))
        {
            Suspend();
            return false;
        }
        if(Latest.Frame!=0 && !AdjacentFleetDecisionAdmission(Latest,admission))
            Suspend();
        Latest=admission;
        Open=true;
        return true;
    }

    bool Observe(const FleetDecisionSample& sample) noexcept
    {
        if(!Open || sample.Attempt!=Attempt || sample.Frame!=Latest.Frame ||
            sample.EngineFrame!=Latest.EngineFrame || sample.Epoch!=Latest.Epoch ||
            sample.Slot<=0 || sample.Slot>=StartGridCapacity ||
            sample.Actor!=Identity[static_cast<std::size_t>(sample.Slot)])
            return false;
        auto& slot=Slots[static_cast<std::size_t>(sample.Slot-1)];
        // Each accepted native frame may contribute at most once per
        // physical rival. Rejected duplicates cannot add time or counters.
        if(slot.LastAttemptedFrame==sample.Frame) return false;
        slot.LastAttemptedFrame=sample.Frame;
        const auto endpoint=AnalyzeFleetDecision(sample);
        if(!endpoint.Valid || sample.GateOrdinal>=TotalGates ||
            (slot.History.HasSamples &&
                (sample.GateOrdinal<slot.History.LastGateOrdinal ||
                 sample.TeleportRevision<slot.PreviousTeleport)))
        {
            Increment(slot.History.Refused);
            Cut(slot);
            return false;
        }
        const bool contiguous=slot.HavePrevious &&
            AdjacentFleetDecisionAdmission(slot.PreviousAdmission,Latest) &&
            sample.TeleportRevision==slot.PreviousTeleport;
        if(contiguous)
            AddInterval(slot,slot.Previous,endpoint,slot.PreviousAdmission.ClockSeconds,
                Latest.ClockSeconds);
        else
            Cut(slot);
        ObserveEndpoint(slot.History,endpoint,Latest.ClockSeconds);
        slot.Previous=endpoint;
        slot.PreviousAdmission=Latest;
        slot.PreviousTeleport=sample.TeleportRevision;
        slot.HavePrevious=true;
        slot.Broken=false;
        return true;
    }

    FleetDecisionReadout Read(std::uint64_t attempt,const FleetDecisionIdentity& ids,
        double clock,std::uint64_t frame,bool finished,
        const std::array<FleetDecisionRouteEvidence,StartGridCapacity-1>& evidence) const noexcept
    {
        FleetDecisionReadout out;
        if(!Attempt || attempt!=Attempt || ids!=Identity ||
            !std::isfinite(clock) || clock<0.0 || clock>FleetDecisionMaxSeconds)
            return out;
        out.Valid=true;
        if(clock>0.0)
        {
            out.Coverage.Valid=true;
            out.Coverage.ActiveSeconds=clock;
            out.Coverage.ExpectedCarSeconds=clock*(StartGridCapacity-1);
        }
        for(int i=0;i<StartGridCapacity-1;++i)
        {
            const auto index=static_cast<std::size_t>(i);
            auto& row=out.Rivals[index];
            row.Slot=i+1;
            const auto& slot=Slots[index];
            const auto& e=evidence[index];
            if(!e.Verified || e.AcceptedGates<0 || e.AcceptedGates>TotalGates ||
                !slot.History.HasSamples || slot.History.LastClockSeconds>clock ||
                slot.History.LastGateOrdinal>e.AcceptedGates)
                continue;
            row.Valid=true;
            row.History=slot.History;
            row.Sectors=slot.Sectors.Review(row.History.ObservedSeconds);
            ++out.ObservedRivals;
            if(out.Coverage.Valid)
            {
                ++out.Coverage.VerifiedSlots;
                if(row.History.HasDuration())
                {
                    ++out.Coverage.DurationSlots;
                    out.Coverage.ObservedCarSeconds+=
                        row.History.RaceWindowCoverage(clock)*clock;
                }
            }
            if(!finished && Open && frame==Latest.Frame &&
                slot.PreviousAdmission.Frame==frame && slot.HavePrevious &&
                slot.Previous.Valid && slot.Previous.GateOrdinal==e.AcceptedGates &&
                slot.PreviousAdmission.ClockSeconds==clock)
            {
                row.LiveValid=true;
                row.Live=slot.Previous;
                ++out.CurrentRivals;
                if(row.Live.Final.Brake) ++out.CurrentBraking;
                if(row.Live.BrakeHold) ++out.CurrentTrafficHold;
                if(FleetDecisionPassingActive(row.Live.Passing)) ++out.CurrentPassing;
            }
        }
        return out;
    }

private:
    struct Slot final
    {
        FleetDecisionSlotSummary History{};
        FleetDecisionSectorHistory Sectors{};
        FleetDecisionAdmission PreviousAdmission{};
        FleetDecisionEndpoint Previous{};
        std::uint64_t PreviousTeleport=0;
        std::uint64_t LastAttemptedFrame=0;
        bool HavePrevious=false;
        bool Broken=false;
        FleetDecisionEpisode BrakeEpisode{};
        FleetDecisionEpisode StationaryEpisode{};
    };

    static void Increment(std::uint64_t& counter) noexcept
    {
        if(counter!=std::numeric_limits<std::uint64_t>::max()) ++counter;
    }

    static void AddSeconds(double& total,double dt) noexcept
    {
        total=std::min(FleetDecisionMaxSeconds,total+dt);
    }

    static void Cut(Slot& slot) noexcept
    {
        if(slot.History.HasSamples && !slot.Broken)
        {
            Increment(slot.History.Breaks);
            slot.Broken=true;
        }
        slot.HavePrevious=false;
        slot.BrakeEpisode={};
        slot.StationaryEpisode={};
    }

    static void ObserveEndpoint(FleetDecisionSlotSummary& history,
        const FleetDecisionEndpoint& now,double clock) noexcept
    {
        if(!history.HasSamples) history.FirstClockSeconds=clock;
        history.HasSamples=true;
        history.LastClockSeconds=clock;
        history.LastGateOrdinal=now.GateOrdinal;
        history.MaxSpeedCmS=std::max(history.MaxSpeedCmS,now.SpeedCmS);
        Increment(history.Samples);
        for(std::size_t i=0;i<now.Effects.size();++i)
        {
            const auto& e=now.Effects[i];
            auto& h=history.Stages[i];
            if(e.ThrottleReduced) Increment(h.ThrottleReductionSamples);
            if(e.ThrottleRaised) Increment(h.ThrottleIncreaseSamples);
            if(e.BrakeAsserted) Increment(h.BrakeAssertionSamples);
            if(e.BrakeReleased) Increment(h.BrakeReleaseSamples);
            if(e.SteeringChanged) Increment(h.SteeringChangeSamples);
            h.MaxReduction=std::max(h.MaxReduction,e.Reduction);
        }
    }

    static void ExtendEpisode(FleetDecisionEpisode& current,
        FleetDecisionEpisode& longest,double start,double end,
        bool qualified,FleetDecisionStage origin,bool stationary) noexcept
    {
        if(!qualified)
        {
            current={};
            return;
        }
        if(!current.Valid || current.BrakeOrigin!=origin ||
            current.Stationary!=stationary ||
            std::abs(current.EndSeconds-start)>FleetDecisionClockTolerance)
            current={true,start,end,origin,stationary};
        else
            current.EndSeconds=end;
        if(!longest.Valid || current.Duration()>longest.Duration())
            longest=current;
    }

    static void AddInterval(Slot& slot,const FleetDecisionEndpoint& before,
        const FleetDecisionEndpoint& now,double start,double end) noexcept
    {
        auto& h=slot.History;
        const double dt=end-start;
        Increment(h.Intervals);
        AddSeconds(h.ObservedSeconds,dt);
        slot.Sectors.Observe(before,now,dt);
        // Durations require both endpoints. An unknown middle cannot
        // be counted across missing native samples, a hitch or recovery.
        const bool braking=before.Final.Brake && now.Final.Brake;
        const bool stationary=before.StationaryNoDrive && now.StationaryNoDrive;
        if(braking) AddSeconds(h.BrakingSeconds,dt);
        if(before.SpeedCmS<FleetDecisionStationaryCmS &&
            now.SpeedCmS<FleetDecisionStationaryCmS)
            AddSeconds(h.StationarySeconds,dt);
        if(stationary)
        {
            AddSeconds(h.NoDriveSeconds,dt);
            const auto a=ClassifyFleetNoDrive(before);
            const auto b=ClassifyFleetNoDrive(now);
            const auto context=a==b ? a : FleetNoDriveContext::Transition;
            AddSeconds(h.NoDriveContexts.Seconds[static_cast<std::size_t>(context)],dt);
        }
        if(!before.RoadQualified && !now.RoadQualified)
            AddSeconds(h.RoadUnqualifiedSeconds,dt);
        if(!before.TrafficQualified && !now.TrafficQualified)
            AddSeconds(h.TrafficUnqualifiedSeconds,dt);
        if((FleetDecisionEmergency(before.Risk)||before.RoadSpeedExceeded) &&
            (FleetDecisionEmergency(now.Risk)||now.RoadSpeedExceeded))
            AddSeconds(h.EmergencyForecastSeconds,dt);
        if(before.BrakeHold && now.BrakeHold) AddSeconds(h.TrafficHoldSeconds,dt);
        if(before.Queue==FleetQueueStatus::Waiting && now.Queue==FleetQueueStatus::Waiting)
            AddSeconds(h.QueueSeconds,dt);
        if(FleetDecisionPassingActive(before.Passing) && FleetDecisionPassingActive(now.Passing))
            AddSeconds(h.PassingSeconds,dt);
        if(before.OffroadBlockers>0 && now.OffroadBlockers>0)
            AddSeconds(h.DegradedRoadSeconds,dt);

        h.IntegratedSpeed+=(before.SpeedCmS+now.SpeedCmS)*0.5*dt;
        h.IntegratedThrottle+=(before.Final.Throttle+now.Final.Throttle)*0.5*dt;
        for(std::size_t i=0;i<now.Effects.size();++i)
        {
            const auto& a=before.Effects[i];
            const auto& b=now.Effects[i];
            auto& s=h.Stages[i];
            if(a.ThrottleReduced && b.ThrottleReduced) AddSeconds(s.ReductionSeconds,dt);
            if(a.ThrottleRaised && b.ThrottleRaised) AddSeconds(s.IncreaseSeconds,dt);
            if(a.SteeringChanged && b.SteeringChanged) AddSeconds(s.SteeringChangeSeconds,dt);
        }
        const bool sameBrake=braking && before.BrakeOrigin==now.BrakeOrigin &&
            FleetDecisionIndex(now.BrakeOrigin)<FleetDecisionStageCount;
        if(sameBrake)
            AddSeconds(h.Stages[FleetDecisionIndex(now.BrakeOrigin)].BrakeOriginSeconds,dt);
        ExtendEpisode(slot.BrakeEpisode,h.LongestBrake,start,end,sameBrake,
            now.BrakeOrigin,false);
        ExtendEpisode(slot.StationaryEpisode,h.LongestStationary,start,end,stationary,
            FleetDecisionStage::Count,true);
    }

    std::uint64_t Attempt=0;
    FleetDecisionIdentity Identity{};
    int TotalGates=0;
    FleetDecisionAdmission Latest{};
    bool Open=false;
    std::array<Slot,StartGridCapacity-1> Slots{};
};
} // namespace latebrake
