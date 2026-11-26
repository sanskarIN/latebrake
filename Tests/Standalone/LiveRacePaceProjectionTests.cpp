#include "Racing/LiveRacePaceProjection.h"
#include "Racing/GateAwareRacePace.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <limits>

namespace
{
void Require(bool accepted, const char* label)
{
    if (!accepted)
    {
        std::cerr << "Live race pace FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void CompleteLap(latebrake::RaceProgress& race, double seconds)
{
    for (int checkpoint = 0; checkpoint < race.GetCheckpointCount(); ++checkpoint)
    {
        race.Advance(seconds / static_cast<double>(race.GetCheckpointCount()));
        Require(race.PassCheckpoint(checkpoint), "ordered checkpoint pass");
    }
}
bool Near(double a,double b)
{
    return std::abs(a-b)<1e-7;
}

void AcceptGate(latebrake::RaceProgress& race,double time,double fee=0.0)
{
    race.Advance(time);
    if (fee>0.0) Require(race.AddTimePenalty(fee),"charge actual penalty");
    Require(race.PassCheckpoint(race.GetNextCheckpoint()),"ordered measured gate");
}

void GatePaceUsesReferenceAndChargedClock()
{
    using namespace latebrake;
    RaceProgress race(4,3);
    Require(ProjectGateAwareRacePace(race).Status==GatePaceStatus::Unavailable,
        "no reference before first measured lap");
    for(double seconds:{2.0,4.0,6.0,8.0}) AcceptGate(race,seconds);
    auto p=ProjectGateAwareRacePace(race);
    Require(p.Status==GatePaceStatus::Estimated && p.ReferenceLapNumber==1 &&
        p.CurrentLapNumber==2 && p.NextGateIndex==0 && p.LapsRemaining==2 &&
        Near(p.EstimatedFinishSeconds,60.0) &&
        Near(p.RemainingCurrentSectorSeconds,2.0) &&
        Near(p.RemainingLaterSectorsSeconds,18.0) &&
        Near(p.RemainingLaterLapsSeconds,20.0),"entire reference duration at lap start");
    race.Advance(1.0);
    Require(race.AddTimePenalty(3.0),"current fee accepted");
    p=ProjectGateAwareRacePace(race);
    Require(p.Status==GatePaceStatus::Estimated && Near(p.CurrentSectorClockSeconds,4.0) &&
        Near(p.CurrentSectorFeeSeconds,3.0) &&
        Near(p.CurrentSectorDrivingClockSeconds,1.0) &&
        Near(p.EstimatedFinishSeconds,63.0),"fee already charged never replaces driving");
    Require(race.PassCheckpoint(0),"first gate completed early");
    p=ProjectGateAwareRacePace(race);
    Require(p.Status==GatePaceStatus::Estimated && p.NextGateIndex==1 &&
        Near(p.CurrentSectorFeeSeconds,0.0) && Near(p.EstimatedFinishSeconds,62.0),
        "accepted gate updates estimate using actual early progress");
    race.Advance(4.0);
    p=ProjectGateAwareRacePace(race);
    Require(p.Status==GatePaceStatus::CurrentSectorOverdue &&
        p.EstimatedFinishSeconds==0.0 && Near(p.CurrentSectorOverdueSeconds,0.0),
        "unfinished gate at reference deadline never claims zero remaining");
    race.Advance(2.0);
    p=ProjectGateAwareRacePace(race);
    Require(p.Status==GatePaceStatus::CurrentSectorOverdue &&
        Near(p.CurrentSectorOverdueSeconds,2.0),"overdue sector gives no finish estimate");
    Require(race.PassCheckpoint(1),"late physical gate restores next reference");
    p=ProjectGateAwareRacePace(race);
    Require(p.Status==GatePaceStatus::Estimated && p.NextGateIndex==2 &&
        Near(p.EstimatedFinishSeconds,64.0),"next gate reconciles actual late progress");
    AcceptGate(race,6.0);
    AcceptGate(race,8.0);
    p=ProjectGateAwareRacePace(race);
    Require(p.Status==GatePaceStatus::Estimated && p.ReferenceLapNumber==1 &&
        p.CurrentLapNumber==3 && p.LapsRemaining==1 &&
        p.RemainingLaterLapsSeconds==0.0 && Near(p.EstimatedFinishSeconds,64.0),
        "penalized lap cannot replace the fee-free reference");
    for(double seconds:{2.0,4.0,6.0,8.0}) AcceptGate(race,seconds);
    Require(race.IsFinished() &&
        ProjectGateAwareRacePace(race).Status==GatePaceStatus::Unavailable,
        "official finish clears projection");
    race.Reset();
    Require(ProjectGateAwareRacePace(race).Status==GatePaceStatus::Unavailable,
        "restart removes exact reference");
}

void GatePaceRejectsUnqualifiedPrefixes()
{
    using namespace latebrake;
    RaceProgress race(2,5);
    Require(race.AddTimePenalty(3.0),"charge fee without driving");
    Require(race.PassCheckpoint(0),"fee-only gate accepted by race model");
    AcceptGate(race,5.0);
    Require(ProjectGateAwareRacePace(race).Status==GatePaceStatus::Unavailable,
        "fee-only sector cannot become measured reference");
    AcceptGate(race,3.0); AcceptGate(race,5.0);
    Require(ProjectGateAwareRacePace(race).Status==GatePaceStatus::Estimated,
        "next complete active lap establishes reference");
    Require(race.PassCheckpoint(0),"zero-clock sector accepted by race model");
    Require(ProjectGateAwareRacePace(race).Status==GatePaceStatus::Unavailable,
        "invalid current prefix cannot produce reference projection");
    AcceptGate(race,8.0);
    Require(ProjectGateAwareRacePace(race).Status==GatePaceStatus::Estimated,
        "following lap can recover independent projection evidence");

    RaceProgress overflow(1,3);
    AcceptGate(overflow,300000.0);
    Require(ProjectGateAwareRacePace(overflow).Status==GatePaceStatus::Unavailable,
        "projection outside supported clock range rejected");
    RaceProgress extreme(1,std::numeric_limits<int>::max());
    AcceptGate(extreme,2.0);
    Require(ProjectGateAwareRacePace(extreme).Status==GatePaceStatus::Unavailable,
        "large required lap count cannot overflow bounded finish clock");
}

void GatePaceAllCheckpointCounts()
{
    using namespace latebrake;
    for(int gates=1;gates<=MeasuredLapMaxGates;++gates)
    {
        RaceProgress race(gates,3);
        double reference=0.0;
        for(int i=0;i<gates;++i)
        {
            const double seconds=2.0+static_cast<double>(i%5);
            AcceptGate(race,seconds);
            reference+=seconds;
        }
        double fees=0.0;
        for(int i=0;i<gates;++i)
        {
            const double seconds=2.0+static_cast<double>(i%5);
            race.Advance(seconds*0.5);
            Require(race.AddTimePenalty(0.5),"exact half-second fee");
            fees+=0.5;
            const auto p=ProjectGateAwareRacePace(race);
            Require(p.Status==GatePaceStatus::Estimated && p.NextGateIndex==i &&
                Near(p.CurrentSectorDrivingClockSeconds,seconds*0.5) &&
                Near(p.CurrentSectorFeeSeconds,0.5) &&
                Near(p.EstimatedFinishSeconds,3.0*reference+fees),
                "all supported gate capacities use exact gate and fee evidence");
            AcceptGate(race,seconds*0.5);
        }
        const auto last=ProjectGateAwareRacePace(race);
        Require(last.Status==GatePaceStatus::Estimated && last.NextGateIndex==0 &&
            Near(last.EstimatedFinishSeconds,3.0*reference+fees),
            "full paid lap keeps fee-free reference for final lap");
    }
}
}

int main()
{
    using namespace latebrake;
    RaceProgress race(4, 3);
    Require(!ProjectLiveRacePace(race).Valid,
            "no pace is claimed before finishing a real measured lap");

    CompleteLap(race, 60.0);
    const LiveRacePaceProjection first = ProjectLiveRacePace(race);
    Require(first.Valid && first.SampledLaps == 1 && first.LapsRemaining == 2 &&
            first.ReferenceLapSeconds == 60.0 &&
            first.ProjectedFinishSeconds == 180.0,
            "one timed lap gives stable remaining-laps estimate");
    race.Advance(10.0);
    Require(ProjectLiveRacePace(race).ProjectedFinishSeconds == 180.0,
            "time spent during current lap reduces its remaining reference");

    race.Advance(100.0); // Current lap now exceeds the previous lap pace.
    const auto slow = ProjectLiveRacePace(race);
    Require(slow.Valid && slow.ProjectedFinishSeconds >=
            race.GetElapsedSeconds(),
            "estimate never goes back before the current race clock");
    Require(slow.ProjectedFinishSeconds == 230.0,
            "slow current lap does not claim negative remaining time");

    // Complete the second lap at 170 seconds total (110-second lap).
    for (int checkpoint = 0; checkpoint < 4; ++checkpoint)
    {
        Require(race.PassCheckpoint(checkpoint), "second lap gate accepts");
    }
    const auto two = ProjectLiveRacePace(race);
    Require(two.Valid && two.SampledLaps == 2 &&
            two.ReferenceLapSeconds == 85.0 &&
            two.ProjectedFinishSeconds == 255.0,
            "two observed laps average without leaking an old session");
    race.Advance(70.0);
    for (int checkpoint = 0; checkpoint < 4; ++checkpoint)
    {
        Require(race.PassCheckpoint(checkpoint), "final lap gate accepts");
    }
    Require(race.IsFinished() && !ProjectLiveRacePace(race).Valid,
            "never claim a projected time after authoritative finish");
    race.Reset();
    Require(!ProjectLiveRacePace(race).Valid,
            "restart clears old pace samples and predictions");

    RaceProgress zero(4, 2);
    for (int checkpoint = 0; checkpoint < 4; ++checkpoint)
    {
        Require(zero.PassCheckpoint(checkpoint), "zero-time test ordered gate");
    }
    Require(zero.GetTimedLapCount() == 0 && !ProjectLiveRacePace(zero).Valid,
            "zero-duration laps cannot seed a false projection");

    RaceProgress multi(4, 7);
    CompleteLap(multi, 10.0);
    CompleteLap(multi, 20.0);
    CompleteLap(multi, 30.0);
    CompleteLap(multi, 40.0);
    const auto bounded = ProjectLiveRacePace(multi);
    Require(bounded.Valid && bounded.SampledLaps == 3 &&
            bounded.ReferenceLapSeconds == 30.0 &&
            bounded.ProjectedFinishSeconds == 190.0,
            "rolling reference uses only three most recent timed laps");

    // A recovery penalty can advance the official clock and create a
    // positive timed lap without ANY driven time. It must not seed the
    // read-only HUD's displayed estimated finish pace. The rolling
    // three-lap window may recover once all sampled laps are authentic.
    RaceProgress penaltyOnly(4, 6);
    Require(penaltyOnly.AddTimePenalty(5.0),
            "recovery-only lap legitimately charges the race clock");
    for (int gate = 0; gate < 4; ++gate)
    {
        Require(penaltyOnly.PassCheckpoint(gate),
                "penalty-only lap completes original ordered gates");
    }
    const auto unDriven = penaltyOnly.GetRecentTimedLap(0);
    Require(unDriven.Valid() && !unDriven.ActiveClockEvidence &&
            penaltyOnly.GetActiveTimedLapCount() == 0 &&
            !ProjectLiveRacePace(penaltyOnly).Valid,
            "positive penalty-only duration cannot invent a driven pace");
    CompleteLap(penaltyOnly, 60.0);
    Require(penaltyOnly.GetRecentTimedLap(0).ActiveClockEvidence &&
            !ProjectLiveRacePace(penaltyOnly).Valid,
            "one good lap cannot hide older unqualified sampled lap");
    CompleteLap(penaltyOnly, 70.0);
    Require(!ProjectLiveRacePace(penaltyOnly).Valid,
            "two good laps cannot hide unqualified third sampled lap");
    CompleteLap(penaltyOnly, 80.0);
    const auto recovered = ProjectLiveRacePace(penaltyOnly);
    Require(recovered.Valid && recovered.SampledLaps == 3 &&
            recovered.ReferenceLapSeconds == 70.0 &&
            recovered.ProjectedFinishSeconds == 355.0,
            "three consecutive driven laps restore safe live HUD pace");

    RaceProgress missingLap(4, 6);
    CompleteLap(missingLap, 50.0);
    for (int gate = 0; gate < 4; ++gate)
    {
        Require(missingLap.PassCheckpoint(gate),
                "zero-duration lap cannot become recent pace sample");
    }
    CompleteLap(missingLap, 60.0);
    Require(!ProjectLiveRacePace(missingLap).Valid,
            "zero-time gap cannot silently join older driven history");
    CompleteLap(missingLap, 70.0);
    Require(!ProjectLiveRacePace(missingLap).Valid,
            "stale older physical lap cannot bridge the gap");
    CompleteLap(missingLap, 80.0);
    Require(ProjectLiveRacePace(missingLap).Valid,
            "new contiguous three-lap window restores projection");

    RaceProgress overflow(4, 2);
    CompleteLap(overflow, 604800.0);
    Require(!ProjectLiveRacePace(overflow).Valid,
            "unrealistic projection beyond record clock bound is hidden");

    GatePaceUsesReferenceAndChargedClock();
    GatePaceRejectsUnqualifiedPrefixes();
    GatePaceAllCheckpointCounts();
    std::cout << "Live race pace projection and gate-aware reference checks passed.\n";
    return EXIT_SUCCESS;
}
