#include "AI/WorldScaleVariantFleetContinuityAudit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using Distances=std::array<double,latebrake::WorldScaleVariantFleetCars>;
using Speeds=std::array<double,latebrake::WorldScaleVariantFleetCars>;

void Require(bool yes,const char* why)
{
    if(!yes)
    {
        std::cerr<<"WorldScaleVariantFleetContinuity FAIL: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::WorldScaleVariantFleetFrame Observe(
    const latebrake::WorldScaleVariantRecipe& recipe,
    std::uint64_t frame,
    const Distances& stations,
    const Speeds& speed,
    double dt,
    std::uint64_t session=123)
{
    using namespace latebrake;
    WorldScaleVariantFleetFrame f{};
    f.FrameOrdinal=frame;
    f.SessionToken=session;
    f.DeltaSeconds=dt;
    f.ObservationClockSeconds=100.0+frame*dt;
    f.RaceGreen=true;
    f.PhysicsEngineQualified=true;
    f.SixRealActorsConfirmed=true;
    for(unsigned i=0;i<WorldScaleVariantFleetCars;++i)
    {
        const double source=stations[i];
        const auto here=WorldScaleVariantCenterlineAt(recipe,source);
        const auto before=WorldScaleVariantCenterlineAt(
            recipe,source-speed[i]*dt*0.5);
        const auto after=WorldScaleVariantCenterlineAt(
            recipe,source+speed[i]*dt*0.5);
        Require(here.Valid && before.Valid && after.Valid,
                "source mock only uses actual authored centerline geometry");
        WorldScaleVariantFleetActor actor{};
        actor.Slot=static_cast<int>(i);
        actor.ActorToken=1000u+i;
        actor.SignedSpatialRevolution=static_cast<int>(
            std::floor(source/recipe.LapLengthCm));
        actor.ReportedWrappedStationCm=
            WorldScaleVariantWrapDistance(recipe,source);
        actor.WorldPosition=here.Pose.Position;
        actor.WorldVelocityCmps={
            (after.Pose.Position.X-before.Pose.Position.X)/dt,
            (after.Pose.Position.Y-before.Pose.Position.Y)/dt
        };
        actor.OriginalRoadContactQualified=true;
        f.Actors[i]=actor;
    }
    return f;
}

void Advance(Distances& station,const Speeds& speed,double dt)
{
    for(unsigned i=0;i<station.size();++i)
        station[i]+=speed[i]*dt;
}

// The full-pace reference must live at one address across frames,
// exactly as a future Unreal circuit-owner would own its track data.
void VerifyCircuit(const latebrake::WorldScaleVariantRecipe& recipe)
{
    using namespace latebrake;
    const auto plan=BuildWorldScaleVariantPace(recipe);
    Require(plan.Valid,"future physical road has a valid local pace plan");

    for(double fps:{30.0,60.0,120.0})
    {
        WorldScaleVariantFleetContinuityAudit audit;
        const double dt=1.0/fps;
        Distances stations{};
        Speeds speeds{};
        for(unsigned i=0;i<WorldScaleVariantFleetCars;++i)
        {
            stations[i]=recipe.LapLengthCm-225.0+800.0*i;
            speeds[i]=2100.0+50.0*i;
        }
        for(std::uint64_t frame=1;frame<=90;++frame)
        {
            const auto observed=Observe(
                recipe,frame,stations,speeds,dt);
            const auto result=audit.Observe(plan,observed);
            Require(result.Valid &&
                    result.TrackId==recipe.Id &&
                    result.NoOfficialLapCredit &&
                    result.DoesNotControlVehicles &&
                    result.AdvisoryOnly &&
                    result.MotionContinuous==(frame>1u) &&
                    result.RejectedFrames==0 &&
                    result.AcceptedContinuousFrames==frame-1u &&
                    std::abs(result.ContinuousPhysicalSeconds-
                        (frame-1u)*dt)<=1e-6,
                    "all six source cars stay continuous through signed seam "
                    "at 30/60/120Hz");
            std::array<bool,WorldScaleVariantFleetCars> ordered{};
            for(unsigned place=0;place<WorldScaleVariantFleetCars;++place)
            {
                const unsigned slot=result.AdvisoryOrder[place];
                Require(slot<WorldScaleVariantFleetCars &&
                        !ordered[slot] &&
                        result.CarsBySlot[slot].ActorToken==1000u+slot &&
                        result.CarsBySlot[slot].WithinOriginalRoad &&
                        std::isfinite(result.CarsBySlot[slot].
                            UnwrappedProjectedStationCm),
                        "advisory ranking is based on uniquely observed "
                        "world XY and actual per-track lap length");
                ordered[slot]=true;
            }
            Advance(stations,speeds,dt);
        }
    }
}

void InvalidSourcesAndContinuity(
    const latebrake::WorldScaleVariantRecipe& recipe)
{
    using namespace latebrake;
    const auto plan=BuildWorldScaleVariantPace(recipe);
    const auto alternate=BuildWorldScaleVariantPace(
        WorldScaleVariantAt(
            (static_cast<unsigned>(recipe.Id)+1u)%
                WorldScaleVariantCount));
    Require(plan.Valid&&alternate.Valid,"two source tracks for swap rejection");
    const double dt=1.0/60.0;
    Distances positions{};
    Speeds speed{};
    for(unsigned i=0;i<WorldScaleVariantFleetCars;++i)
    {
        positions[i]=10000.0+700.0*i;
        speed[i]=2000.0;
    }
    const auto base=Observe(recipe,1,positions,speed,dt);
    WorldScaleVariantFleetContinuityAudit audit;
    Require(audit.Observe(plan,base).Valid,
            "qualified initial 6-car physical snapshot is accepted");

    auto bad=base;
    bad.FrameOrdinal=2;
    bad.ObservationClockSeconds+=dt;
    bad.Actors[4].OriginalRoadContactQualified=false;
    Require(!audit.Observe(plan,bad).Valid,
            "lost wheel/road source invalidates entire fleet");
    Require(audit.Readout().RejectedFrames==1,
            "invalid source increments monotonic rejection counter");

    bad=base;
    bad.FrameOrdinal=3;
    bad.Actors[5].ActorToken=bad.Actors[4].ActorToken;
    Require(!audit.Observe(plan,bad).Valid,
            "duplicate real actor identity cannot become sixth rival");

    bad=base;
    bad.FrameOrdinal=4;
    bad.Actors[3].Slot=bad.Actors[2].Slot;
    Require(!audit.Observe(plan,bad).Valid,
            "duplicate slot cannot invent a real final car");

    bad=base;
    bad.FrameOrdinal=5;
    bad.Actors[5].WorldPosition.X=
        std::numeric_limits<double>::infinity();
    Require(!audit.Observe(plan,bad).Valid,
            "infinite world location rejects entire fleet");

    bad=base;
    bad.FrameOrdinal=6;
    bad.Actors[1].ReportedWrappedStationCm+=5000.0;
    Require(!audit.Observe(plan,bad).Valid,
            "reported route station cannot deviate from measured XY");

    bad=base;
    bad.FrameOrdinal=7;
    bad.SixRealActorsConfirmed=false;
    Require(!audit.Observe(plan,bad).Valid,
            "a fifth missing physical car is not a valid traffic slot");

    bad=base;
    bad.FrameOrdinal=8;
    bad.ObservationClockSeconds=
        std::numeric_limits<double>::quiet_NaN();
    Require(!audit.Observe(plan,bad).Valid,
            "nonfinite engine timestamp never yields a standings receipt");

    audit.Reset();
    Require(audit.Observe(plan,base).Valid,
            "reset establishes a fully fresh epoch");
    bad=base;
    bad.FrameOrdinal=2;
    bad.ObservationClockSeconds+=dt;
    Require(!audit.Observe(alternate,bad).Valid,
            "same session may not silently switch to another track map");

    audit.Reset();
    Require(audit.Observe(plan,base).Valid,
            "baseline before stale clock injection");
    bad=base;
    bad.FrameOrdinal=2;
    Require(!audit.Observe(plan,bad).Valid,
            "incremented ordinal with stale physics clock fails closed");

    audit.Reset();
    Require(audit.Observe(plan,base).Valid,
            "baseline before frozen motion");
    bad=base;
    bad.FrameOrdinal=2;
    bad.ObservationClockSeconds+=dt;
    Require(!audit.Observe(plan,bad).Valid,
            "frozen measured chassis with stale moving velocity fails");
    
    audit.Reset();
    Require(audit.Observe(plan,base).Valid,
            "baseline before forged station progress");
    Advance(positions,speed,dt);
    bad=Observe(recipe,2,positions,speed,dt);
    bad.Actors[3].ReportedWrappedStationCm+=110.0;
    Require(!audit.Observe(plan,bad).Valid,
            "forged 110cm reported progress cannot bypass real motion proof");

    audit.Reset();
    Require(audit.Observe(plan,base).Valid,
            "initial complete source roster");
    bad=Observe(recipe,3,positions,speed,dt);
    const auto gap=audit.Observe(plan,bad);
    Require(gap.Valid && !gap.MotionContinuous &&
            gap.AcceptedContinuousFrames==0,
            "one missed physics frame is a new observation baseline, "
            "not made-up continuous race evidence");
}

void HonestReverseNotPositiveProgress(
    const latebrake::WorldScaleVariantRecipe& recipe)
{
    using namespace latebrake;
    const auto plan=BuildWorldScaleVariantPace(recipe);
    WorldScaleVariantFleetContinuityAudit audit;
    constexpr double dt=1.0/60.0;
    Distances station{};
    Speeds speed{};
    for(unsigned i=0;i<WorldScaleVariantFleetCars;++i)
    {
        station[i]=9000.0+i*650.0;
        speed[i]=(i==2u)?-1700.0:1800.0;
    }
    const auto first=audit.Observe(
        plan,Observe(recipe,1,station,speed,dt));
    Require(first.Valid && !first.MotionContinuous,
            "real backward-moving chassis can create a neutral baseline");
    Advance(station,speed,dt);
    const auto second=audit.Observe(
        plan,Observe(recipe,2,station,speed,dt));
    Require(second.Valid && second.MotionContinuous &&
            second.CarsBySlot[2].SignedRouteSpeedCmps < -1000.0 &&
            second.CarsBySlot[2].UnwrappedReportedStationCm <
                first.CarsBySlot[2].UnwrappedReportedStationCm &&
            second.NoOfficialLapCredit,
            "honest signed reverse motion is tracked but not awarded laps");
}
} // namespace

int main()
{
    for(unsigned i=0;i<latebrake::WorldScaleVariantCount;++i)
    {
        const auto recipe=latebrake::WorldScaleVariantAt(i);
        VerifyCircuit(recipe);
        InvalidSourcesAndContinuity(recipe);
        HonestReverseNotPositiveProgress(recipe);
    }
    std::cout<<"WorldScaleVariantFleetContinuity PASS: six real-actor "
                 "source contract across six different lap lengths, "
                 "30/60/120Hz seam, observed ranking and authenticated "
                 "reverse/teleport/forged-source rejection, NO Unreal\n";
    return EXIT_SUCCESS;
}
