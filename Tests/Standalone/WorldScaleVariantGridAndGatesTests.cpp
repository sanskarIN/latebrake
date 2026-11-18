#include "Track/WorldScaleVariantGridAndGates.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition,const char* message)
{
    if(!condition)
    {
        std::cerr<<"WorldScaleVariantGridAndGates FAIL: "
                 <<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

void SixCarsAndFourGates(
    const latebrake::WorldScaleVariantRecipe& track)
{
    using namespace latebrake;
    const auto grid=BuildWorldScaleVariantGrid(track);
    Require(grid.Valid &&
            grid.SourceTrack==track.Id &&
            grid.OriginalPhysicalRoadUnchanged &&
            grid.OfficialRaceGridUnchanged &&
            grid.MinimumObservedCenterGapCm>0.0 &&
            grid.MinimumObservedBodyClearanceCm>=55.0,
            "six distinct source grid slots must have road-supported separated chassis");

    for(unsigned slot=0;slot<WorldScaleVariantGridCars;++slot)
    {
        const auto& car=grid.Cars[slot];
        Require(car.GridSlot==static_cast<int>(slot) &&
                car.SourceToken!=0 &&
                car.AllBodyCornersOnRoad &&
                car.UnwrappedStationCm<0.0 &&
                WorldScaleVariantBodyOnRoad(
                    track,car.Pose,
                    WorldScaleVariantGridHalfLengthCm,
                    WorldScaleVariantGridHalfWidthCm),
                "six 3x2 source grid footprints are fully on authored 9m asphalt");
        for(unsigned previous=0;previous<slot;++previous)
        {
            const auto& other=grid.Cars[previous];
            Require(car.SourceToken!=other.SourceToken &&
                    WorldScaleVariantGridBodyClearance(
                        car.Pose,other.Pose,
                        WorldScaleVariantGridHalfLengthCm,
                        WorldScaleVariantGridHalfWidthCm)>=55.0,
                    "physical chassis footprints cannot overlap in original source slots");
        }
    }

    const auto gates=BuildWorldScaleVariantGates(track);
    Require(gates.Valid && gates.AdvisoryOnly &&
            gates.DoesNotIssueOfficialLapCredit &&
            gates.SourceTrack==track.Id,
            "original multi-track gates are never granted race authority");

    for(unsigned i=0;i<WorldScaleVariantGateCount;++i)
    {
        const auto& gate=gates.Gates[i];
        Require(gate.GateIndex==i &&
                gate.WholeGateOnAuthoredRoad &&
                std::abs(gate.StationCm -
                    track.LapLengthCm*i/WorldScaleVariantGateCount)
                    <=1e-6 &&
                IsOnWorldScaleVariantRoad(track,gate.Center.Position) &&
                IsOnWorldScaleVariantRoad(track,gate.LeftExtent) &&
                IsOnWorldScaleVariantRoad(track,gate.RightExtent),
                "all four sector planes lie inside the actual variant road");

        const auto before=WorldScaleVariantCenterlineAt(
            track,gate.StationCm-75.0);
        const auto after=WorldScaleVariantCenterlineAt(
            track,gate.StationCm+75.0);
        Require(before.Valid && after.Valid,
                "gate has source-grounded signed upstream/downstream stations");
        WorldScaleVariantGateObservation observation{};
        observation.Previous=before.Pose.Position;
        observation.Current=after.Pose.Position;
        observation.DeltaSeconds=0.05;
        observation.MaximumObservedSpeedCmps=4000.0;
        observation.PhysicalActorToken=1234;
        observation.ExpectedGate=i;
        observation.OriginalRoadContactVerified=true;
        observation.ActiveRaceVerified=true;

        const auto valid=ObserveWorldScaleVariantGate(
            track,gates,observation);
        Require(valid.ValidSource && valid.CrossedForward &&
                valid.PreviousSignedDistanceCm<0.0 &&
                valid.CurrentSignedDistanceCm>0.0 &&
                std::abs(valid.CrossingLateralOffsetCm)<
                    WorldScaleVariantGateHalfSpanCm &&
                !valid.OfficialLapAwarded,
                "real signed XY crossing is evidence, NOT official lap credit");
        std::swap(observation.Previous,observation.Current);
        const auto reverse=ObserveWorldScaleVariantGate(
            track,gates,observation);
        Require(reverse.ValidSource && !reverse.CrossedForward &&
                !reverse.OfficialLapAwarded,
                "reverse crossing cannot create an authorized forward gate");
        observation.Previous=after.Pose.Position;
        observation.Current=after.Pose.Position;
        const auto stopped=ObserveWorldScaleVariantGate(
            track,gates,observation);
        Require(stopped.ValidSource && !stopped.CrossedForward,
                "stationary car in a gate never receives progress");

        observation.Previous=before.Pose.Position;
        observation.Current=after.Pose.Position;
        observation.MaximumObservedSpeedCmps=200.0;
        Require(!ObserveWorldScaleVariantGate(
                    track,gates,observation).ValidSource,
                "claimed slow physical velocity cannot conceal large teleport");

        observation.MaximumObservedSpeedCmps=4000.0;
        observation.OriginalRoadContactVerified=false;
        Require(!ObserveWorldScaleVariantGate(
                    track,gates,observation).ValidSource,
                "unqualified physics-road contact cannot pass sector crossing");

        observation.OriginalRoadContactVerified=true;
        observation.ExpectedGate=WorldScaleVariantGateCount;
        Require(!ObserveWorldScaleVariantGate(
                    track,gates,observation).ValidSource,
                "out-of-range sector index never becomes a valid race checkpoint");
    }
}

void RejectedInvalidProposals()
{
    using namespace latebrake;
    const auto track=WorldScaleVariantAt(0);
    auto settings=WorldScaleVariantGridSettings{};
    settings.LateralSlotCm=100.0;
    Require(!BuildWorldScaleVariantGrid(track,settings).Valid,
            "cars too close laterally cannot enter a 6-car grid");
    settings={};
    settings.RowSpacingCm=350.0;
    Require(!BuildWorldScaleVariantGrid(track,settings).Valid,
            "rows too close longitudinally cannot pass safety preflight");
    settings={};
    settings.NoActiveUnrealSpawns=false;
    Require(!BuildWorldScaleVariantGrid(track,settings).Valid,
            "source planner is not permitted to create Unreal actors");
    settings={};
    settings.VehicleHalfWidthCm=300.0;
    Require(!BuildWorldScaleVariantGrid(track,settings).Valid,
            "too-wide chassis never receives fabricated physical pavement");
    settings={};
    settings.MinimumBodyGapCm=
        std::numeric_limits<double>::infinity();
    Require(!BuildWorldScaleVariantGrid(track,settings).Valid,
            "unbounded safety radius fails closed");

    auto empty=WorldScaleVariantRecipe{};
    empty.LapLengthCm=0.0;
    Require(!BuildWorldScaleVariantGrid(empty).Valid &&
            !BuildWorldScaleVariantGates(empty).Valid,
            "invalid track cannot build fictional grid or gates");
    const auto gates=BuildWorldScaleVariantGates(track);
    Require(gates.Valid,"reference gates exist");
    WorldScaleVariantGateObservation observation{};
    observation.DeltaSeconds=0.05;
    observation.MaximumObservedSpeedCmps=4000.0;
    observation.PhysicalActorToken=12;
    observation.OriginalRoadContactVerified=true;
    observation.ActiveRaceVerified=true;
    observation.Previous=
        WorldScaleVariantCenterlineAt(track,-40.0).Pose.Position;
    observation.Current=
        WorldScaleVariantCenterlineAt(track,40.0).Pose.Position;
    observation.DeltaSeconds=
        std::numeric_limits<double>::quiet_NaN();
    Require(!ObserveWorldScaleVariantGate(
                track,gates,observation).ValidSource,
            "nonfinite tick cannot create a gate pass");
    observation.DeltaSeconds=0.05;
    observation.PhysicalActorToken=0;
    Require(!ObserveWorldScaleVariantGate(
                track,gates,observation).ValidSource,
            "no synthetic actor identity may fake a checkpoint");
}
} // namespace

int main()
{
    for(unsigned id=0;id<latebrake::WorldScaleVariantCount;++id)
    {
        SixCarsAndFourGates(latebrake::WorldScaleVariantAt(id));
    }
    RejectedInvalidProposals();
    std::cout<<"WorldScaleVariantGridAndGates PASS: six "
                 "oriented nonoverlapping source chassis, "
                 "four ordered road-supported gate planes per track, "
                 "reverse/stationary/teleport safety on all six maps; "
                 "no physical UE actors, no official lap authority\n";
    return EXIT_SUCCESS;
}
