#include "AI/WorldScaleVariantRivalPilot.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok,const char* why)
{
    if(!ok)
    {
        std::cerr<<"WorldScaleVariantRivalPilot FAIL: "
                 <<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::WorldScaleVariantPilotObservation DriverAt(
    const latebrake::WorldScaleVariantRecipe& track,
    double station,double signedSpeed,double dt,
    std::uint64_t ordinal=1u,double clock=100.0)
{
    using namespace latebrake;
    const auto current=WorldScaleVariantCenterlineAt(track,station);
    const auto before=WorldScaleVariantCenterlineAt(
        track,station-0.5*dt*signedSpeed);
    const auto after=WorldScaleVariantCenterlineAt(
        track,station+0.5*dt*signedSpeed);
    Check(current.Valid&&before.Valid&&after.Valid,
          "mock car pose/velocity comes exclusively from authored circuit");
    const CircuitPoint velocity{
        (after.Pose.Position.X-before.Pose.Position.X)/dt,
        (after.Pose.Position.Y-before.Pose.Position.Y)/dt
    };
    const double heading=current.Pose.HeadingDegrees*
        WorldScalePaceRadPerDegree;
    const double speed=std::hypot(velocity.X,velocity.Y);
    const double bodySpeed=velocity.X*std::cos(heading)+
        velocity.Y*std::sin(heading);
    WorldScalePilotSample physical{
        current.Pose.Position,heading,speed,bodySpeed,
        dt,true,true
    };
    return {
        physical,velocity,42u,1000u,ordinal,clock,
        true,true
    };
}

void VerifyCarOnTrack(
    const latebrake::WorldScaleVariantRecipe& track,double fps)
{
    using namespace latebrake;
    const auto plan=BuildWorldScaleVariantPace(track);
    Check(plan.Valid,"authored variant pace plan exists");
    WorldScaleVariantRivalPilot pilot{};
    const double dt=1.0/fps;
    const double speed=2050.0;
    const double start=track.LapLengthCm-185.0;
    for(unsigned frame=1u;frame<=150u;++frame)
    {
        const double station=start+(frame-1u)*dt*speed;
        const auto source=DriverAt(
            track,station,speed,dt,frame,100.0+dt*frame);
        const auto out=pilot.Step(plan,source);
        Check(out.Valid &&
              out.SourceTrack==track.Id &&
              out.SourceFrame==frame &&
              out.SourceActor==1000u &&
              out.OriginalPhysicalRoadVerified &&
              !out.HasLiveActuatorAuthority &&
              !out.OfficialLapCredit &&
              out.Decision.Valid &&
              out.Decision.WrappedLapDistanceCm>=0.0 &&
              out.Decision.WrappedLapDistanceCm<
                  track.LapLengthCm &&
              out.Decision.SpeedReferenceCmps>=
                  WorldScalePaceMinSpeedCmps-1e-6 &&
              out.Decision.SpeedReferenceCmps<=
                  WorldScalePaceMaxSpeedCmps+1e-6 &&
              out.Decision.LookaheadCm>=WorldScalePilotMinLookaheadCm &&
              out.Decision.LookaheadCm<=WorldScalePilotMaxLookaheadCm &&
              out.Decision.Command.Steering>=-1.0 &&
              out.Decision.Command.Steering<=1.0 &&
              out.Decision.Command.Throttle>=0.0 &&
              out.Decision.Command.Throttle<=1.0 &&
              (!out.Decision.Command.Brake ||
                  out.Decision.Command.Throttle==0.0) &&
              std::abs(out.WorldMotionResidualCm)<=10.0,
              "actual measured 30/60/120Hz rival has bounded circuit-specific steering and braking");
    }
}

void RejectedObservations(
    const latebrake::WorldScaleVariantRecipe& track)
{
    using namespace latebrake;
    const auto plan=BuildWorldScaleVariantPace(track);
    WorldScaleVariantRivalPilot pilot;
    constexpr double dt=1.0/60.0;
    const auto grid=DriverAt(track,10000.0,1000.0,dt);
    Check(pilot.Step(plan,grid).Valid,
          "trusted source baseline exists");

    auto bad=grid;
    bad.PhysicsFrame=2;
    bad.PhysicsClockSeconds+=dt;
    bad.OriginalRoadContactVerified=false;
    Check(!pilot.Step(plan,bad).Valid,
          "no real road contact denies source driving authority");
    Check(pilot.Step(plan,grid).Valid,
          "trusted car can reacquire after missing contact");

    bad=grid;
    bad.AuthenticatedActorObservation=false;
    Check(!pilot.Step(plan,bad).Valid,
          "fabricated actor source cannot be treated as real Chaos car");

    bad=grid;
    bad.MeasuredWorldVelocityCmps.X=
        std::numeric_limits<double>::quiet_NaN();
    Check(!pilot.Step(plan,bad).Valid,
          "nonfinite actual physics velocity fails closed");

    bad=grid;
    bad.MeasuredWorldVelocityCmps={0.0,0.0};
    Check(!pilot.Step(plan,bad).Valid,
          "a speedometer cannot claim nonzero speed with stationary velocity");

    bad=grid;
    bad.Physical.ForwardSpeedCmps=
        -bad.Physical.FlatSpeedCmps;
    Check(!pilot.Step(plan,bad).Valid,
          "reported signed forward speed must match actual observed XY velocity");

    bad=grid;
    bad.Physical.RaceGreen=false;
    Check(!pilot.Step(plan,bad).Valid,
          "unverified start sequence has no accelerating permission");

    bad=grid;
    bad.PhysicsClockSeconds=
        std::numeric_limits<double>::infinity();
    Check(!pilot.Step(plan,bad).Valid,
          "infinite physics timestamp cannot preserve source continuity");

    bad=grid;
    bad.SessionToken=0;
    Check(!pilot.Step(plan,bad).Valid,
          "anonymous race sessions fail closed");

    Check(pilot.Step(plan,grid).Valid,
          "valid measured source can restart after negative probes");
    const auto stale=DriverAt(
        track,10000.0+1000.0*dt,1000.0,dt,2,
        grid.PhysicsClockSeconds);
    Check(!pilot.Step(plan,stale).Valid,
          "monotone frame number cannot conceal a frozen engine clock");

    Check(pilot.Step(plan,grid).Valid,
          "fresh baseline for spatial teleport");
    const auto teleported=DriverAt(
        track,15000.0,1000.0,dt,2,grid.PhysicsClockSeconds+dt);
    Check(!pilot.Step(plan,teleported).Valid,
          "unrealistically long physical move is rejected at 60Hz");

    Check(pilot.Step(plan,grid).Valid,
          "fresh baseline before actor replacement");
    bad=DriverAt(track,10000.0+1000.0*dt,
                 1000.0,dt,2,grid.PhysicsClockSeconds+dt);
    bad.ActorToken=9999u;
    Check(!pilot.Step(plan,bad).Valid,
          "a different physical car cannot inherit previous steering state");

    Check(pilot.Step(plan,grid).Valid,
          "fresh baseline before route source swap");
    auto sameContentDifferentPlan=plan;
    bad=DriverAt(track,10000.0+1000.0*dt,
                 1000.0,dt,2,grid.PhysicsClockSeconds+dt);
    Check(!pilot.Step(sameContentDifferentPlan,bad).Valid,
          "another plan object cannot reuse continuity across same race");

    pilot.Reset();
    const auto launch=DriverAt(track,10000.0,0.0,dt);
    const auto launchResult=pilot.Step(plan,launch);
    Check(launchResult.Valid && launchResult.Decision.Valid &&
          launchResult.Decision.Command.Throttle>0.0 &&
          !launchResult.Decision.Command.Brake,
          "stationary aligned car may be advised to start moving");

    pilot.Reset();
    const auto fast=DriverAt(track,10000.0,7500.0,dt);
    const auto overspeed=pilot.Step(plan,fast);
    Check(overspeed.Valid &&
          overspeed.Decision.Command.Brake &&
          overspeed.Decision.Command.Throttle==0.0,
          "unsafe actual high-speed cornering demands a brake not acceleration");

    pilot.Reset();
    auto backwards=DriverAt(track,10000.0,2000.0,dt);
    backwards.Physical.HeadingRadians+=WorldScalePilotRadiansPi;
    backwards.Physical.ForwardSpeedCmps*=-1.0;
    const auto reversed=pilot.Step(plan,backwards);
    Check(reversed.Valid &&
          reversed.Decision.Command.Brake &&
          reversed.Decision.Command.Throttle==0.0,
          "backwards-facing real body with reverse signed motion receives no propulsion");

    pilot.Reset();
    const auto reverseMotion=DriverAt(track,10000.0,-1600.0,dt);
    const auto response=pilot.Step(plan,reverseMotion);
    Check(response.Valid &&
          response.Decision.Command.Brake &&
          response.Decision.Command.Throttle==0.0,
          "actual reverse travel retains original dry pilot brake authority");
}
} // namespace

int main()
{
    for(unsigned i=0;i<latebrake::WorldScaleVariantCount;++i)
    {
        const auto track=latebrake::WorldScaleVariantAt(i);
        for(double fps:{30.0,60.0,120.0})
        {
            VerifyCarOnTrack(track,fps);
        }
        RejectedObservations(track);
    }
    std::cout<<"WorldScaleVariantRivalPilot PASS: six distinct "
                 "source circuits, real XY speed and signed forward "
                 "evidence, 30/60/120Hz lap seams, steering/throttle/brake "
                 "and invalid-source fail-closed control (no UE actor)\n";
    return EXIT_SUCCESS;
}
