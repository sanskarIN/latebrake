#include "Garage/PhysicalBrakeThermalReview.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Require(bool good,const char* what) {
    if(!good){
        std::cerr<<"PhysicalBrakeThermalReview FAIL: "<<what<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::SixCarTrackSurfaceEvidence Grip(
    latebrake::WorldScaleVariantId id)
{
    latebrake::SixCarTrackSurfaceEvidence evidence;
    evidence.Circuit=id;
    evidence.Session=17u;
    evidence.SourceFrame=32u;
    evidence.ClockSeconds=10.0;
    evidence.MeasuredGripFraction=0.83;
    evidence.SourceConfidence=1.0;
    evidence.AuthenticRoadMaterialObserved=true;
    evidence.ActualTireContactObserved=true;
    evidence.OriginalRaceRulesPreserved=true;
    return evidence;
}
std::array<latebrake::PhysicalBrakeRotorWitness,4> Rotors() {
    using namespace latebrake;
    std::array<PhysicalBrakeRotorWitness,4> result{};
    for(unsigned i=0u;i<PhysicalBrakeCornerCount;++i){
        auto& rotor=result[i];
        rotor.DriverActor=500u;
        rotor.NativeRotor=2000u+i;
        rotor.NativeWheel=3000u+i;
        rotor.SourceFrame=90u;
        rotor.SourceClockSeconds=10.5;
        rotor.RotorTemperatureC=350.0;
        rotor.RotorThicknessMm=32.0;
        rotor.ManufacturerMinimumThicknessMm=20.0;
        rotor.HydraulicPressureKpa=2200.0;
        rotor.MeasuredBrakeTorqueNm=i<2u?1750.0:1250.0;
        rotor.AngularVelocityRadiansPerSecond=150.0;
        rotor.MeasuredContactConfidence=1.0;
        rotor.NativeRotorTemperatureObserved=true;
        rotor.NativeHydraulicPressureObserved=true;
        rotor.OriginalPhysicalWheelObserved=true;
        rotor.NativeRoadContactVerified=true;
        rotor.ActualRotorWearObserved=true;
    }
    return result;
}
}
int main() {
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto settings=PhysicalBrakeThermalSettings{};
    Require(ValidPhysicalBrakeThermalSettings(settings),
            "real brake thermal source bounds valid");
    settings.NeverApplyBrakeForce=false;
    Require(!ValidPhysicalBrakeThermalSettings(settings),
            "source-only review cannot write Chaos brake forces");
    settings={};
    settings.MaximumRotorTemperatureC=nan;
    Require(!ValidPhysicalBrakeThermalSettings(settings),
            "nonfinite physical temperature bound rejected");
    settings={};
    Require(ClassifyPhysicalBrakeTemperature(50.0)==
                PhysicalBrakeThermalBand::Cold &&
            ClassifyPhysicalBrakeTemperature(400.0)==
                PhysicalBrakeThermalBand::Nominal &&
            ClassifyPhysicalBrakeTemperature(690.0)==
                PhysicalBrakeThermalBand::Hot &&
            ClassifyPhysicalBrakeTemperature(830.0)==
                PhysicalBrakeThermalBand::FadeRisk &&
            ClassifyPhysicalBrakeTemperature(990.0)==
                PhysicalBrakeThermalBand::EmergencyReview &&
            ClassifyPhysicalBrakeTemperature(nan)==
                PhysicalBrakeThermalBand::Unverified,
            "cold/nominal/fade/emergency thresholds are explicit");
    unsigned verified=0u;
    for(unsigned circuit=0u;circuit<WorldScaleVariantCount;++circuit) {
        const auto& road=WorldScaleOriginalVariantRecipes[circuit];
        for(int car=0;car<VehicleCount;++car) {
            GarageSnapshot saved;
            saved.Credits=1000;
            saved.OwnedVehicleMask=KnownVehicleMask;
            saved.SelectedVehicle=car;
            saved.EngineLevel=1;
            saved.GripLevel=2;
            const auto garage=BuildSixCarCircuitSetup(
                road,saved,Grip(road.Id));
            Require(garage.Valid,
                "original 36 configurations have authentic car IDs");
            const auto data=Rotors();
            Require(ValidPhysicalBrakeRotorWitness(
                data[0],500u,90u,10.5),
                "fully measured rotor sensor readout valid");
            const auto result=BuildPhysicalBrakeThermalReview(
                garage,500u,90u,10.5,data);
            Require(result.Valid &&
                result.Circuit==road.Id &&
                result.OriginalGarageVehicleId==car &&
                result.DriverActor==500u &&
                result.SourceSession==17u &&
                result.SourceFrame==90u &&
                result.Corners.size()==PhysicalBrakeCornerCount &&
                result.MaximumRotorTemperatureC==350.0 &&
                result.MeanRotorTemperatureC==350.0 &&
                result.MinimumRotorThicknessMarginMm==12.0 &&
                result.FadeRiskCorners==0u &&
                result.EmergencyReviewCorners==0u &&
                result.LowThicknessCorners==0u &&
                result.AxlesWithHotSplit==0u &&
                result.AxlesWithTorqueImbalance==0u &&
                result.FrontTorqueFraction>0.5 &&
                result.FrontTorqueFraction<0.7 &&
                result.NoBrakeCommandsIssued &&
                result.SourceOnly &&
                result.NoCareerPenaltyOrRaceResultAuthority,
                "all four actual original discs audited without changing physics");
            for(unsigned wheel=0u;wheel<PhysicalBrakeCornerCount;++wheel){
                const auto& rotor=result.Corners[wheel];
                Require(rotor.Valid && rotor.CornerIndex==wheel &&
                    rotor.NativeRotor==2000u+wheel &&
                    rotor.NativeWheel==3000u+wheel &&
                    rotor.ActualRotorTemperatureC==350.0 &&
                    rotor.ActualPressureKpa==2200.0 &&
                    rotor.MeasuredThicknessMarginMm==12.0 &&
                    rotor.WearReserveFraction>0.0 &&
                    rotor.WearReserveFraction<=1.0 &&
                    rotor.EstimatedThermalHeadroomC>0.0 &&
                    rotor.SuggestedCoolingReviewSeconds==0.0 &&
                    rotor.Band==PhysicalBrakeThermalBand::Nominal &&
                    rotor.PhysicalRotorUnchanged,
                    "source-native corner keeps independent thermal/wear proof");
            }
            auto heated=data;
            heated[1].RotorTemperatureC=980.0;
            heated[1].RotorThicknessMm=21.0;
            const auto emergency=BuildPhysicalBrakeThermalReview(
                garage,500u,90u,10.5,heated);
            Require(emergency.Valid &&
                emergency.EmergencyReviewCorners==1u &&
                emergency.FadeRiskCorners==1u &&
                emergency.LowThicknessCorners==1u &&
                emergency.AxlesWithHotSplit==1u &&
                emergency.MaximumAxleTemperatureSplitC>=600.0 &&
                emergency.MaximumSuggestedCoolingReviewSeconds>0.0 &&
                !emergency.Corners[1].Band==false,
                "high rotor heat and minimum measured disc thickness require review");
            auto asymmetric=data;
            asymmetric[1].MeasuredBrakeTorqueNm=100.0;
            const auto imbalance=BuildPhysicalBrakeThermalReview(
                garage,500u,90u,10.5,asymmetric);
            Require(imbalance.Valid &&
                imbalance.AxlesWithTorqueImbalance==1u &&
                imbalance.MaximumAxleTorqueImbalance>
                    settings.MaximumAxleTorqueImbalanceFraction &&
                imbalance.NoBrakeCommandsIssued,
                "native left/right rotor torque mismatch yields diagnostic only");
            auto malformed=data;
            malformed[1].NativeRotor=malformed[0].NativeRotor;
            Require(!BuildPhysicalBrakeThermalReview(
                garage,500u,90u,10.5,malformed).Valid,
                "duplicated Chaos brake rotor identity rejected");
            malformed=data;
            malformed[2].NativeRoadContactVerified=false;
            Require(!BuildPhysicalBrakeThermalReview(
                garage,500u,90u,10.5,malformed).Valid,
                "unverified native asphalt wheel contact fails closed");
            malformed=data;
            malformed[3].RotorTemperatureC=nan;
            Require(!BuildPhysicalBrakeThermalReview(
                garage,500u,90u,10.5,malformed).Valid,
                "nonfinite rotor temperature rejected");
            malformed=data;
            malformed[3].ManufacturerMinimumThicknessMm=33.0;
            Require(!BuildPhysicalBrakeThermalReview(
                garage,500u,90u,10.5,malformed).Valid,
                "minimum thickness greater than actual disc fails closed");
            malformed=data;
            malformed[3].SourceFrame=89u;
            Require(!BuildPhysicalBrakeThermalReview(
                garage,500u,90u,10.5,malformed).Valid,
                "stale brake sensor frame cannot inherit original race");
            malformed=data;
            malformed[3].DriverActor=0u;
            Require(!BuildPhysicalBrakeThermalReview(
                garage,500u,90u,10.5,malformed).Valid,
                "unidentified brake source actor cannot qualify");
            ++verified;
        }
    }
    Require(verified==SixCarSetupVehicleCount*
        WorldScaleVariantCount,
        "all 36 original car and course combinations independently verified");
    std::cout<<"PhysicalBrakeThermalReview: six cars, six circuits passed\n";
    return EXIT_SUCCESS;
}
