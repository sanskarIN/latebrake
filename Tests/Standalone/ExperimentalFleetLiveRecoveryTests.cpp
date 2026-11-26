#include "AI/ExperimentalFleetLiveRecovery.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Require(bool value,const char* message)
{
    if (!value) {std::cerr<<message<<'\n';std::exit(EXIT_FAILURE);}
}

FleetPhysicalFrame Frame()
{
    FleetPhysicalFrame frame;
    frame.AttemptToken=1;frame.FrameToken=1;frame.Count=StartGridCapacity;
    for(int slot=0;slot<StartGridCapacity;++slot)
        frame.Cars[static_cast<std::size_t>(slot)]={true,slot,
            static_cast<std::uint64_t>(slot+1),{{-10000.0-slot*1000.0,10000.0},0.0},0.0,0.0};
    return frame;
}
}
int main()
{
    using namespace latebrake;
    const auto original=Frame();
    const auto ids=InspectFleetRoster(original).Tokens;
    Require(MatchLiveFleetIdentity(original,ids),"fresh roster matches bound tokens");
    for(int slot=0;slot<StartGridCapacity;++slot)
    {
        std::array<RecoveryObstacle,MaxRecoveryObstacles> blockers{};
        int count=0;
        Require(BuildLiveFleetBlockers(original,ids,slot,ids[slot],blockers,count),
            "exact subject excluded");
        Require(count==5,"five blockers remain");
        for(int i=0;i<count;++i)
            Require(blockers[i].Slot!=slot && blockers[i].ActorToken!=ids[slot],
                "subject cannot be its own blocker");
        auto replacement=original;
        replacement.Cars[slot].ActorToken=100;
        Require(!BuildLiveFleetBlockers(replacement,ids,slot,ids[slot],blockers,count)&&count==0,
            "replacement invalidates fresh preflight");
        auto missing=original;missing.Cars[slot].Verified=false;
        Require(!MatchLiveFleetIdentity(missing,ids),"missing car invalidates fresh preflight");
        auto duplicate=original;duplicate.Cars[slot].ActorToken=ids[(slot+1)%6];
        Require(!MatchLiveFleetIdentity(duplicate,ids),"duplicate identity rejected");
    }
    for(int gate=0;gate<CircuitGateCount;++gate)
    {
        auto clear=SelectFleetPlayerRecovery(gate,original,ids,ids[0]);
        Require(clear.Safe(),"unoccupied original player landing");
        auto blocked=original;blocked.Cars[5].Pose=PlayerRecovery(gate);
        Require(SelectFleetPlayerRecovery(gate,blocked,ids,ids[0]).Status==
            PlayerRecoveryClearanceStatus::Occupied,"fifth rival blocks actual player landing");
        for(int slot=1;slot<StartGridCapacity;++slot)
        {
            auto chosen=SelectFleetRivalRecovery(gate,slot,ids[slot],original,ids);
            Require(chosen.Clear && chosen.Evaluated<=5,"clear fleet rival candidate");
            Require(RecoveryChassisFitsPhysicalAsphalt(chosen.Pose),"candidate lies on physical road");
            const auto primary=RivalRecovery(gate);
            if (gate==0||gate==2) Require(chosen.Pose.Position.X==primary.Position.X,
                "horizontal longitudinal progress unchanged");
            else Require(chosen.Pose.Position.Y==primary.Position.Y,
                "vertical longitudinal progress unchanged");
            auto occupied=original;
            int index=0;
            for(int other=0;other<StartGridCapacity;++other)
            {
                if(other==slot)continue;
                CircuitPose pose{};
                TrySingleRivalRecoveryPose(gate,SingleRivalRecoveryOffsetsCm[index++],pose);
                occupied.Cars[other].Pose=pose;
            }
            Require(!SelectFleetRivalRecovery(gate,slot,ids[slot],occupied,ids).Clear,
                "all five candidates occupied: no teleport");
        }
    }
    RivalFleetTrafficSample traffic{};
    Require(MakeFreshFleetTraffic(original,ids,3,ids[3],true,0.5,traffic) &&
        traffic.BlockerCount==5,"current traffic qualified");
    auto moved=original;moved.Cars[1].Pose.Position.X+=700.0;
    Require(MakeFreshFleetTraffic(moved,ids,3,ids[3],true,0.5,traffic) &&
        traffic.Blockers[1].Pose.Position.X==moved.Cars[1].Pose.Position.X,
        "traffic reads current position instead of retained grid snapshot");
    Require(!MakeFreshFleetTraffic(moved,ids,0,ids[0],true,0.5,traffic)&&
        traffic.BlockerCount==0,"human cannot request AI traffic");
    auto inactive=original;inactive.Inactive=true;
    Require(!MakeFreshFleetTraffic(inactive,ids,1,ids[1],true,0.5,traffic),
        "inactive frame denies traffic");
    Require(!MakeFreshFleetTraffic(original,ids,1,ids[1],true,
        std::numeric_limits<double>::quiet_NaN(),traffic),"invalid brake evidence");
    std::cout<<"Fresh fleet recovery and traffic identity checks passed\n";
}
