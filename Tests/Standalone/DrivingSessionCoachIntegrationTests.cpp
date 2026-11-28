#include "Racing/DrivingSessionStats.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok,const char* message)
{
    if(!ok)
    {
        std::cerr<<"Driving session coach integration: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a,double b)
{
    return std::abs(a-b)<1e-6;
}
}

int main()
{
    latebrake::DrivingSessionStats session;
    for(int frame=0;frame<160;++frame)
        Check(session.Observe({0.05,2000.0,frame<32?800.0:0.0,frame<50}),
            "physical sampled input accepted by the existing telemetry API");
    const auto coaching=session.GetCoachReview();
    Check(coaching.Valid && coaching.AcceptedSamples==160 &&
        coaching.AcceptedBrakeEpisodes==
        static_cast<std::uint64_t>(session.GetBrakeApplications()),
        "coach shares the same trusted sample boundary as existing driver stats");
    Check(Near(coaching.ObservedSeconds,session.GetActiveSampledSeconds()) &&
        Near(coaching.DistanceKm,session.GetDistanceKm()) &&
        Near(coaching.PeakSpeedKmh,session.GetPeakSpeedKmH()) &&
        Near(coaching.BrakeSeconds,session.GetBrakingSeconds()) &&
        Near(coaching.HighSlipSeconds,session.GetHighSlipSeconds()),
        "new coach aggregates exactly match legacy observation counters");
    Check(session.GetSpeedDistribution().Valid,
        "legacy sampled speed distribution retains its validation");
    const auto accepted=session.GetAcceptedSamples();
    const auto priorTime=coaching.ObservedSeconds;
    Check(!session.Observe({std::numeric_limits<double>::quiet_NaN(),0,0,false}),
        "NaN input still refused by authoritative telemetry filter");
    const auto refused=session.GetCoachReview();
    Check(refused.RefusedSamples==1 && refused.Suspensions==1 &&
        refused.AcceptedSamples==static_cast<std::uint64_t>(accepted) &&
        Near(refused.ObservedSeconds,priorTime),
        "invalid frame increments diagnostic refusal without inventing mileage");
    session.Suspend();
    Check(session.GetCoachReview().Suspensions==2,
        "explicit pause also invalidates episode continuity");
    session.Reset();
    Check(!session.GetCoachReview().Valid && session.GetAcceptedSamples()==0 &&
        session.GetBrakeApplications()==0 && session.GetDistanceKm()==0.0,
        "session restart clears both new and legacy transient data");
    std::cout<<"DrivingSessionCoachIntegration: passed.\n";
    return EXIT_SUCCESS;
}
