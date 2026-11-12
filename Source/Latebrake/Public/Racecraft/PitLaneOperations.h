#pragma once
#include "Racecraft/CircuitGeometry.h"
#include "Racecraft/RacecraftFrames.h"
#include "Racing/RacePitService.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class PitLanePhase : std::uint8_t
{
    Outside, Approaching, Entering, LaneTransit,
    BrakingForBox, StoppedAtBox, ServiceRequested,
    WaitingForPhysicalService, Departing, Exiting,
    Completed, Unsafe
};
enum class PitLaneSafetyFault : std::uint8_t
{
    None, NotPhysical, MissingPitGeometry, OffRoad,
    TooFast, UnauthorizedService, WrongDirection,
    EngineOnWrongEpoch, ServiceWithoutStop,
    UnsafeThrottle, WrongBox, NoActualServiceReceipt,
    ChassisMoving, RaceNotRunning
};
struct PitLaneSettings final
{
    double BayCenterXcm=latebrake::RacePitBay.X;
    double BayCenterYcm=latebrake::RacePitBay.Y;
    double BayRadiusCm=latebrake::RacePitBayRadiusCm;
    double EntranceDistanceCm=1050.0;
    double ExitDistanceCm=1050.0;
    double LaneHalfWidthCm=260.0;
    double EntrySpeedLimitCmS=1500.0;
    double LaneSpeedLimitCmS=900.0;
    double ExitSpeedLimitCmS=1300.0;
    double ServiceStopSpeedThresholdCmS=45.0;
    double ServiceThrottleThreshold=0.02;
    double ServiceHoldSeconds=2.5;
    double MaximumFrameSeconds=0.10;
    double MinimumActualServiceDurationSeconds=0.3;
    double MinimumPhysicalContactFraction=0.75;
    double MaximumYawErrorRadians=0.25;
    double MinimumEntryGapCm=500.0;
    double MaximumPitClockGapSeconds=0.12;
    double MaximumEarlyExitSpeedCmS=900.0;
    bool RequireExactOriginalBay=true;
    bool RequireOriginalPhysicalCar=true;
    bool RequireServiceGameModeReceipt=true;
    bool ProhibitServiceWhileMoving=true;
    bool ProhibitUnsafeReentry=true;
};
struct PitLaneObservation final
{
    PhysicalVehicleFrame Car{};
    double DeltaSeconds=0.0;
    double MeasuredPlanarSpeedCmS=0.0;
    double OriginalRaceClockSeconds=0.0;
    double EngineWorldSeconds=0.0;
    double RequestedPitServiceHoldSeconds=0.0;
    double ActualServiceElapsedSeconds=0.0;
    int PhysicalServiceReceiptCount=0;
    bool ServiceCommandRequested=false;
    bool OriginalPitBayCollisionVerified=false;
    bool EngineServiceAuthorized=false;
    bool GameModeAcceptedPitStop=false;
    bool ActualServiceApplied=false;
    bool VehicleMechanicallyServiced=false;
    bool PitLanePhysicallyPresent=false;
};
struct PitLaneDecision final
{
    PitLanePhase Phase=PitLanePhase::Outside;
    PitLaneSafetyFault Fault=PitLaneSafetyFault::NotPhysical;
    double DistanceToPitBayCm=0.0;
    double DistanceToPitEntranceCm=0.0;
    double DistanceToPitExitCm=0.0;
    double PitLaneOffsetCm=0.0;
    double CurrentSpeedCmS=0.0;
    double MaximumAllowedSpeedCmS=0.0;
    double RequiredBrakingDistanceCm=0.0;
    double RequiredDecelerationCmS2=0.0;
    double ServiceHoldProgress=0.0;
    double AdvisoryThrottleCeiling=0.0;
    double AdvisoryBrakeFloor=1.0;
    bool NearPitEntrance=false;
    bool PhysicallyInPitLane=false;
    bool InOriginalServiceBay=false;
    bool CompletelyStationary=false;
    bool TrueServiceCompleted=false;
    bool MustStayStopped=false;
    bool TrueRacePitRulesApplied=false;
    bool VehicleControlsModified=false;
    bool Valid=false;
};
struct PitLaneHistory final
{
    std::uint64_t QualifiedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t LaneEntries=0;
    std::uint64_t ServiceRequests=0;
    std::uint64_t PhysicalServices=0;
    std::uint64_t UnsafeSpeedFrames=0;
    std::uint64_t UnauthorizedServiceAttempts=0;
    double TotalLaneSeconds=0.0;
    double TotalStationarySeconds=0.0;
    double HighestLaneSpeedCmS=0.0;
    double MinimumServiceBayDistanceCm=0.0;
    double LastServiceRaceClockSeconds=0.0;
    PitLanePhase LastPhase=PitLanePhase::Outside;
    PitLaneSafetyFault LastFault=PitLaneSafetyFault::None;
    bool IssuedActualRepair=false;
};
inline bool ValidPitLaneSettings(PitLaneSettings s) noexcept
{
    return Finite(s.BayCenterXcm) &&
        Finite(s.BayCenterYcm) &&
        Finite(s.BayRadiusCm) &&
        s.BayRadiusCm>0.0 &&
        Finite(s.EntranceDistanceCm) &&
        s.EntranceDistanceCm>0.0 &&
        Finite(s.ExitDistanceCm) &&
        s.ExitDistanceCm>0.0 &&
        Finite(s.LaneHalfWidthCm) &&
        s.LaneHalfWidthCm>0.0 &&
        Finite(s.EntrySpeedLimitCmS) &&
        s.EntrySpeedLimitCmS>0.0 &&
        Finite(s.LaneSpeedLimitCmS) &&
        s.LaneSpeedLimitCmS>0.0 &&
        Finite(s.ExitSpeedLimitCmS) &&
        s.ExitSpeedLimitCmS>0.0 &&
        Finite(s.ServiceStopSpeedThresholdCmS) &&
        s.ServiceStopSpeedThresholdCmS>=0.0 &&
        Finite(s.ServiceHoldSeconds) &&
        s.ServiceHoldSeconds>0.0 &&
        Finite(s.MaximumFrameSeconds) &&
        s.MaximumFrameSeconds>0.0 &&
        s.MaximumFrameSeconds<=MaxSimulationFrameSeconds &&
        Finite(s.MinimumActualServiceDurationSeconds) &&
        s.MinimumActualServiceDurationSeconds>0.0 &&
        Finite(s.MinimumPhysicalContactFraction) &&
        s.MinimumPhysicalContactFraction>0.0 &&
        s.MinimumPhysicalContactFraction<=1.0 &&
        Finite(s.MaximumYawErrorRadians) &&
        s.MaximumYawErrorRadians>0.0;
}
inline bool PitBayWithinOriginalRoad(PitLaneSettings s) noexcept
{
    return CenterOnOriginalRoad({
        s.BayCenterXcm,s.BayCenterYcm}) &&
        s.BayRadiusCm<=OriginalRoadHalfWidthCm;
}
inline bool ValidPitLaneObservation(
    PitLaneObservation sample,PitLaneSettings s) noexcept
{
    if (ValidateVehicleFrame(sample.Car,true)!=
            SnapshotFault::None ||
        !ValidFrameDelta(sample.DeltaSeconds) ||
        sample.DeltaSeconds>s.MaximumFrameSeconds ||
        !Finite(sample.MeasuredPlanarSpeedCmS) ||
        !Finite(sample.OriginalRaceClockSeconds) ||
        !Finite(sample.EngineWorldSeconds) ||
        !Finite(sample.RequestedPitServiceHoldSeconds) ||
        !Finite(sample.ActualServiceElapsedSeconds) ||
        sample.MeasuredPlanarSpeedCmS<0.0 ||
        sample.MeasuredPlanarSpeedCmS>MaximumPhysicalSpeedCmS ||
        sample.OriginalRaceClockSeconds<0.0 ||
        sample.EngineWorldSeconds<0.0 ||
        sample.RequestedPitServiceHoldSeconds<0.0 ||
        sample.ActualServiceElapsedSeconds<0.0 ||
        sample.PhysicalServiceReceiptCount<0 ||
        !sample.OriginalPitBayCollisionVerified ||
        !sample.PitLanePhysicallyPresent)
        return false;
    return true;
}
inline PitLaneDecision EvaluatePitLaneApproach(
    PitLaneObservation sample,
    PitLaneSettings s={}) noexcept
{
    PitLaneDecision out;
    if (!ValidPitLaneSettings(s) ||
        !PitBayWithinOriginalRoad(s) ||
        !ValidPitLaneObservation(sample,s))
        return out;
    const Vec2 position=
        sample.Car.Chassis.PlanarPose.Position;
    const Vec2 pitCenter{s.BayCenterXcm,s.BayCenterYcm};
    const double distance=Distance(position,pitCenter);
    const double forwardSpeed=ForwardSpeed(
        {sample.Car.Chassis.VelocityCmS.X,
         sample.Car.Chassis.VelocityCmS.Y},
        sample.Car.Chassis.PlanarPose.HeadingRadians);
    out.CurrentSpeedCmS=sample.MeasuredPlanarSpeedCmS;
    out.DistanceToPitBayCm=distance;
    out.DistanceToPitEntranceCm=std::max(
        0.0,distance-s.EntranceDistanceCm);
    out.DistanceToPitExitCm=
        distance+s.ExitDistanceCm;
    out.PitLaneOffsetCm=
        position.Y-s.BayCenterYcm;
    out.NearPitEntrance=
        distance<s.EntranceDistanceCm;
    out.PhysicallyInPitLane=
        out.NearPitEntrance &&
        std::abs(out.PitLaneOffsetCm)<s.LaneHalfWidthCm &&
        CenterOnOriginalRoad(position);
    out.InOriginalServiceBay=
        distance<=s.BayRadiusCm &&
        out.PhysicallyInPitLane;
    out.CompletelyStationary=
        sample.MeasuredPlanarSpeedCmS<=
            s.ServiceStopSpeedThresholdCmS;
    out.MaximumAllowedSpeedCmS=
        out.InOriginalServiceBay ?
        s.ServiceStopSpeedThresholdCmS :
        out.PhysicallyInPitLane ?
        s.LaneSpeedLimitCmS :
        out.NearPitEntrance ?
        s.EntrySpeedLimitCmS :
        MaximumPhysicalSpeedCmS;
    out.RequiredBrakingDistanceCm=
        StoppingEnvelope(
            sample.MeasuredPlanarSpeedCmS,
            std::min(sample.MeasuredPlanarSpeedCmS,
                     out.MaximumAllowedSpeedCmS),
            800.0,0.18).DistanceCm;
    out.RequiredDecelerationCmS2=
        SafeDivision(
            Square(sample.MeasuredPlanarSpeedCmS)-
            Square(std::min(
                sample.MeasuredPlanarSpeedCmS,
                out.MaximumAllowedSpeedCmS)),
            2.0*std::max(
                100.0,distance-s.BayRadiusCm));
    out.AdvisoryThrottleCeiling=
        sample.MeasuredPlanarSpeedCmS>
            out.MaximumAllowedSpeedCmS?0.0:1.0;
    out.AdvisoryBrakeFloor=Clamp01(
        out.RequiredDecelerationCmS2/950.0);
    out.Phase=PitLanePhase::Outside;
    out.Fault=PitLaneSafetyFault::None;
    out.Valid=true;
    if (sample.Car.Race.Phase!=RacePhase::Green &&
        sample.Car.Race.Phase!=RacePhase::Caution)
    {
        out.Fault=PitLaneSafetyFault::RaceNotRunning;
        out.Phase=PitLanePhase::Unsafe;
        return out;
    }
    if (forwardSpeed<-50.0 &&
        out.NearPitEntrance)
    {
        out.Fault=PitLaneSafetyFault::WrongDirection;
        out.Phase=PitLanePhase::Unsafe;
        return out;
    }
    if (out.NearPitEntrance)
        out.Phase=PitLanePhase::Approaching;
    if (out.PhysicallyInPitLane)
        out.Phase=PitLanePhase::LaneTransit;
    if (out.InOriginalServiceBay)
        out.Phase=out.CompletelyStationary ?
            PitLanePhase::StoppedAtBox :
            PitLanePhase::BrakingForBox;
    if (out.InOriginalServiceBay &&
        sample.ServiceCommandRequested)
        out.Phase=PitLanePhase::ServiceRequested;
    if (sample.MeasuredPlanarSpeedCmS>
        out.MaximumAllowedSpeedCmS+60.0)
    {
        out.Fault=PitLaneSafetyFault::TooFast;
        out.AdvisoryThrottleCeiling=0.0;
        out.AdvisoryBrakeFloor=std::max(
            out.AdvisoryBrakeFloor,0.55);
    }
    if (sample.ServiceCommandRequested)
    {
        if (!out.InOriginalServiceBay ||
            !sample.EngineServiceAuthorized ||
            !out.CompletelyStationary ||
            sample.Car.Controls.Throttle>
                s.ServiceThrottleThreshold)
        {
            out.Fault=PitLaneSafetyFault::UnauthorizedService;
            out.Phase=PitLanePhase::Unsafe;
            out.MustStayStopped=true;
            out.AdvisoryThrottleCeiling=0.0;
            out.AdvisoryBrakeFloor=1.0;
            return out;
        }
        out.ServiceHoldProgress=Clamp01(
            sample.RequestedPitServiceHoldSeconds/
            s.ServiceHoldSeconds);
    }
    out.TrueServiceCompleted=
        sample.GameModeAcceptedPitStop &&
        sample.ActualServiceApplied &&
        sample.VehicleMechanicallyServiced &&
        sample.ActualServiceElapsedSeconds>=
            s.MinimumActualServiceDurationSeconds &&
        out.CompletelyStationary &&
        out.InOriginalServiceBay;
    if (sample.GameModeAcceptedPitStop &&
        !out.TrueServiceCompleted)
    {
        out.Fault=PitLaneSafetyFault::NoActualServiceReceipt;
        out.Phase=PitLanePhase::Unsafe;
    }
    if (out.TrueServiceCompleted)
        out.Phase=PitLanePhase::Completed;
    out.TrueRacePitRulesApplied=false;
    out.VehicleControlsModified=false;
    return out;
}
class PhysicalPitLaneMonitor final
{
public:
    bool Configure(PitLaneSettings settings) noexcept
    {
        if (!ValidPitLaneSettings(settings) ||
            !PitBayWithinOriginalRoad(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        History={};
        Actor=0;
        Session=0;
        HasEpoch=false;
        WasInside=false;
        PreviousServiceReceipts=0;
    }
    PitLaneDecision Observe(PitLaneObservation sample) noexcept
    {
        auto reviewed=EvaluatePitLaneApproach(
            sample,Settings);
        if (!reviewed.Valid)
        {
            ++History.RejectedFrames;
            return {};
        }
        if (HasEpoch &&
            (Actor!=sample.Car.Identity.Actor ||
             Session!=sample.Car.Identity.Session))
        {
            Reset();
        }
        Actor=sample.Car.Identity.Actor;
        Session=sample.Car.Identity.Session;
        HasEpoch=true;
        if (reviewed.PhysicallyInPitLane)
        {
            History.TotalLaneSeconds+=sample.DeltaSeconds;
            if (!WasInside)
                ++History.LaneEntries;
            History.HighestLaneSpeedCmS=std::max(
                History.HighestLaneSpeedCmS,
                reviewed.CurrentSpeedCmS);
        }
        WasInside=reviewed.PhysicallyInPitLane;
        if (reviewed.CompletelyStationary)
            History.TotalStationarySeconds+=
                sample.DeltaSeconds;
        if (sample.ServiceCommandRequested)
            ++History.ServiceRequests;
        if (reviewed.TrueServiceCompleted &&
            sample.PhysicalServiceReceiptCount>
                PreviousServiceReceipts)
        {
            ++History.PhysicalServices;
            History.LastServiceRaceClockSeconds=
                sample.OriginalRaceClockSeconds;
        }
        PreviousServiceReceipts=
            sample.PhysicalServiceReceiptCount;
        if (reviewed.Fault==PitLaneSafetyFault::TooFast)
            ++History.UnsafeSpeedFrames;
        if (reviewed.Fault==
            PitLaneSafetyFault::UnauthorizedService)
            ++History.UnauthorizedServiceAttempts;
        History.LastPhase=reviewed.Phase;
        History.LastFault=reviewed.Fault;
        History.IssuedActualRepair=false;
        if (History.QualifiedFrames==0)
            History.MinimumServiceBayDistanceCm=
                reviewed.DistanceToPitBayCm;
        else History.MinimumServiceBayDistanceCm=std::min(
            History.MinimumServiceBayDistanceCm,
            reviewed.DistanceToPitBayCm);
        ++History.QualifiedFrames;
        Last=reviewed;
        return Last;
    }
    PitLaneDecision Snapshot() const noexcept{return Last;}
    PitLaneHistory HistorySnapshot() const noexcept{return History;}
private:
    PitLaneSettings Settings{};
    PitLaneDecision Last{};
    PitLaneHistory History{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    int PreviousServiceReceipts=0;
    bool HasEpoch=false;
    bool WasInside=false;
};
} // namespace latebrake::racecraft
