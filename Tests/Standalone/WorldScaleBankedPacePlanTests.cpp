#include "Track/WorldScaleBankedPacePlan.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool yes,const char* what)
{
    if(!yes)
    {
        std::cerr<<"WorldScaleBankedPace FAIL: "<<what<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1.0e-6)
{
    return std::abs(a-b)<=tolerance;
}
}

int main()
{
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    Require(!ValidBankedPaceSettings({
                .NominalGripG=nan}),
            "nonfinite friction input rejected");
    Require(!ValidBankedPaceSettings({
                .GripScale=0.1}),
            "unqualified very-low grip rejected");
    Require(!ValidBankedPaceSettings({
                .MaximumSpeedCmps=20000.0}),
            "unbounded speed ceiling rejected");
    Require(!ValidBankedPaceSettings({
                .LateralBudgetFraction=0.99}),
            "cannot consume the entire braking friction circle");
    Require(!ValidBankedPaceSettings({
                .NoUnrealActuation=false}),
            "portable planner cannot claim gameplay authority");
    Require(!ValidBankedPaceSettings({
                .NoOfficialLapCredit=false}),
            "portable planner cannot award official timing");
    Require(!ValidBankedPaceSettings({
                .UseOriginalSourceRoad=false}),
            "unverified replacement geometry must be rejected");
    unsigned verified=0u;
    for(unsigned index=0u;index<WorldScaleVariantCount;++index)
    {
        const auto circuit=WorldScaleOriginalVariantRecipes[index];
        WorldScaleBankedRoadProfile profile;
        Require(ChooseWorldScaleBankedRoadProfile(
                    circuit.Id,profile),
                "select independent original bank/grade recipe");
        BankedPaceSettings dry;
        BankedPaceSettings wet;
        wet.GripScale=0.5;
        const auto full=BuildWorldScaleBankedPace(
            circuit,profile,dry);
        const auto rain=BuildWorldScaleBankedPace(
            circuit,profile,wet);
        Require(full.Valid && rain.Valid,
                "all six original source circuits have feasible pace plans");
        Require(full.Circuit==circuit.Id &&
                rain.Circuit==circuit.Id &&
                full.AllNodesOnOriginalRoad &&
                rain.AllNodesOnOriginalRoad &&
                full.OfficialTimingUnchanged &&
                rain.OfficialTimingUnchanged &&
                full.SourceOnly && rain.SourceOnly,
                "source provenance and no gameplay side effects");
        Require(full.RelaxationPasses>0u &&
                rain.RelaxationPasses>0u &&
                full.RelaxationPasses<=BankedPaceCapacity &&
                rain.RelaxationPasses<=BankedPaceCapacity,
                "closed-route relaxation converged");
        Require(full.StepCm>100.0 &&
                full.StepCm<1000.0 &&
                Near(full.StepCm*BankedPaceCapacity,
                     circuit.LapLengthCm),
                "each original circuit is discretized at same source cadence");
        Require(full.MinimumTargetSpeedCmps>=dry.MinimumAcceptedSpeedCmps &&
                full.MaximumTargetSpeedCmps<=dry.MaximumSpeedCmps &&
                full.MinimumTargetSpeedCmps<=full.MaximumTargetSpeedCmps,
                "finite speed limits remain inside accepted envelope");
        Require(rain.MinimumLocalSpeedCapCmps<=
                    full.MinimumLocalSpeedCapCmps+1e-6,
                "low grip never raises minimum cornering speed limit");
        Require(rain.EstimatedGeometricLapSeconds>=
                    full.EstimatedGeometricLapSeconds-1e-6,
                "lower known grip never invents a faster candidate lap");
        Require(full.MinimumGuaranteedBrakeCmps2>0.0 &&
                rain.MinimumGuaranteedBrakeCmps2>0.0,
                "brakeable route always reserves longitudinal friction");
        Require(full.EstimatedGeometricLapSeconds>1.0 &&
                full.EstimatedGeometricLapSeconds<3600.0,
                "advisory geometric lap duration is bounded");

        for(unsigned k=0u;k<BankedPaceCapacity;++k)
        {
            const auto& node=full.Nodes[k];
            const auto& wetNode=rain.Nodes[k];
            const auto& next=full.Nodes[(k+1u)%BankedPaceCapacity];
            const double braking=std::min(
                node.GuaranteedBrakingCmps2,
                next.GuaranteedBrakingCmps2);
            const double drive=std::min(
                node.GuaranteedDriveCmps2,
                next.GuaranteedDriveCmps2);
            Require(node.OriginalRoadVerified &&
                    wetNode.OriginalRoadVerified &&
                    node.DistanceCm>=0.0 &&
                    node.DistanceCm<circuit.LapLengthCm &&
                    node.TargetSpeedCmps>0.0 &&
                    node.TargetSpeedCmps<=node.LocalSpeedCapCmps+1e-6,
                    "each pace knot backed by authentic source road");
            Require(node.LateralDemandAtCapCmps2<=
                        node.MaximumLateralAccelerationCmps2+1e-4 &&
                    node.MaximumLateralAccelerationCmps2<
                        node.EffectiveGripAccelerationCmps2 &&
                    node.GuaranteedBrakingCmps2>0.0 &&
                    node.GuaranteedDriveCmps2>0.0,
                    "friction circle leaves a real conservative reserve");
            Require(wetNode.LocalSpeedCapCmps<=
                        node.LocalSpeedCapCmps+1e-6,
                    "each low-grip knot cannot raise speed limit");
            const double v2=node.TargetSpeedCmps*node.TargetSpeedCmps;
            const double next2=next.TargetSpeedCmps*next.TargetSpeedCmps;
            Require(v2<=next2+2.0*full.StepCm*braking+0.10 &&
                    next2<=v2+2.0*full.StepCm*drive+0.10,
                    "all directed edges including seam obey brake/drive");
        }
        for(double distance: {
            0.0,
            1.0,
            circuit.LapLengthCm*0.125,
            circuit.LapLengthCm*0.5,
            circuit.LapLengthCm-1.0})
        {
            const auto a=SampleWorldScaleBankedPace(
                circuit,full,distance);
            const auto b=SampleWorldScaleBankedPace(
                circuit,full,distance+circuit.LapLengthCm);
            Require(a.Valid && b.Valid &&
                    a.NoOfficialResultAuthority &&
                    b.NoOfficialResultAuthority &&
                    Near(a.TargetSpeedCmps,b.TargetSpeedCmps,1e-6) &&
                    Near(a.WorldHeightCm,b.WorldHeightCm,1e-6),
                    "interpolated pace and height repeat across lap seam");
        }
        const auto invalid=SampleWorldScaleBankedPace(
            circuit,full,nan);
        Require(!invalid.Valid,"no nonfinite pace lookup");
        const auto wrong=WorldScaleOriginalVariantRecipes[
            (index+1u)%WorldScaleVariantCount];
        Require(!SampleWorldScaleBankedPace(
                    wrong,full,0.0).Valid,
                "source track identity must match pace provenance");
        BankedPaceKnot knot;
        Require(!ComputeBankedPaceKnot(
            circuit,profile,dry,inf,0.1,knot),
            "nonfinite station cannot fabricate friction evidence");
        Require(!ComputeBankedPaceKnot(
            circuit,profile,dry,0.0,0.0,knot),
            "zero curvature window cannot be trusted");
        Require(ComputeBankedPaceKnot(
            circuit,profile,dry,0.0,full.StepCm*0.5,knot),
            "authentic road centerline friction sample");
        Require(Near(knot.WorldHeightCm,profile.RoadTopCm) &&
                Near(knot.RoadBankRadians,0.0),
                "start-line elevation and bank are periodic");

        // Copying a profile from another route never proves this geometry.
        auto invalidProfile=profile;
        invalidProfile.Circuit=wrong.Id;
        Require(!BuildWorldScaleBankedPace(
                    circuit,invalidProfile).Valid,
                "reject alien source track geometry");
        ++verified;
    }
    Require(verified==6u,"all six original routes independently tested");

    const auto circuit=WorldScaleOriginalVariantRecipes[0];
    WorldScaleBankedRoadProfile profile;
    Require(ChooseWorldScaleBankedRoadProfile(
                circuit.Id,profile),"choose original loop");
    const auto banked=BuildWorldScaleBankedPace(
        circuit,profile);
    profile.BankAmplitudeRadians=0.0;
    const auto flat=BuildWorldScaleBankedPace(
        circuit,profile);
    Require(banked.Valid && flat.Valid,
            "banked and flat candidate paths are both defined");
    Require(banked.EstimatedGeometricLapSeconds>=
                flat.EstimatedGeometricLapSeconds-1e-6,
            "cautious bank penalty never invents a speed bonus");
    std::cout<<"WorldScaleBankedPace: six circuits / friction proof passed\n";
    return EXIT_SUCCESS;
}
