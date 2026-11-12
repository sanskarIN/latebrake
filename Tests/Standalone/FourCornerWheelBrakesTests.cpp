#include "Vehicles/FourCornerWheelBrakes.h"
#include "Vehicles/GripLimitedBraking.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
void Require(bool pass,const char* why)
{
    if(!pass){std::cerr<<"FourCornerWheelBrakes FAIL: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
latebrake::SuspensionResponse Springs(std::array<double,4> fractions)
{
    latebrake::SuspensionResponse r;
    r.Valid=true;r.WeightForce=1100.0*980.0;
    for(unsigned i=0;i<4;++i){r.Corners[i].Contact=fractions[i]>0.0;
        r.Corners[i].UpForce=fractions[i]*r.WeightForce;
        r.TotalUpForce+=r.Corners[i].UpForce;if(r.Corners[i].Contact)++r.ContactCount;}
    return r;
}
latebrake::ChassisGroundContactOutput Ground(const latebrake::SuspensionResponse& s)
{
    latebrake::ChassisGroundContactOutput g;g.Valid=true;g.SupportingProbes=s.ContactCount;
    return latebrake::ConstrainGroundContactToSuspension(g,s);
}
latebrake::FourCornerWheelBrakeSample Frame(double speed=3000.0,double dt=1.0/60.0)
{
    latebrake::FourCornerWheelBrakeSample s;s.DeltaSeconds=dt;s.MassKg=1100.0;
    s.PhysicsReady=true;s.ServiceBrake=true;s.RoadSpeedCmPerSecond.fill(speed);
    s.RoadNormalZ.fill(1.0);return s;
}
latebrake::SprungContactBrakeOutput Authorized(const latebrake::FourCornerWheelBrakeSample& f,
    const latebrake::ChassisGroundContactOutput& g,const latebrake::SuspensionResponse& s,double speed=3000.0)
{
    const auto request=latebrake::CalculateGripLimitedBraking({f.DeltaSeconds,speed,1100.0,
        f.ServiceBrake,f.MechanicalHandbrake,true,1.0,1.0,1.0,f.ServiceHeatScale});
    return latebrake::CalculateSprungContactBraking({1100.0,speed,
        request.ForwardForce*g.ContactTractionScale,f.MechanicalHandbrake&&!f.ServiceBrake,
        true,f.RoadSpeedCmPerSecond,f.PlanarForwardScales},g,s);
}
}
int main()
{
    using namespace latebrake;
    auto springs=Springs({0.25,0.25,0.25,0.25});auto ground=Ground(springs);
    auto frame=Frame();auto profile=BrakeProfileForVehicle(0,true);
    auto allowed=Authorized(frame,ground,springs);
    FourCornerWheelBrakes model;
    auto first=model.Propose(frame,profile,ground,springs,allowed);
    Require(first.Valid&&first.Receipt>0&&!model.LastCommitted().Valid&&model.HasUnsettledProposal(),
            "proposal is not advertised as delivered wheel work");
    for(const auto& w:first.Wheels)Require(w.Available&&w.NewlySeeded,"four real loaded corners seed independently");
    Require(!model.CommitDeliveredFrame(first.Receipt+1)&&model.HasUnsettledProposal(),"stale receipt cannot settle current work");
    Require(model.CommitDeliveredFrame(first.Receipt)&&model.LastCommitted().Valid&&
            !model.CommitDeliveredFrame(first.Receipt),"exact delivery commits once");
    auto next=model.Propose(frame,profile,ground,springs,allowed);
    Require(next.Valid&&!next.Wheels[0].NewlySeeded,"committed modeled rotation carries to next braking frame");
    Require(model.RejectUndeliveredFrame(next.Receipt)&&!model.LastCommitted().Valid&&
            !model.CommitDeliveredFrame(next.Receipt),"rejected force cannot become delivered rotor work");
    FourCornerWheelBrakes orphan,baseline;
    for(unsigned i=0;i<250;++i){const auto a=orphan.Propose(frame,profile,ground,springs,allowed);
        const auto b=baseline.Propose(frame,profile,ground,springs,allowed);
        Require(a.Valid&&b.Valid&&a.Wheels[0].NewlySeeded&&
                std::abs(a.Brakes.TotalForwardForce-b.Brakes.TotalForwardForce)<1.0e-8,
                "unanswered proposals cannot advance pressure or rotor state");
        baseline.RejectUndeliveredFrame(b.Receipt);}
    const auto oldReceipt=next.Receipt;model.Reset();
    next=model.Propose(frame,profile,ground,springs,allowed);
    Require(next.Valid&&next.Receipt>oldReceipt&&!model.CommitDeliveredFrame(oldReceipt),
            "reset never reuses old receipt identities");
    model.CommitDeliveredFrame(next.Receipt);
    auto bad=frame;bad.RoadSpeedCmPerSecond[3]=std::numeric_limits<double>::quiet_NaN();
    Require(!model.Propose(bad,profile,ground,springs,allowed).Valid&&!model.LastCommitted().Valid,
            "late bad corner rejects and clears entire model frame");
    next=model.Propose(frame,profile,ground,springs,allowed);
    Require(next.Valid&&next.Wheels[0].NewlySeeded,"invalid frame discards transient modeled wheel state");
    model.CommitDeliveredFrame(next.Receipt);
    auto changed=BrakeProfileForVehicle(5,false);
    next=model.Propose(frame,changed,ground,springs,allowed);
    Require(next.Valid&&next.Wheels[0].NewlySeeded,"garage profile changes reseed without old inertia");
    auto airborne=Springs({0.0,0.0,0.0,0.0});auto noGround=Ground(airborne);
    next=model.Propose(frame,profile,noGround,airborne,Authorized(frame,noGround,airborne));
    Require(next.Valid&&next.Receipt==0&&next.Brakes.TotalForwardForce==0.0,
            "airborne rotor model creates neither contact force nor delivery receipt");
    frame.ServiceBrake=false;frame.MechanicalHandbrake=true;
    auto frontOnly=Springs({0.45,0.45,0.0,0.0});auto frontGround=Ground(frontOnly);
    next=model.Propose(frame,profile,frontGround,frontOnly,Authorized(frame,frontGround,frontOnly));
    Require(next.Valid&&next.Brakes.TotalForwardForce==0.0,"mechanical handbrake never migrates to front support");
    next=model.Propose(frame,profile,ground,springs,Authorized(frame,ground,springs));
    Require(next.Valid&&next.Brakes.TangentForces[0]==0.0&&next.Brakes.TangentForces[1]==0.0&&
            next.Wheels[2].Pressure==1.0&&!next.Wheels[2].Modulating,
            "rear-only handbrake retains deliberate modeled locking without ABS");
    frame=Frame();frame.MechanicalHandbrake=true;
    next=model.Propose(frame,profile,ground,springs,Authorized(frame,ground,springs));
    Require(next.Valid&&next.Wheels[2].EffectiveTorquePressure==1.0,
            "service plus mechanical rear brake retains rear mechanical authority");
    frame=Frame();frame.ServiceHeatScale=0.05;
    for(auto& p:profile.Wheels)p.MaximumBrakeTorqueNm=50.0;
    next=model.Propose(frame,profile,ground,springs,Authorized(frame,ground,springs));
    Require(next.Valid&&next.Wheels[0].EffectiveTorquePressure<=0.05,
            "heat fade scales applied torque, not the validated profile domain");
    profile=BrakeProfileForVehicle(0,true);
    std::mt19937 rng(0x5b9802);std::uniform_real_distribution<double> u(0.0,1.0);
    for(unsigned n=0;n<12000;++n)
    {
        std::array<double,4> loads{};for(auto& v:loads)v=u(rng)<0.15?0.0:u(rng)*0.85;
        springs=Springs(loads);ground=Ground(springs);
        const double speed=(n%2==0?1.0:-1.0)*(100.0+u(rng)*9000.0);
        frame=Frame(speed,(n%3==0?1.0/30.0:n%3==1?1.0/60.0:1.0/120.0));
        frame.MechanicalHandbrake=n%7==0;frame.ServiceBrake=n%11!=0;
        frame.ServiceHeatScale=0.05+0.95*u(rng);frame.GripScale=0.05+3.95*u(rng);
        for(unsigned i=0;i<4;++i){frame.RoadNormalZ[i]=0.68+0.32*u(rng);
            frame.PlanarForwardScales[i]=frame.RoadNormalZ[i];
            frame.RoadSpeedCmPerSecond[i]=speed*(u(rng)*2.0-0.2);}
        allowed=Authorized(frame,ground,springs,speed);
        next=model.Propose(frame,BrakeProfileForVehicle(static_cast<int>(n%6),n%2==0),ground,springs,allowed);
        Require(next.Valid&&next.InternalSteps<=FourCornerWheelBrakes::MaximumInternalSteps&&
                next.SolverIterations<=4*next.InternalSteps*WheelBrakeDynamics::RootIterations,
                "mixed-load frame stays valid with deterministic bounded solve work");
        Require(next.Brakes.TotalBrakeForceMagnitude<=allowed.TotalBrakeForceMagnitude+1.0e-6,
                "modeled tires cannot exceed old body-force admission");
        for(unsigned i=0;i<4;++i)Require(
            next.Brakes.TangentForces[i]*frame.RoadSpeedCmPerSecond[i]<=0.0&&
            std::abs(next.Brakes.TangentForces[i])<=std::abs(allowed.TangentForces[i])+1.0e-6,
            "every accepted tangent force is bounded and locally passive");
        if(next.Receipt!=0){if(n%5==0)model.RejectUndeliveredFrame(next.Receipt);
            else Require(model.CommitDeliveredFrame(next.Receipt),"accepted physical plan commits");}
    }
    springs=Springs({0.25,0.25,0.25,0.25});ground=Ground(springs);
    for(int vehicle=0;vehicle<6;++vehicle)
    {
    double referenceDistance=0.0;
    for(double fps:{120.0,60.0,30.0})
    {
        model.Reset();double speed=4000.0,distance=0.0;bool sawModulation=false;
        for(unsigned n=0;n<static_cast<unsigned>(fps*20.0);++n)
        {
            frame=Frame(speed,1.0/fps);allowed=Authorized(frame,ground,springs,speed);
            next=model.Propose(frame,BrakeProfileForVehicle(vehicle,true),ground,springs,allowed);
            Require(next.Valid,"continuous braking frame accepted");
            const double acceleration=next.Brakes.TotalForwardForce/1100.0;
            distance+=speed/fps+0.5*acceleration/(fps*fps);speed+=acceleration/fps;
            for(const auto& w:next.Wheels)sawModulation=sawModulation||w.Modulating;
            if(next.Receipt)model.CommitDeliveredFrame(next.Receipt);
            Require(speed>=-1.0e-8,"reduced admitted brake never reverses chassis travel");
            if(speed<0.001)break;
        }
        std::cout<<"synthetic braking vehicle="<<vehicle<<" "<<fps<<" FPS distance_cm="<<distance<<" end_speed="<<speed<<'\n';
        Require(speed<0.01&&sawModulation&&distance>4000.0*4000.0/1800.0-0.1&&distance<30000.0,
                "candidate rotor/ABS path stops in bounded synthetic distance");
        if(fps==120.0)referenceDistance=distance;
        else Require(std::abs(distance-referenceDistance)<referenceDistance*0.04,
                     "30/60/120 FPS synthetic stops converge within 4 percent");
    }
    }
    std::cout<<"Four-corner brakes: receipts, profiles, handbrake, heat, 12000 mixed frames and FPS stops passed\n";
}
