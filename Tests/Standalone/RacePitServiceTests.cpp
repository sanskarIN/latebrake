#include "Racing/RacePitService.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}
void StampedObservationContinuity()
{
    using namespace latebrake;
    RacePitService pit;
    PitServiceObservation input{0.05,0,-1300,0,0,true,false,true,true,true};
    PitServiceFrameStamp stamp{1,0,0,101};
    const auto step=[&]()
    {
        const bool completed=pit.Observe(input,stamp);
        ++stamp.EngineFrame;
        return completed;
    };
    for(int i=0;i<30;++i) Check(!step(),"initial partial hold");
    Check(std::abs(pit.ProgressFraction()-0.6)<1e-8,"partial progress");
    auto duplicate=stamp;--duplicate.EngineFrame;
    Check(!pit.Observe(input,duplicate) && pit.ProgressFraction()==0,
        "duplicate cannot contribute or preserve pending hold");
    auto r=pit.ReviewFrames();
    Check(r.RefusedFrames==1 && r.AcceptedFrames==30 && r.InterruptedHolds==1 &&
        std::abs(r.LongestInterruptedHoldSeconds-1.5)<1e-8,"accurate interruption evidence");
    for(int i=0;i<10;++i) Check(!step(),"resume fresh");
    ++stamp.EngineFrame;
    Check(!step() && std::abs(pit.ProgressFraction()-0.02)<1e-8,
        "missing tick restarts hold");
    for(int i=0;i<10;++i) Check(!step(),"before epoch change");
    ++stamp.SessionRevision;
    Check(!step() && std::abs(pit.ProgressFraction()-0.02)<1e-8,"epoch cuts hold");
    for(int i=0;i<10;++i) Check(!step(),"before teleport");
    ++stamp.TeleportRevision;
    Check(!step() && std::abs(pit.ProgressFraction()-0.02)<1e-8,"teleport cuts hold");
    auto stale=stamp;--stale.TeleportRevision;
    Check(!pit.Observe(input,stale),"teleport rollback");
    stale=stamp;--stale.SessionRevision;
    Check(!pit.Observe(input,stale),"session rollback");
    auto invalid=stamp;invalid.ControllerToken=0;
    Check(!pit.Observe(input,invalid),"missing controller");
    Check(!step(),"fresh authentic stamp");
    ++stamp.ControllerToken;
    Check(!step() && std::abs(pit.ProgressFraction()-0.02)<1e-8,"controller replacement cuts hold");
    for(int i=0;i<49;++i) Check(step()==(i==48),"full new physical hold");
    Check(pit.StopCount()==1 && !pit.IsArmed(),"one completed repair");
    // A duplicate outside observation cannot rearm the service latch.
    input.X=500;
    duplicate=stamp;--duplicate.EngineFrame;
    Check(!pit.Observe(input,duplicate) && !pit.IsArmed(),"stale exit cannot rearm");
    Check(!step() && pit.IsArmed(),"actual fresh exit re-arms");
    input.X=0;
    Check(!step(),"next visit starts from zero");
    pit.Suspend();
    Check(!step() && std::abs(pit.ProgressFraction()-0.02)<1e-8,
        "explicit suspension cuts hold but preserves high-water frame");
    r=pit.ReviewFrames();
    Check(r.RefusedFrames==5 && r.Discontinuities==4 &&
        r.LongestInterruptedHoldSeconds<2.5,"bounded monotonic counters");
    pit.Reset();
    stamp={1,0,0,101};
    Check(!step() && pit.ReviewFrames().AcceptedFrames==1 &&
        pit.ReviewFrames().RefusedFrames==0 && pit.StopCount()==0,
        "new attempt resets stamps and diagnostics");
}

void FiniteServiceCap()
{
    using namespace latebrake;
    RacePitService pit;
    PitServiceObservation input{0.10,0,-1300,0,0,true,false,true,true,true};
    PitServiceFrameStamp stamp{1,1,0,101};
    const auto step=[&]()
    {
        const bool completed=pit.Observe(input,stamp);
        ++stamp.EngineFrame;
        return completed;
    };
    for(int stop=1;stop<=RacePitStopLimit;++stop)
    {
        input.X=500;
        Check(!step(),"physical exit");
        input.X=0;
        for(int frame=0;frame<25;++frame)
            Check(step()==(frame==24),"one finite stop");
        Check(pit.StopCount()==stop,"count equals actual service completions");
    }
    input.X=500;
    Check(!step() && !pit.IsArmed(),"cap cannot rearm");
    input.X=0;
    for(int frame=0;frame<50;++frame)
        Check(!step(),"cap never grants uncounted repair");
    Check(pit.StopCount()==RacePitStopLimit &&
        pit.Guidance()==PitServiceGuidance::StopLimitReached,"explicit cap guidance");
}
}

int main()
{
    using latebrake::RacePitService;
    using latebrake::PitServiceObservation;
    using latebrake::IsInRacePitBay;

    Check(IsInRacePitBay(0.0, -1300.0), "bay center is on the road");
    Check(latebrake::IsOnPavedSurface(latebrake::RacePitBay),
          "pit is in the existing paved circuit");
    Check(!IsInRacePitBay(0.0, -1800.0),
          "rival center-line cannot count as pit service");
    Check(!IsInRacePitBay(0.0, -1000.0), "bay radius is bounded");
    Check(!IsInRacePitBay(std::numeric_limits<double>::quiet_NaN(), -1300.0),
          "nonfinite position rejected");

    RacePitService pit;
    PitServiceObservation input{
        0.05, 0.0, -1300.0, 0.0, 0.0, true, false, true, true, true};
    Check(pit.StopCount() == 0 && pit.IsArmed(), "initial arm");

    // Fail closed on official countdown/pause/menu/restart unavailability.
    input.Authorized = false;
    for (int i = 0; i < 60; ++i)
        Check(!pit.Observe(input), "unauthorized hold must not complete");
    Check(pit.ProgressFraction() == 0.0, "ineligible progress never accrues");
    input.Authorized = true;
    input.X = 0.0; input.Y = -1800.0;
    for (int i = 0; i < 60; ++i)
        Check(!pit.Observe(input), "main racing line cannot pit");
    Check(!pit.IsInBay(), "outside bay");
    input.Y = -1300.0;

    input.PlanarSpeedCmPerSecond = 300.0;
    Check(!pit.Observe(input), "rolling at speed disallowed");
    input.PlanarSpeedCmPerSecond = 0.0;
    input.RequestedThrottle = 0.2;
    Check(!pit.Observe(input), "held acceleration disallowed");
    input.RequestedThrottle = 0.0;
    input.NitroHeld = true;
    Check(!pit.Observe(input), "held nitro disallowed");
    input.NitroHeld = false;
    input.ButtonHeld = false;
    Check(!pit.Observe(input), "must physically hold service key");
    input.ButtonHeld = true;
    input.MaintenanceNeeded = false;
    Check(!pit.Observe(input), "fully healthy car cannot create extra stop");
    input.MaintenanceNeeded = true;

    for (int i = 0; i < 49; ++i)
        Check(!pit.Observe(input), "service cannot finish early");
    Check(pit.IsInBay() && pit.ProgressFraction() > 0.97 &&
              pit.ProgressFraction() < 1.0,
          "2.45 seconds earns no early service");
    Check(pit.Observe(input), "2.50s stationary hold completes one service");
    Check(pit.StopCount() == 1 && !pit.IsArmed() &&
              pit.ProgressFraction() == 0.0, "first stop is latched");

    for (int i = 0; i < 200; ++i)
        Check(!pit.Observe(input), "no infinite repairs on one pit visit");
    Check(pit.StopCount() == 1, "holding in the same bay never repeats");
    input.X = 500.0;
    Check(!pit.Observe(input) && pit.IsArmed(), "physical exit rearms");
    input.X = 0.0;
    for (int i = 0; i < 50; ++i)
        Check(pit.Observe(input) == (i == 49), "second timed service");
    Check(pit.StopCount() == 2, "return visit counts one stop");

    input.X = 500.0;
    Check(!pit.Observe(input), "exit twice");
    input.X = 0.0;
    Check(!pit.Observe(input), "begin third service");
    input.ButtonHeld = false;
    Check(!pit.Observe(input), "release cancels progress");
    Check(pit.ProgressFraction() == 0.0, "release is not resume-able");
    input.ButtonHeld = true;
    for (int i = 0; i < 20; ++i) Check(!pit.Observe(input), "hold begins");
    input.StableGround = false;
    Check(!pit.Observe(input), "lost stable support cancels the held service");
    Check(pit.ProgressFraction() == 0.0, "no partial repair credit after bounce");
    input.StableGround = true;
    Check(!pit.Observe(input), "ground settles but hold restarts from zero");
    input.DeltaSeconds = 0.4;
    Check(!pit.Observe(input), "large frame refuses credit");
    Check(pit.ProgressFraction() == 0.0, "long frame resets hold");

    const double invalid[] = {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(), -1.0};
    for (double n : invalid)
    {
        input.DeltaSeconds = 0.05;
        input.PlanarSpeedCmPerSecond = n;
        Check(!pit.Observe(input), "invalid chassis velocity");
        Check(pit.ProgressFraction() == 0.0, "invalid velocity clears progress");
        input.PlanarSpeedCmPerSecond = 0.0;
        input.RequestedThrottle = n;
        Check(!pit.Observe(input), "invalid driver axis");
        input.RequestedThrottle = 0.0;
    }
    pit.Reset();
    Check(pit.IsArmed() && !pit.IsInBay() && pit.StopCount() == 0 &&
              pit.ProgressFraction() == 0.0,
          "new race erases pit history");

    StampedObservationContinuity();
    FiniteServiceCap();
    std::cout << "RacePitService: all scenarios passed\n";
    return 0;
}
