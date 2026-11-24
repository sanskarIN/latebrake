#include "Garage/SixCarCircuitSetupResearch.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Require(bool v,const char* why){
    if(!v){std::cerr<<"SixCarCircuitSetup FAIL: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
latebrake::SixCarTrackSurfaceEvidence Surface(
    latebrake::WorldScaleVariantId id,double grip)
{
    latebrake::SixCarTrackSurfaceEvidence e;
    e.Circuit=id;
    e.Session=42u;
    e.SourceFrame=99u;
    e.ClockSeconds=11.0;
    e.MeasuredGripFraction=grip;
    e.SourceConfidence=1.0;
    e.AuthenticRoadMaterialObserved=true;
    e.ActualTireContactObserved=true;
    e.OriginalRaceRulesPreserved=true;
    return e;
}
latebrake::GarageSnapshot GarageFor(int car){
    latebrake::GarageSnapshot g;
    g.Credits=1000;
    g.OwnedVehicleMask=latebrake::KnownVehicleMask;
    g.SelectedVehicle=car;
    g.EngineLevel=2;
    g.GripLevel=3;
    return g;
}
bool Near(double a,double b,double tolerance=1.0e-5){
    return std::abs(a-b)<=tolerance;
}
}
int main(){
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    Require(SixCarSetupVehicleCount==6u &&
            SixCarSetupCircuitCount==WorldScaleVariantCount,
            "all six original car and road IDs retained");
    SixCarSetupSettings settings;
    Require(ValidSixCarSetupSettings(settings),
            "finite original mechanical setup policy valid");
    settings.NeverSetUnrealWheelForces=false;
    Require(!ValidSixCarSetupSettings(settings),
            "source setup cannot actuate native suspension");
    settings={};
    settings.MaximumWingDegrees=nan;
    Require(!ValidSixCarSetupSettings(settings),
            "nonfinite aero bound rejected");
    settings={};
    Require(ValidSixCarOriginalGarage({}),
            "default original garage owns car zero");
    for(unsigned car=0u;car<SixCarSetupVehicleCount;++car)
        Require(ValidSixCarMechanicalSetup(
            SixCarOriginalVehicleSetup[car]),
            "original six mechanical car profiles have physically bounded values");
    unsigned authentic=0u;
    for(unsigned circuitIndex=0u;
        circuitIndex<WorldScaleVariantCount;++circuitIndex){
        const auto circuit=WorldScaleOriginalVariantRecipes[circuitIndex];
        Require(ValidWorldScaleVariantRecipe(circuit),
                "original six independent routes are source validated");
        for(int car=0;car<VehicleCount;++car){
            const auto garage=GarageFor(car);
            const auto drySurface=Surface(circuit.Id,1.0);
            const auto wetSurface=Surface(circuit.Id,0.55);
            const auto dry=BuildSixCarCircuitSetup(
                circuit,garage,drySurface);
            const auto wet=BuildSixCarCircuitSetup(
                circuit,garage,wetSurface);
            Require(dry.Valid && wet.Valid &&
                    dry.VehicleId==car &&
                    wet.VehicleId==car &&
                    dry.Circuit==circuit.Id &&
                    wet.Circuit==circuit.Id &&
                    dry.SourceSession==42u &&
                    dry.SourceFrame==99u &&
                    dry.OriginalUpgradeEngineLevel==2u &&
                    dry.OriginalUpgradeGripLevel==3u &&
                    dry.OriginalGarageStateUnchanged &&
                    wet.NativeChaosConfigurationUnchanged &&
                    wet.SourceOnly,
                    "garage setup is a preview over true owned six-car identity");
            Require(ValidSixCarMechanicalSetup(dry.Values) &&
                    ValidSixCarMechanicalSetup(wet.Values) &&
                    wet.Values.FrontRideCm>=dry.Values.FrontRideCm &&
                    wet.Values.RearRideCm>=dry.Values.RearRideCm &&
                    wet.Values.DifferentialLockFraction<=
                        dry.Values.DifferentialLockFraction &&
                    wet.Values.FrontAntiRollNm<=
                        dry.Values.FrontAntiRollNm &&
                    dry.Values.TyrePressureKpa>0.0,
                    "wet source grip recommends safer, bounded original setup");
            Require(dry.PlannedAeroBalanceFront>0.0 &&
                    dry.PlannedAeroBalanceFront<1.0 &&
                    dry.EstimatedSpringBalanceFront>0.0 &&
                    dry.EstimatedSpringBalanceFront<1.0 &&
                    dry.CornerSeverity>=0.0 &&
                    dry.CornerSeverity<=1.0 &&
                    dry.SetupDifferenceNorm>=0.0 &&
                    dry.RideStanceCentimeters>=6.0 &&
                    dry.PitchStanceCentimeters>=0.0,
                    "all mechanical balance diagnostics bounded per circuit");
            const auto delta=CompareSixCarSetup(dry,wet);
            Require(delta.Valid &&
                    delta.VehicleId==car &&
                    delta.TotalAbsoluteChange>=0.0 &&
                    delta.FrontRideDeltaCm>=0.0 &&
                    delta.RearRideDeltaCm>=0.0 &&
                    delta.FrontBrakeFractionDelta>=0.0 &&
                    delta.NoNativeCarModification,
                    "setup comparison never actually writes chassis");
            const auto same=CompareSixCarSetup(dry,dry);
            Require(same.Valid &&
                    Near(same.TotalAbsoluteChange,0.0),
                    "identical native-read-only setups produce zero difference");
            // Comparisons are meaningful only for the SAME original
            // circuit, owned vehicle, source session and garage upgrades.
            // Source receipts are read-only evidence, not a way to apply
            // forged settings to Chaos or mutate player progression.
            auto other=wet;
            other.Circuit=WorldScaleOriginalVariantRecipes[
                (circuitIndex+1u)%WorldScaleVariantCount].Id;
            Require(!CompareSixCarSetup(dry,other).Valid,
                    "foreign circuit previews cannot be compared");
            other=wet;
            other.SourceSession=43u;
            Require(!CompareSixCarSetup(dry,other).Valid,
                    "foreign race sessions cannot share setup provenance");
            other=wet;
            other.SourceFrame=0u;
            Require(!CompareSixCarSetup(dry,other).Valid,
                    "unobserved source frame cannot be compared");
            other=wet;
            ++other.SourceFrame;
            Require(CompareSixCarSetup(dry,other).Valid,
                    "real newer frame in same session can show wet change");
            other=wet;
            other.OriginalUpgradeGripLevel=2u;
            Require(!CompareSixCarSetup(dry,other).Valid,
                    "different garage upgrade states need separate preview");
            other=wet;
            other.OriginalUpgradeEngineLevel=4u;
            Require(!CompareSixCarSetup(dry,other).Valid,
                    "corrupt original engine upgrade level rejected");
            other=wet;
            other.SourceOnly=false;
            Require(!CompareSixCarSetup(dry,other).Valid,
                    "native-actuating receipt cannot masquerade as preview");
            other=wet;
            other.NativeChaosConfigurationUnchanged=false;
            Require(!CompareSixCarSetup(dry,other).Valid,
                    "native wheel mutation invalidates advisory comparison");
            other=wet;
            other.OriginalGarageStateUnchanged=false;
            Require(!CompareSixCarSetup(dry,other).Valid,
                    "garage credit or selection mutation invalidates receipt");
            other=wet;
            other.MeasuredGripFraction=nan;
            Require(!CompareSixCarSetup(dry,other).Valid,
                    "nonfinite contact grip cannot enter comparison");
            other=wet;
            other.MeasuredGripFraction=-0.1;
            Require(!CompareSixCarSetup(dry,other).Valid,
                    "negative fabricated grip cannot enter comparison");
            auto forgedFirst=dry;
            auto forgedNext=wet;
            forgedFirst.VehicleId=VehicleCount;
            forgedNext.VehicleId=VehicleCount;
            Require(!CompareSixCarSetup(forgedFirst,forgedNext).Valid,
                    "matching out-of-range car IDs are not proof of ownership");
            forgedFirst=dry;
            forgedNext=wet;
            forgedFirst.Circuit=static_cast<WorldScaleVariantId>(255);
            forgedNext.Circuit=forgedFirst.Circuit;
            Require(!CompareSixCarSetup(forgedFirst,forgedNext).Valid,
                    "matching unknown circuit IDs are never authentic");
            auto corrupt=garage;
            corrupt.OwnedVehicleMask=(1<<car);
            if(car>0){
                Require(!BuildSixCarCircuitSetup(
                    circuit,corrupt,drySurface).Valid,
                    "no original car-zero ownership bit is invalid saved state");
            }
            corrupt=garage;
            corrupt.SelectedVehicle=VehicleCount;
            Require(!BuildSixCarCircuitSetup(
                circuit,corrupt,drySurface).Valid,
                "out-of-range garage ID cannot create a preview");
            corrupt=garage;
            corrupt.GripLevel=100;
            Require(!BuildSixCarCircuitSetup(
                circuit,corrupt,drySurface).Valid,
                "invalid purchased upgrade cannot be silently coerced");
            auto bad=wetSurface;
            bad.ActualTireContactObserved=false;
            Require(!BuildSixCarCircuitSetup(
                circuit,garage,bad).Valid,
                "unobserved physical grip cannot alter a preview");
            bad=wetSurface;
            bad.MeasuredGripFraction=nan;
            Require(!BuildSixCarCircuitSetup(
                circuit,garage,bad).Valid,
                "nonfinite measured grip rejected on every circuit");
            bad=wetSurface;
            bad.Circuit=WorldScaleOriginalVariantRecipes[
                (circuitIndex+1u)%WorldScaleVariantCount].Id;
            Require(!BuildSixCarCircuitSetup(
                circuit,garage,bad).Valid,
                "a foreign track cannot inherit original circuit tuning");
            ++authentic;
        }
    }
    Require(authentic==SixCarSetupVehicleCount*
            WorldScaleVariantCount,
            "36 original car/circuit combinations validated without Chaos");
    std::cout<<"SixCarCircuitSetupResearch: 36 source combinations passed\n";
    return EXIT_SUCCESS;
}
