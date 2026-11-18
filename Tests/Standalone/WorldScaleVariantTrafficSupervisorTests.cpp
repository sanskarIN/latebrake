#include "AI/WorldScaleVariantFleetTrafficArbitration.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Require(bool okay,const char* message)
{
    if(!okay)
    {
        std::cerr<<"WorldScaleVariantTrafficSupervisor FAIL: "
                 <<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

WorldScaleVariantPilotReadout Pilot(
    WorldScaleVariantId id=WorldScaleVariantId::OriginalLoop)
{
    WorldScaleVariantPilotReadout out{};
    out.Valid=true;
    out.SourceTrack=id;
    out.SourceActor=1100;
    out.SourceSessionToken=101;
    out.SourceFrame=2;
    out.OriginalPhysicalRoadVerified=true;
    out.HasLiveActuatorAuthority=false;
    out.OfficialLapCredit=false;
    out.Decision={
        true,{0.85,0.2,false,false},
        10000.0,0.0,1000.0,2500.0,0.1
    };
    return out;
}

WorldScaleVariantFleetReadout Fleet(
    WorldScaleVariantId id=WorldScaleVariantId::OriginalLoop)
{
    WorldScaleVariantFleetReadout out{};
    out.Valid=true;
    out.MotionContinuous=true;
    out.TrackId=id;
    out.SourceSessionToken=101;
    out.SourceFrameOrdinal=2;
    out.AcceptedContinuousFrames=1;
    for(unsigned i=0;i<WorldScaleVariantFleetCars;++i)
    {
        auto& b=out.CarsBySlot[i];
        b.Slot=static_cast<int>(i);
        b.ActorToken=1100+i;
        b.MeasuredSpeedCmps=2000.0;
        b.SignedRouteSpeedCmps=2000.0;
        b.SourceLapStationCm=10000.0+10000.0*i;
        b.UnwrappedProjectedStationCm=10000.0+10000.0*i;
        b.UnwrappedReportedStationCm=b.UnwrappedProjectedStationCm;
        b.SignedLateralOffsetCm=0.0;
        b.CenterlineResidualCm=0.0;
        b.WithinOriginalRoad=true;
        out.AdvisoryOrder[i]=i;
    }
    out.CarsBySlot[0].SignedRouteSpeedCmps=2100.0;
    out.CarsBySlot[0].MeasuredSpeedCmps=2100.0;
    return out;
}

void IndependentSixCircuitCases()
{
    for(unsigned track=0;track<WorldScaleVariantCount;++track)
    {
        const auto id=WorldScaleVariantAt(track).Id;
        const auto pilot=Pilot(id);
        const auto empty=Fleet(id);
        auto result=ApplyWorldScaleVariantTrafficSupervisor(
            pilot,empty,0);
        Require(result.Valid && !result.EmergencyBrake &&
                !result.FollowingThrottleRestricted &&
                result.FullPhysicalRosterQualified &&
                result.LimitingOtherSlot==-1 &&
                result.Decision.Command.Throttle==0.85 &&
                result.Decision.Command.Steering==0.2 &&
                !result.Decision.Command.Brake &&
                result.NoLiveBrakeActuatorAuthority &&
                result.SourceTrack==id &&
                result.SourceSessionToken==101 &&
                result.SourceFrameOrdinal==2,
                "unopposed authenticated rival keeps original safe pilot advice");

        auto approaching=empty;
        approaching.CarsBySlot[1].UnwrappedProjectedStationCm=12000.0;
        approaching.CarsBySlot[1].UnwrappedReportedStationCm=12000.0;
        result=ApplyWorldScaleVariantTrafficSupervisor(
            pilot,approaching,0);
        Require(result.Valid && !result.EmergencyBrake &&
                result.FollowingThrottleRestricted &&
                result.LimitingOtherSlot==1 &&
                result.Decision.Command.Throttle<=0.15 &&
                result.Decision.Command.Throttle>=0.0 &&
                !result.Decision.Command.Brake &&
                result.Decision.Command.Steering==0.2 &&
                result.ConservativeStopDistanceCm>0.0,
                "2m source following gap caps throttle without inventing brakes");

        auto threat=empty;
        threat.CarsBySlot[1].UnwrappedProjectedStationCm=10500.0;
        threat.CarsBySlot[1].UnwrappedReportedStationCm=10500.0;
        threat.CarsBySlot[1].SignedRouteSpeedCmps=1300.0;
        threat.CarsBySlot[1].MeasuredSpeedCmps=1300.0;
        result=ApplyWorldScaleVariantTrafficSupervisor(
            pilot,threat,0);
        Require(result.Valid && result.EmergencyBrake &&
                !result.FollowingThrottleRestricted &&
                result.LimitingOtherSlot==1 &&
                result.Decision.Command.Brake &&
                result.Decision.Command.Throttle==0.0 &&
                result.ThrottleCeiling==0.0 &&
                result.MaximumApproachSpeedCmps>=800.0,
                "short source-backed slow-car gap demands full advisory brake");

        auto beside=threat;
        beside.CarsBySlot[1].SignedLateralOffsetCm=350.0;
        result=ApplyWorldScaleVariantTrafficSupervisor(
            pilot,beside,0);
        Require(result.Valid && !result.EmergencyBrake &&
                result.Decision.Command.Throttle==0.85,
                "physically distinct lanes do not trigger false contact braking");

        auto behind=threat;
        behind.CarsBySlot[1].UnwrappedProjectedStationCm=9500.0;
        behind.CarsBySlot[1].UnwrappedReportedStationCm=9500.0;
        result=ApplyWorldScaleVariantTrafficSupervisor(
            pilot,behind,0);
        Require(result.Valid && !result.EmergencyBrake &&
                result.LimitingOtherSlot==-1,
                "a trailing rival cannot be misclassified as forward conflict");

        auto slowdown=pilot;
        slowdown.Decision.Command.Brake=true;
        slowdown.Decision.Command.Throttle=0.0;
        result=ApplyWorldScaleVariantTrafficSupervisor(
            slowdown,empty,0);
        Require(result.Valid && result.Decision.Command.Brake &&
                result.Decision.Command.Throttle==0.0,
                "traffic never releases an earlier physics or corner brake");

        auto rivalReverse=threat;
        rivalReverse.CarsBySlot[1].SignedRouteSpeedCmps=-1300.0;
        result=ApplyWorldScaleVariantTrafficSupervisor(
            pilot,rivalReverse,0);
        Require(result.Valid && result.EmergencyBrake &&
                result.Decision.Command.Throttle==0.0,
                "wrong-way traffic closing at combined speed fails conservatively");

        // Source projection uses SIGNED unwrapped station to distinguish
        // vehicles on consecutive revolutions of the same physical road.
        auto previousLap=threat;
        previousLap.CarsBySlot[1].UnwrappedProjectedStationCm=
            10500.0-WorldScaleVariantAt(track).LapLengthCm;
        previousLap.CarsBySlot[1].UnwrappedReportedStationCm=
            previousLap.CarsBySlot[1].UnwrappedProjectedStationCm;
        result=ApplyWorldScaleVariantTrafficSupervisor(
            pilot,previousLap,0);
        Require(result.Valid && !result.EmergencyBrake &&
                result.LimitingOtherSlot==-1,
                "same XY location a revolution behind is not the same car gap");

        auto nearTwo=threat;
        nearTwo.CarsBySlot[2].UnwrappedProjectedStationCm=10400.0;
        nearTwo.CarsBySlot[2].UnwrappedReportedStationCm=10400.0;
        result=ApplyWorldScaleVariantTrafficSupervisor(
            pilot,nearTwo,0);
        Require(result.Valid && result.EmergencyBrake &&
                result.LimitingOtherSlot==2 &&
                result.ClosestRelevantGapCm==400.0,
                "the nearest unsafe measured rival limits six-way traffic");
    }
}

void RejectUnqualifiedData()
{
    const auto pilot=Pilot();
    const auto fleet=Fleet();
    const auto reject=[&](const WorldScaleVariantPilotReadout& a,
                          const WorldScaleVariantFleetReadout& b,
                          int slot,
                          WorldScaleVariantTrafficSettings s={})
    {
        const auto out=ApplyWorldScaleVariantTrafficSupervisor(
            a,b,slot,s);
        Require(!out.Valid && !out.FullPhysicalRosterQualified &&
                out.NoLiveBrakeActuatorAuthority &&
                out.Decision.Command.Brake &&
                out.Decision.Command.Throttle==0.0,
                "invalid source always returns zero-power, braked advice");
    };
    reject(pilot,fleet,-1);
    reject(pilot,fleet,6);
    auto p=pilot;
    p.SourceSessionToken=102;
    reject(p,fleet,0);
    p=pilot;
    p.SourceFrame=3;
    reject(p,fleet,0);
    p=pilot;
    p.SourceTrack=WorldScaleVariantId::TestOval;
    reject(p,fleet,0);
    p=pilot;
    p.SourceActor=999;
    reject(p,fleet,0);
    p=pilot;
    p.Valid=false;
    reject(p,fleet,0);
    p=pilot;
    p.Decision.Valid=false;
    reject(p,fleet,0);
    p=pilot;
    p.Decision.Command.Throttle=2.0;
    reject(p,fleet,0);
    p=pilot;
    p.Decision.Command.Steering=
        std::numeric_limits<double>::quiet_NaN();
    reject(p,fleet,0);
    p=pilot;
    p.HasLiveActuatorAuthority=true;
    reject(p,fleet,0);
    p=pilot;
    p.OfficialLapCredit=true;
    reject(p,fleet,0);

    auto f=fleet;
    f.MotionContinuous=false;
    reject(pilot,f,0);
    f=fleet;
    f.AcceptedContinuousFrames=0;
    reject(pilot,f,0);
    f=fleet;
    f.NoOfficialLapCredit=false;
    reject(pilot,f,0);
    f=fleet;
    f.SourceSessionToken=0;
    reject(pilot,f,0);
    f=fleet;
    f.CarsBySlot[5].ActorToken=f.CarsBySlot[4].ActorToken;
    reject(pilot,f,0);
    f=fleet;
    f.CarsBySlot[5].Slot=4;
    reject(pilot,f,0);
    f=fleet;
    f.CarsBySlot[2].WithinOriginalRoad=false;
    reject(pilot,f,0);
    f=fleet;
    f.CarsBySlot[2].UnwrappedProjectedStationCm=
        std::numeric_limits<double>::infinity();
    reject(pilot,f,0);
    f=fleet;
    f.CarsBySlot[2].MeasuredSpeedCmps=-1.0;
    reject(pilot,f,0);
    f=fleet;
    f.CarsBySlot[2].SignedLateralOffsetCm=
        std::numeric_limits<double>::quiet_NaN();
    reject(pilot,f,0);

    auto settings=WorldScaleVariantTrafficSettings{};
    settings.RequireAllSixRealActors=false;
    reject(pilot,fleet,0,settings);
    settings={};
    settings.NeverIncreasePilotThrottle=false;
    reject(pilot,fleet,0,settings);
    settings={};
    settings.NeverReleaseOriginalPilotBrake=false;
    reject(pilot,fleet,0,settings);
    settings={};
    settings.NoSteeringOrCollisionAuthority=false;
    reject(pilot,fleet,0,settings);
    settings={};
    settings.AvailableBrakingCmps2=
        std::numeric_limits<double>::infinity();
    reject(pilot,fleet,0,settings);
    settings={};
    settings.LaneConflictLateralCm=100.0;
    reject(pilot,fleet,0,settings);
}

void FrameSequenceReceipt()
{
    const auto pilot=Pilot();
    const auto fleet=Fleet();
    for(std::uint64_t frame=2;frame<100;++frame)
    {
        auto p=pilot;
        auto f=fleet;
        p.SourceFrame=frame;
        f.SourceFrameOrdinal=frame;
        f.AcceptedContinuousFrames=frame-1;
        const auto r=ApplyWorldScaleVariantTrafficSupervisor(
            p,f,0);
        Require(r.Valid && r.SourceFrameOrdinal==frame &&
                r.SourceSessionToken==101,
                "source-only traffic uses a matched physics frame receipt");
    }
}
void WholeFleetAtomicArbitration()
{
    for(unsigned n=0;n<WorldScaleVariantCount;++n)
    {
        const auto id=WorldScaleVariantAt(n).Id;
        const auto fleet=Fleet(id);
        std::array<WorldScaleVariantPilotReadout,
                   WorldScaleVariantFleetCars> pilots{};
        for(unsigned slot=0;slot<WorldScaleVariantFleetCars;++slot)
        {
            pilots[slot]=Pilot(id);
            pilots[slot].SourceActor=fleet.CarsBySlot[slot].ActorToken;
        }
        auto result=ApplyWorldScaleVariantFleetTraffic(pilots,fleet);
        Require(result.Valid &&
                result.AllSixPilotsPhysicallyQualified &&
                result.SourceTrack==id &&
                result.SourceSessionToken==fleet.SourceSessionToken &&
                result.SourceFrameOrdinal==fleet.SourceFrameOrdinal &&
                result.EmergencyBrakeCount==0 &&
                result.RestrictedThrottleCount==0 &&
                result.NoOfficialLapCredit &&
                result.NoLiveActuatorAuthority,
                "all six source rival pilots agree on track/session/frame");
        for(unsigned slot=0;slot<WorldScaleVariantFleetCars;++slot)
        {
            Require(result.Car[slot].Valid &&
                    result.Car[slot].SelfSlot==static_cast<int>(slot) &&
                    result.Car[slot].Decision.Command.Throttle==0.85 &&
                    !result.Car[slot].Decision.Command.Brake &&
                    result.Car[slot].NoLiveBrakeActuatorAuthority,
                    "all real-source roster slots receive individual clean-road advice");
        }

        auto close=fleet;
        close.CarsBySlot[1].UnwrappedProjectedStationCm=10500.0;
        close.CarsBySlot[1].UnwrappedReportedStationCm=10500.0;
        close.CarsBySlot[1].SignedRouteSpeedCmps=1300.0;
        result=ApplyWorldScaleVariantFleetTraffic(pilots,close);
        Require(result.Valid && result.EmergencyBrakeCount>=1u &&
                result.Car[0].EmergencyBrake &&
                result.Car[0].Decision.Command.Throttle==0.0 &&
                result.Car[0].Decision.Command.Brake &&
                result.Car[1].Decision.Command.Throttle==0.85,
                "six-slot arbitration preserves independent safe braking decisions");

        const auto reject=[&](const auto& badPilots,
                              const auto& badFleet)
        {
            const auto rejected=ApplyWorldScaleVariantFleetTraffic(
                badPilots,badFleet);
            Require(!rejected.Valid &&
                    !rejected.AllSixPilotsPhysicallyQualified &&
                    rejected.EmergencyBrakeCount==0 &&
                    rejected.RestrictedThrottleCount==0,
                    "one faulty physical source invalidates entire six-driver frame");
            for(const auto& car:rejected.Car)
            {
                Require(!car.Valid && car.Decision.Command.Brake &&
                        car.Decision.Command.Throttle==0.0 &&
                        car.NoLiveBrakeActuatorAuthority,
                        "no partial advisory may leak after whole-fleet rejection");
            }
        };
        auto invalid=pilots;
        invalid[4].SourceFrame+=1u;
        reject(invalid,fleet);
        invalid=pilots;
        invalid[3].SourceSessionToken+=1u;
        reject(invalid,fleet);
        invalid=pilots;
        invalid[2].SourceActor=pilots[0].SourceActor;
        reject(invalid,fleet);
        invalid=pilots;
        invalid[5].SourceTrack=WorldScaleVariantId::TestOval;
        if(id==WorldScaleVariantId::TestOval)
            invalid[5].SourceTrack=WorldScaleVariantId::OriginalLoop;
        reject(invalid,fleet);
        invalid=pilots;
        invalid[1].Valid=false;
        reject(invalid,fleet);
        invalid=pilots;
        invalid[2].Decision.Command.Throttle=
            std::numeric_limits<double>::quiet_NaN();
        reject(invalid,fleet);

        auto badFleet=fleet;
        badFleet.MotionContinuous=false;
        reject(pilots,badFleet);
        badFleet=fleet;
        badFleet.SourceSessionToken=0;
        reject(pilots,badFleet);
        badFleet=fleet;
        badFleet.CarsBySlot[5].ActorToken=0;
        reject(pilots,badFleet);
        badFleet=fleet;
        badFleet.CarsBySlot[3].WithinOriginalRoad=false;
        reject(pilots,badFleet);
    }
}

} // namespace

int main()
{
    IndependentSixCircuitCases();
    RejectUnqualifiedData();
    FrameSequenceReceipt();
    WholeFleetAtomicArbitration();
    std::cout<<"WorldScaleVariantTrafficSupervisor PASS: all six "
                 "proposed circuits, six-body token/clock matching, "
                 "lane separation, following, braking, reverse "
                 "traffic and invalid-evidence fail-safe. "
                 "No actual Unreal vehicle or multiplayer session.\n";
    return EXIT_SUCCESS;
}
