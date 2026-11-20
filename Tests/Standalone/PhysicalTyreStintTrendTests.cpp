#include "Garage/PhysicalTyreStintTrend.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Require(bool pass,const char* what) {
    if(!pass) {
        std::cerr<<"PhysicalTyreStintTrend FAIL: "<<what<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::PhysicalTyreTrendSample Frame(
    unsigned id,int originalVehicle=0)
{
    using namespace latebrake;
    PhysicalTyreTrendSample frame;
    frame.Session=17u;
    frame.Revision=2u;
    frame.OwnerActor=100u;
    frame.Frame=id;
    frame.ActiveClockSeconds=0.05*id;
    frame.StepSeconds=0.05;
    frame.OriginalVehicleId=originalVehicle;
    frame.Circuit=WorldScaleVariantId::OriginalLoop;
    frame.OriginalEngineTyresObserved=true;
    frame.NoUnrealForceChanges=true;
    for(unsigned i=0;i<PhysicalTyreTrendWheels;++i) {
        auto& wheel=frame.Wheels[i];
        wheel.OwnerActor=frame.OwnerActor;
        wheel.NativeWheel=300u+i;
        wheel.SourceFrame=frame.Frame;
        wheel.ClockSeconds=frame.ActiveClockSeconds;
        wheel.HotPressureKpa=210.0+0.10*id;
        wheel.CarcassTemperatureC=72.0+0.15*id;
        wheel.SurfaceTemperatureC=75.0+0.08*id;
        wheel.TreadDepthMm=7.0-0.0005*id;
        wheel.SensorConfidence=1.0;
        wheel.ActualChaosWheelObserved=true;
        wheel.ActualOriginalRoadContact=true;
        wheel.ActualPressureMeasurement=true;
        wheel.ActualTemperatureMeasurement=true;
        wheel.ActualTreadMeasurement=true;
    }
    return frame;
}
bool Near(double a,double b,double tolerance=1e-6) {
    return std::abs(a-b)<=tolerance;
}
}
int main() {
    using namespace latebrake;
    auto settings=PhysicalTyreTrendSettings{};
    Require(ValidPhysicalTyreTrendSettings(settings),
        "original tire-source rate settings bounded");
    settings.NeverWriteTyreWear=false;
    Require(!ValidPhysicalTyreTrendSettings(settings),
        "source-only data cannot write actual tire wear");
    settings={};
    settings.MaximumStepSeconds=
        std::numeric_limits<double>::quiet_NaN();
    Require(!ValidPhysicalTyreTrendSettings(settings),
        "nonfinite maximum frame time fails closed");
    settings={};
    PhysicalTyreStintTrend tracker;
    auto before=tracker.Summarize();
    Require(!before.Valid && before.Fault==
        PhysicalTyreTrendFault::MissingBaseline &&
        tracker.Stored()==0u,
        "no fabricated tire trend before physical observations");
    Require(ValidPhysicalTyreTrendSample(Frame(1u)),
        "all four physical wheel sources are accepted");
    Require(tracker.Append(Frame(1u)) &&
        tracker.Stored()==1u,
        "first original sample starts a bounded history");
    auto first=tracker.Summarize();
    Require(first.Valid &&
        first.StoredSourceFrames==1u &&
        first.ObservedDurationSeconds==0.0 &&
        first.MinimumObservedTreadMm>0.0 &&
        first.NoUnrealTyreCorrectionSubmitted,
        "one truthful wheel frame produces descriptive, not action, status");

    auto malformed=Frame(2u);
    malformed.Wheels[0].NativeWheel=
        malformed.Wheels[1].NativeWheel;
    Require(!tracker.Append(malformed) &&
        tracker.LastFault()==PhysicalTyreTrendFault::InvalidSource &&
        tracker.Stored()==1u,
        "duplicate Chaos wheel identity cannot corrupt ring");
    malformed=Frame(2u);
    malformed.Wheels[0].HotPressureKpa=
        std::numeric_limits<double>::quiet_NaN();
    Require(!tracker.Append(malformed) &&
        tracker.LastFault()==PhysicalTyreTrendFault::InvalidSource,
        "nonfinite wheel pressure rejects entire source frame");
    malformed=Frame(2u);
    malformed.Wheels[1].ActualOriginalRoadContact=false;
    Require(!tracker.Append(malformed),
        "lost authentic wheel-to-road contact denies source");
    malformed=Frame(2u);
    malformed.Revision=3u;
    Require(!tracker.Append(malformed) &&
        tracker.LastFault()==PhysicalTyreTrendFault::RosterChanged,
        "different racing epoch cannot borrow prior trend");
    malformed=Frame(2u);
    malformed.OwnerActor=999u;
    for(auto& w:malformed.Wheels)w.OwnerActor=999u;
    Require(!tracker.Append(malformed) &&
        tracker.LastFault()==PhysicalTyreTrendFault::RosterChanged,
        "different physical chassis starts a different tire session");

    malformed=Frame(2u);
    malformed.Wheels[0].TreadDepthMm=5.0;
    Require(!tracker.Append(malformed) &&
        tracker.LastFault()==PhysicalTyreTrendFault::UnrealisticSensorRate,
        "unphysical sudden 2mm tread jump cannot be called measured wear");
    malformed=Frame(2u);
    malformed.Wheels[0].HotPressureKpa=290.0;
    Require(!tracker.Append(malformed) &&
        tracker.LastFault()==PhysicalTyreTrendFault::UnrealisticSensorRate,
        "implausible pressure sensor jump fails continuity");
    malformed=Frame(2u);
    malformed.Wheels[0].CarcassTemperatureC=130.0;
    Require(!tracker.Append(malformed) &&
        tracker.LastFault()==PhysicalTyreTrendFault::UnrealisticSensorRate,
        "unrealistic thermal discontinuity fails continuity");
    malformed=Frame(2u);
    malformed.ActiveClockSeconds=0.03;
    for(auto& w:malformed.Wheels)
        w.ClockSeconds=0.03;
    Require(!tracker.Append(malformed) &&
        tracker.LastFault()==PhysicalTyreTrendFault::TimeRollback,
        "clock reversal rejected despite plausible wheel values");
    malformed=Frame(2u);
    malformed.Frame=1u;
    for(auto& w:malformed.Wheels)w.SourceFrame=1u;
    Require(!tracker.Append(malformed) &&
        tracker.LastFault()==PhysicalTyreTrendFault::StaleFrame,
        "duplicate engine frame never counted twice");

    for(unsigned id=2u;id<=90u;++id) {
        Require(tracker.Append(Frame(id)),
            "continuous four-wheel real-sensor readout accepted");
        Require(tracker.Stored()<=PhysicalTyreTrendCapacity,
            "stint ring never grows beyond fixed bounded source history");
    }
    Require(tracker.Stored()==PhysicalTyreTrendCapacity,
        "history retains most recent 64 verified physical frames");
    PhysicalTyreTrendSample oldest;
    PhysicalTyreTrendSample newest;
    Require(tracker.ReadOldest(0u,oldest) &&
        tracker.ReadOldest(PhysicalTyreTrendCapacity-1u,newest) &&
        oldest.Frame==90u-PhysicalTyreTrendCapacity+1u &&
        newest.Frame==90u &&
        !tracker.ReadOldest(PhysicalTyreTrendCapacity,newest) &&
        newest.Frame==0u,
        "chronological bounded ring correctly evicts old readings");
    const auto summary=tracker.Summarize();
    Require(summary.Valid &&
        summary.Session==17u &&
        summary.Revision==2u &&
        summary.OwnerActor==100u &&
        summary.FirstFrame==oldest.Frame &&
        summary.LastFrame==90u &&
        summary.StoredSourceFrames==PhysicalTyreTrendCapacity &&
        summary.RejectedFrames>=8u &&
        summary.ObservedDurationSeconds>0.0 &&
        summary.PeakObservedCarcassC>0.0 &&
        summary.MinimumObservedTreadMm>0.0 &&
        summary.NoUnrealTyreCorrectionSubmitted &&
        summary.NoCareerPitCreditsAwarded,
        "bounded stint trend carries source provenance not native mutations");
    for(unsigned i=0u;i<PhysicalTyreTrendWheels;++i) {
        const auto& wheel=summary.Wheels[i];
        Require(wheel.Valid &&
            wheel.NativeWheel==300u+i &&
            wheel.WheelIndex==i &&
            wheel.PressureGainKpaPerSecond>0.0 &&
            wheel.CarcassGainCPerSecond>0.0 &&
            wheel.TreadWearMmPerSecond>0.0 &&
            wheel.MeanCarcassTemperatureC>=
                wheel.FirstCarcassTemperatureC &&
            wheel.MeanCarcassTemperatureC<=
                wheel.LatestCarcassTemperatureC &&
            wheel.PeakCarcassTemperatureC>=
                wheel.MeanCarcassTemperatureC &&
            Near(wheel.LatestPressureKpa,219.0),
            "each genuine wheel independently accumulates thermal/wear rate");
    }
    tracker.Reset();
    Require(tracker.Stored()==0u &&
        tracker.RejectedCount()==0u &&
        !tracker.Summarize().Valid &&
        tracker.Append(Frame(1u)),
        "explicit new session reset prevents cross-race sensor reuse");
    std::cout<<"PhysicalTyreStintTrend: source ring checks passed\n";
    return EXIT_SUCCESS;
}
