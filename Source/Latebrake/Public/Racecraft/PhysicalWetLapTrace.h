#pragma once

#include "Racing/PhysicalSixCarReplayTimeline.h"
#include "Track/WorldScaleObservedWaterFilm.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Source-only, physically witnessed six-car WET DRIVING trace. The actual
// Chaos bodies, independent real-road projections and measured grip must
// all agree before a lap sample is eligible. No fictitious time, lap credit,
// car steering, friction, race position or weather is generated.
inline constexpr unsigned WetLapTraceCarCount=PhysicalReplayCarCount;
inline constexpr unsigned WetLapTraceCapacity=96u;
inline constexpr unsigned WetLapTraceSectorCount=16u;
inline constexpr double WetLapTracePi=
    3.1415926535897932384626433832795;

enum class WetLapTraceFault : std::uint8_t
{
    None,
    InvalidPolicy,
    MissingChaosFrame,
    MissingMeasuredWater,
    UntrustedRoadProjection,
    MismatchedPhysicalEpoch,
    WrongVehicleRoster,
    NonmonotonicSourceTime,
    ImpossibleCarMovement,
    UnrealPhysicsAuthorityChanged
};

struct WetLapTraceSettings final
{
    double MaximumSourceClockSkewSeconds=0.05;
    double MaximumFrameIntervalSeconds=0.10;
    double MinimumFrameIntervalSeconds=0.001;
    double MaximumObservedSpeedCmps=15000.0;
    double MaximumMeasuredLateralCm=2200.0;
    double MaximumMeasuredGripFraction=1.0;
    double MinimumMeasuredGripFraction=0.2;
    double MinimumNativeContactConfidence=0.85;
    double MaximumAbsoluteChassisSlipRadians=WetLapTracePi;
    double MaximumPositionContinuitySlackCm=175.0;
    double MaximumStationContinuitySlackCm=250.0;
    double MaximumLateralContinuitySlackCm=150.0;
    double WetRoadThresholdMm=0.5;
    double StandingWaterThresholdMm=3.0;
    unsigned MaximumSkippedSourceFrames=8u;
    bool RequirePhysicalRoadProjection=true;
    bool RequireNativeWheelContact=true;
    bool RequireIndependentGripSensor=true;
    bool RequireExactNativeRoster=true;
    bool NeverWriteChaosWheelForces=true;
    bool NeverAwardLapOrPosition=true;
    bool NeverModifyWeatherActors=true;
};

struct WetLapTraceRoadWitness final
{
    std::uint64_t Actor=0u;
    std::uint64_t SourceFrame=0u;
    std::uint64_t RoadBody=0u;
    double SourceClockSeconds=0.0;
    double MeasuredStationCm=0.0;
    double MeasuredLateralCm=0.0;
    double MeasuredGripFraction=0.0;
    double MeasuredConfidence=0.0;
    bool ActualRoadProjectionObserved=false;
    bool ActualOriginalAsphaltContact=false;
    bool GripIndependentlyMeasured=false;
    bool NativeRaceClockUnchanged=true;
};

struct WetLapTraceCar final
{
    bool Valid=false;
    std::uint64_t Actor=0u;
    double WrappedStationCm=0.0;
    double SignedLateralCm=0.0;
    double WorldXcm=0.0;
    double WorldYcm=0.0;
    double WorldZcm=0.0;
    double PlanarSpeedCmps=0.0;
    double Speed3dCmps=0.0;
    double HeadingRadians=0.0;
    double LateralSlipRadians=0.0;
    double ActualThrottleFraction=0.0;
    double ActualBrakeFraction=0.0;
    double ActualGripFraction=0.0;
    double ActualWaterFilmMm=0.0;
    unsigned SourceRoadSector=0u;
    bool WetPhysicalRoad=false;
    bool StandingWaterObserved=false;
    bool NoNativeRaceAuthority=true;
    bool NoWheelContactModified=true;
};

struct WetLapTraceFrame final
{
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t SourceFrame=0u;
    double SourceClockSeconds=0.0;
    double NativeSourceDeltaSeconds=0.0;
    double CircuitLapLengthCm=0.0;
    double MaximumSourceWaterMm=0.0;
    double MaximumChassisSpeedCmps=0.0;
    unsigned WetVehicleCount=0u;
    unsigned StandingWaterVehicleCount=0u;
    unsigned VerifiedRoadBodies=0u;
    std::array<WetLapTraceCar,WetLapTraceCarCount> Cars{};
    bool SourceOnly=true;
    bool NativeChaosUnchanged=true;
    bool NoOfficialLapTimeChange=true;
};

struct WetLapTraceReceipt final
{
    bool Accepted=false;
    WetLapTraceFault Fault=WetLapTraceFault::MissingChaosFrame;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t AcceptedFrame=0u;
    double AcceptedClockSeconds=0.0;
    unsigned StoredFrames=0u;
    unsigned WetCars=0u;
    bool NoNativeActuation=true;
    bool NoLapOrRaceResultIssued=true;
};

inline bool ValidWetLapTraceSettings(
    WetLapTraceSettings p) noexcept
{
    return std::isfinite(p.MaximumSourceClockSkewSeconds) &&
        p.MaximumSourceClockSkewSeconds>=0.0 &&
        p.MaximumSourceClockSkewSeconds<=0.10 &&
        std::isfinite(p.MaximumFrameIntervalSeconds) &&
        p.MaximumFrameIntervalSeconds>=0.05 &&
        p.MaximumFrameIntervalSeconds<=0.10 &&
        std::isfinite(p.MinimumFrameIntervalSeconds) &&
        p.MinimumFrameIntervalSeconds>=0.001 &&
        p.MinimumFrameIntervalSeconds<=0.01 &&
        p.MaximumFrameIntervalSeconds>p.MinimumFrameIntervalSeconds &&
        std::isfinite(p.MaximumObservedSpeedCmps) &&
        p.MaximumObservedSpeedCmps>=10000.0 &&
        p.MaximumObservedSpeedCmps<=PhysicalReplayMaximumSpeedCmps &&
        std::isfinite(p.MaximumMeasuredLateralCm) &&
        p.MaximumMeasuredLateralCm>=1000.0 &&
        p.MaximumMeasuredLateralCm<=3000.0 &&
        std::isfinite(p.MaximumMeasuredGripFraction) &&
        p.MaximumMeasuredGripFraction>=0.80 &&
        p.MaximumMeasuredGripFraction<=1.0 &&
        std::isfinite(p.MinimumMeasuredGripFraction) &&
        p.MinimumMeasuredGripFraction>=0.10 &&
        p.MinimumMeasuredGripFraction<=0.4 &&
        p.MaximumMeasuredGripFraction>p.MinimumMeasuredGripFraction &&
        std::isfinite(p.MinimumNativeContactConfidence) &&
        p.MinimumNativeContactConfidence>=0.8 &&
        p.MinimumNativeContactConfidence<=1.0 &&
        std::isfinite(p.MaximumAbsoluteChassisSlipRadians) &&
        p.MaximumAbsoluteChassisSlipRadians>=2.0 &&
        p.MaximumAbsoluteChassisSlipRadians<=WetLapTracePi &&
        std::isfinite(p.MaximumPositionContinuitySlackCm) &&
        p.MaximumPositionContinuitySlackCm>=100.0 &&
        p.MaximumPositionContinuitySlackCm<=250.0 &&
        std::isfinite(p.MaximumStationContinuitySlackCm) &&
        p.MaximumStationContinuitySlackCm>=150.0 &&
        p.MaximumStationContinuitySlackCm<=400.0 &&
        std::isfinite(p.MaximumLateralContinuitySlackCm) &&
        p.MaximumLateralContinuitySlackCm>=75.0 &&
        p.MaximumLateralContinuitySlackCm<=250.0 &&
        std::isfinite(p.WetRoadThresholdMm) &&
        p.WetRoadThresholdMm>=0.05 &&
        p.WetRoadThresholdMm<=1.0 &&
        std::isfinite(p.StandingWaterThresholdMm) &&
        p.StandingWaterThresholdMm>=2.0 &&
        p.StandingWaterThresholdMm<=6.0 &&
        p.StandingWaterThresholdMm>p.WetRoadThresholdMm &&
        p.MaximumSkippedSourceFrames>=1u &&
        p.MaximumSkippedSourceFrames<=8u &&
        p.RequirePhysicalRoadProjection &&
        p.RequireNativeWheelContact &&
        p.RequireIndependentGripSensor &&
        p.RequireExactNativeRoster &&
        p.NeverWriteChaosWheelForces &&
        p.NeverAwardLapOrPosition &&
        p.NeverModifyWeatherActors;
}

inline bool ValidWetLapTraceRoadWitness(
    const PhysicalReplayCarSample& car,
    WetLapTraceRoadWitness source,
    const PhysicalReplayFrame& fleet,
    WetLapTraceSettings p={}) noexcept
{
    return FinitePhysicalReplayCar(car) &&
        source.Actor==car.Actor &&
        source.RoadBody!=0u &&
        source.SourceFrame==fleet.SourceFrame &&
        !std::isnan(source.SourceClockSeconds) &&
        std::isfinite(source.SourceClockSeconds) &&
        std::abs(source.SourceClockSeconds-fleet.ActiveClockSeconds)<=
            p.MaximumSourceClockSkewSeconds &&
        std::isfinite(source.MeasuredStationCm) &&
        std::abs(source.MeasuredStationCm)<=1.0e9 &&
        std::isfinite(source.MeasuredLateralCm) &&
        std::abs(source.MeasuredLateralCm)<=p.MaximumMeasuredLateralCm &&
        std::isfinite(source.MeasuredGripFraction) &&
        source.MeasuredGripFraction>=p.MinimumMeasuredGripFraction &&
        source.MeasuredGripFraction<=p.MaximumMeasuredGripFraction &&
        std::isfinite(source.MeasuredConfidence) &&
        source.MeasuredConfidence>=p.MinimumNativeContactConfidence &&
        source.MeasuredConfidence<=1.0 &&
        source.ActualRoadProjectionObserved &&
        source.ActualOriginalAsphaltContact &&
        source.GripIndependentlyMeasured &&
        source.NativeRaceClockUnchanged;
}

inline double WetLapTraceWrappedStationDifference(
    double fromStation,double toStation,
    double lapLength) noexcept
{
    if(!std::isfinite(fromStation) ||
       !std::isfinite(toStation) ||
       !std::isfinite(lapLength) ||
       lapLength<=0.0)
        return std::numeric_limits<double>::infinity();
    const double raw=std::abs(toStation-fromStation);
    if(raw>lapLength+1.0e-6)
        return std::numeric_limits<double>::infinity();
    return std::min(raw,std::abs(lapLength-raw));
}

inline double WetLapTraceSlipAngle(
    const PhysicalReplayCarSample& car) noexcept
{
    const double planar=std::hypot(
        car.VelocityXcmps,car.VelocityYcmps);
    if(!std::isfinite(planar) || planar<1.0)return 0.0;
    const double heading=std::atan2(
        car.VelocityYcmps,car.VelocityXcmps);
    const double difference=heading-car.HeadingRadians;
    return std::atan2(std::sin(difference),std::cos(difference));
}

inline WetLapTraceFrame BuildWetLapTraceFrame(
    const WorldScaleVariantRecipe& circuit,
    const PhysicalReplayFrame& fleet,
    const ObservedWaterResult& water,
    const std::array<WetLapTraceRoadWitness,WetLapTraceCarCount>& sources,
    WetLapTraceSettings p={}) noexcept
{
    WetLapTraceFrame out;
    if(!ValidWetLapTraceSettings(p) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       ValidatePhysicalReplayFrame(fleet)!=PhysicalReplayReject::None)
        return out;
    if(!water.Valid ||
       water.Fault!=ObservedWaterFault::None ||
       water.Circuit!=circuit.Id ||
       water.Session!=fleet.Session ||
       water.Revision!=fleet.SessionRevision ||
       water.SourceFrame!=fleet.SourceFrame ||
       !std::isfinite(water.ClockSeconds) ||
       std::abs(water.ClockSeconds-fleet.ActiveClockSeconds)>
           p.MaximumSourceClockSkewSeconds ||
       !water.RenderAdvisoryOnly ||
       !water.OriginalRaceUnaffected)
        return out;
    out.Circuit=circuit.Id;
    out.Session=fleet.Session;
    out.Revision=fleet.SessionRevision;
    out.SourceFrame=fleet.SourceFrame;
    out.SourceClockSeconds=fleet.ActiveClockSeconds;
    out.NativeSourceDeltaSeconds=fleet.SourceDeltaSeconds;
    out.CircuitLapLengthCm=circuit.LapLengthCm;
    for(std::size_t i=0u;i<WetLapTraceCarCount;++i)
    {
        const auto& car=fleet.Cars[i];
        const auto& witness=sources[i];
        if(!ValidWetLapTraceRoadWitness(car,witness,fleet,p))
            return {};
        // Different cars on the same circuit normally touch the SAME
        // original asphalt collision body. The car actor must be unique,
        // but the authenticated road body is allowed to be shared.
        for(std::size_t j=0u;j<i;++j)
        {
            if(witness.Actor==sources[j].Actor)
                return {};
        }
        const auto film=SampleObservedWaterFilm(
            circuit,water,witness.MeasuredStationCm);
        const double slip=WetLapTraceSlipAngle(car);
        const double speed=PhysicalReplaySpeed(car);
        const double planar=std::hypot(
            car.VelocityXcmps,car.VelocityYcmps);
        if(!film.Valid ||
           !std::isfinite(slip) ||
           !std::isfinite(speed) ||
           !std::isfinite(planar) ||
           speed>p.MaximumObservedSpeedCmps ||
           std::abs(slip)>p.MaximumAbsoluteChassisSlipRadians)
            return {};
        WetLapTraceCar result;
        result.Valid=true;
        result.Actor=car.Actor;
        result.WrappedStationCm=WorldScaleVariantWrapDistance(
            circuit,witness.MeasuredStationCm);
        result.SignedLateralCm=witness.MeasuredLateralCm;
        result.WorldXcm=car.WorldXcm;
        result.WorldYcm=car.WorldYcm;
        result.WorldZcm=car.WorldZcm;
        result.PlanarSpeedCmps=planar;
        result.Speed3dCmps=speed;
        result.HeadingRadians=car.HeadingRadians;
        result.LateralSlipRadians=slip;
        result.ActualThrottleFraction=car.ThrottleNormalized;
        result.ActualBrakeFraction=car.BrakeNormalized;
        result.ActualGripFraction=witness.MeasuredGripFraction;
        result.ActualWaterFilmMm=film.FilmMm;
        result.SourceRoadSector=std::min(
            static_cast<unsigned>(
                result.WrappedStationCm/circuit.LapLengthCm*
                    WetLapTraceSectorCount),
            WetLapTraceSectorCount-1u);
        result.WetPhysicalRoad=
            result.ActualWaterFilmMm>=p.WetRoadThresholdMm;
        result.StandingWaterObserved=
            result.ActualWaterFilmMm>=p.StandingWaterThresholdMm;
        out.Cars[i]=result;
        ++out.VerifiedRoadBodies;
        if(result.WetPhysicalRoad)++out.WetVehicleCount;
        if(result.StandingWaterObserved)
            ++out.StandingWaterVehicleCount;
        out.MaximumSourceWaterMm=std::max(
            out.MaximumSourceWaterMm,result.ActualWaterFilmMm);
        out.MaximumChassisSpeedCmps=std::max(
            out.MaximumChassisSpeedCmps,speed);
    }
    out.Valid=out.VerifiedRoadBodies==WetLapTraceCarCount;
    return out;
}

inline bool WetLapTracePlausibleContinuity(
    const WetLapTraceFrame& previous,
    const WetLapTraceFrame& next,
    WetLapTraceSettings p={}) noexcept
{
    if(!previous.Valid || !next.Valid ||
       previous.Circuit!=next.Circuit ||
       previous.Session!=next.Session ||
       previous.Revision!=next.Revision ||
       next.SourceFrame<=previous.SourceFrame ||
       next.SourceFrame-previous.SourceFrame>
           p.MaximumSkippedSourceFrames ||
       !std::isfinite(previous.SourceClockSeconds) ||
       !std::isfinite(next.SourceClockSeconds) ||
       !std::isfinite(next.CircuitLapLengthCm) ||
       next.CircuitLapLengthCm!=previous.CircuitLapLengthCm)
        return false;
    const double dt=next.SourceClockSeconds-
        previous.SourceClockSeconds;
    if(!std::isfinite(dt) ||
       dt<p.MinimumFrameIntervalSeconds ||
       dt>p.MaximumFrameIntervalSeconds ||
       std::abs(dt-next.NativeSourceDeltaSeconds)>0.02)
        return false;
    for(unsigned i=0u;i<WetLapTraceCarCount;++i)
    {
        const auto& a=previous.Cars[i];
        const auto& b=next.Cars[i];
        if(!a.Valid || !b.Valid || a.Actor!=b.Actor)return false;
        const double speed=std::max(a.Speed3dCmps,b.Speed3dCmps);
        const double traveled=std::hypot(
            std::hypot(b.WorldXcm-a.WorldXcm,
                       b.WorldYcm-a.WorldYcm),
            b.WorldZcm-a.WorldZcm);
        const double projected=WetLapTraceWrappedStationDifference(
            a.WrappedStationCm,b.WrappedStationCm,
            previous.CircuitLapLengthCm);
        const double lateral=std::abs(
            b.SignedLateralCm-a.SignedLateralCm);
        if(!std::isfinite(traveled) ||
           !std::isfinite(projected) ||
           !std::isfinite(lateral) ||
           traveled>speed*dt+p.MaximumPositionContinuitySlackCm ||
           projected>speed*dt+p.MaximumStationContinuitySlackCm ||
           lateral>speed*dt+p.MaximumLateralContinuitySlackCm)
            return false;
    }
    return true;
}

class PhysicalWetLapTraceJournal final
{
public:
    void Reset() noexcept
    {
        Size=0u;
        Next=0u;
        Last=WetLapTraceFault::None;
    }
    std::size_t Count() const noexcept {return Size;}
    WetLapTraceFault LastFault() const noexcept {return Last;}
    bool Read(std::size_t chronological,WetLapTraceFrame& out) const noexcept
    {
        out={};
        if(chronological>=Size)return false;
        out=FrameAt(chronological);
        return true;
    }
    WetLapTraceReceipt Record(
        const WetLapTraceFrame& frame,
        WetLapTraceSettings p={}) noexcept
    {
        WetLapTraceReceipt result;
        if(!ValidWetLapTraceSettings(p))
        {
            result.Fault=WetLapTraceFault::InvalidPolicy;
            Last=result.Fault;
            return result;
        }
        if(!frame.Valid ||
           frame.VerifiedRoadBodies!=WetLapTraceCarCount ||
           !frame.SourceOnly ||
           !frame.NativeChaosUnchanged ||
           !frame.NoOfficialLapTimeChange)
        {
            result.Fault=WetLapTraceFault::MissingChaosFrame;
            Last=result.Fault;
            return result;
        }
        if(Size>0u)
        {
            const auto& previous=FrameAt(Size-1u);
            if(frame.Session!=previous.Session ||
               frame.Revision!=previous.Revision ||
               frame.Circuit!=previous.Circuit)
            {
                result.Fault=WetLapTraceFault::MismatchedPhysicalEpoch;
                Last=result.Fault;
                return result;
            }
            if(frame.SourceFrame<=previous.SourceFrame ||
               frame.SourceFrame-previous.SourceFrame>
                   p.MaximumSkippedSourceFrames)
            {
                result.Fault=WetLapTraceFault::NonmonotonicSourceTime;
                Last=result.Fault;
                return result;
            }
            if(!WetLapTracePlausibleContinuity(previous,frame,p))
            {
                result.Fault=WetLapTraceFault::ImpossibleCarMovement;
                Last=result.Fault;
                return result;
            }
        }
        Frames[Next]=frame;
        Next=(Next+1u)%WetLapTraceCapacity;
        if(Size<WetLapTraceCapacity)++Size;
        Last=WetLapTraceFault::None;
        result.Accepted=true;
        result.Fault=WetLapTraceFault::None;
        result.Circuit=frame.Circuit;
        result.Session=frame.Session;
        result.AcceptedFrame=frame.SourceFrame;
        result.AcceptedClockSeconds=frame.SourceClockSeconds;
        result.StoredFrames=static_cast<unsigned>(Size);
        result.WetCars=frame.WetVehicleCount;
        return result;
    }
private:
    const WetLapTraceFrame& FrameAt(
        std::size_t chronological) const noexcept
    {
        const std::size_t oldest=
            (Next+WetLapTraceCapacity-Size)%WetLapTraceCapacity;
        return Frames[(oldest+chronological)%WetLapTraceCapacity];
    }
    std::array<WetLapTraceFrame,WetLapTraceCapacity> Frames{};
    std::size_t Size=0u;
    std::size_t Next=0u;
    WetLapTraceFault Last=WetLapTraceFault::None;
};
} // namespace latebrake
