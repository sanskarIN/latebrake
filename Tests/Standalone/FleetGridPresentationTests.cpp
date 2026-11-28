#include "AI/FleetGridPresentation.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Require(bool condition,const char* message)
{
    if(!condition){std::cerr<<message<<'\n';std::exit(EXIT_FAILURE);}
}

FleetGridSettleSample Good(int slot)
{
    return {true,slot,StartGridPoses[static_cast<std::size_t>(slot)],
        180.0,0.0,0.0,-200.0,1.0/60.0};
}

void Settling()
{
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    for(int slot=0;slot<StartGridCapacity;++slot)
    {
        const auto sample=Good(slot);
        const auto settled=PlanFleetGridSettling(sample);
        Require(settled.PreserveDownward && settled.VerticalVelocityCmS==-200.0,
            "only existing downward component preserved");
        auto invalid=sample;invalid.SessionEligible=false;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"blocked session remains zero");
        invalid=sample;invalid.VelocityZ=0.0;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"no invented descent");
        invalid=sample;invalid.VelocityZ=20.0;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"upward bounce not retained");
        invalid=sample;invalid.VelocityZ=-FleetMaximumSettleDownSpeedCmS-0.01;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"excessive descent denied");
        invalid=sample;invalid.HeightCm=FleetMaximumSettleHeightCm+1.0;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"implausible spawn height denied");
        invalid=sample;invalid.HeightCm=LiveGridExpectedChassisCenterZCm-6.0;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"below road height denied");
        invalid=sample;invalid.Pose.Position.X+=LiveGridMaxPoseErrorCm+1.0;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"displaced body denied");
        invalid=sample;invalid.Pose.HeadingDegrees+=LiveGridMaxHeadingErrorDegrees+1.0;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"yaw mismatch denied");
        invalid=sample;invalid.Pose.HeadingDegrees+=360.0;
        Require(PlanFleetGridSettling(invalid).PreserveDownward,"equivalent heading accepted");
        invalid=sample;invalid.VelocityX=251.0;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"moving body denied");
        invalid=sample;invalid.VelocityX=nan;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"invalid planar velocity denied");
        invalid=sample;invalid.VelocityZ=inf;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"invalid vertical velocity denied");
        invalid=sample;invalid.HeightCm=nan;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"invalid height denied");
        invalid=sample;invalid.DeltaSeconds=0.11;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"hitch cannot retain descent");
        invalid=sample;invalid.DeltaSeconds=0.0;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"zero time denied");
        invalid=sample;invalid.Slot=-1;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"anonymous body denied");
        invalid=sample;invalid.Slot=StartGridCapacity;
        Require(!PlanFleetGridSettling(invalid).PreserveDownward,"out-of-range slot denied");
        for(double dt:{1.0/30.0,1.0/60.0,1.0/120.0})
        {
            for(double vertical:{-1.0,-25.0,-500.0,-800.0})
            {
                auto v=sample;v.DeltaSeconds=dt;v.VelocityZ=vertical;
                const auto result=PlanFleetGridSettling(v);
                Require(result.PreserveDownward && result.VerticalVelocityCmS==vertical,
                    "no frame-rate scaling or artificial vertical impulse");
            }
        }
    }
}
void VisualRoster()
{
    for(int vehicle=0;vehicle<VehicleCount;++vehicle)
    for(int paint=0;paint<GaragePaintOptions;++paint)
    for(int stripe=0;stripe<GarageStripeOptions;++stripe)
    for(int wheel=0;wheel<GarageWheelOptions;++wheel)
    {
        const GarageLook look{static_cast<std::uint8_t>(paint),
            static_cast<std::uint8_t>(stripe),static_cast<std::uint8_t>(wheel)};
        const auto plan=BuildFleetVisualPlan(vehicle,look);
        Require(ValidFleetVisualPlan(plan),"all legal human selections yield unique roster");
        Require(plan.Cars[0].VehicleId==vehicle && SameGarageLook(plan.Cars[0].Look,look),
            "player choice is never modified");
        unsigned vehicles=0,paints=0;
        for(const auto& entry:plan.Cars)
        {
            vehicles|=1u<<static_cast<unsigned>(entry.VehicleId);
            paints|=1u<<static_cast<unsigned>(entry.Look.Paint);
        }
        Require(vehicles==63u,"each six-car body ID appears exactly once");
        unsigned paintCount=0;
        for(unsigned bit=0;bit<GaragePaintOptions;++bit)
            if((paints&(1u<<bit))!=0u)++paintCount;
        Require(paintCount==6,"all six colors are distinct");
        auto bad=plan;bad.Cars[5].Look.Paint=bad.Cars[0].Look.Paint;
        Require(!ValidFleetVisualPlan(bad),"duplicate paint denied");
        bad=plan;bad.Cars[5].VehicleId=bad.Cars[1].VehicleId;
        Require(!ValidFleetVisualPlan(bad),"duplicate body denied");
        bad=plan;bad.Cars[5].Slot=4;
        Require(!ValidFleetVisualPlan(bad),"wrong slot denied");
    }
    Require(!BuildFleetVisualPlan(-1,{}).Valid,"invalid selected car denied");
    Require(!BuildFleetVisualPlan(0,{8,0,0}).Valid,"invalid paint denied");
    Require(!BuildFleetVisualPlan(0,{0,3,0}).Valid,"invalid stripe denied");
    Require(!BuildFleetVisualPlan(0,{0,0,3}).Valid,"invalid wheels denied");
}
}
int main()
{
    Settling();VisualRoster();
    std::cout<<"Fleet pre-GO settling and all 432 legal visual rosters passed\n";
}
