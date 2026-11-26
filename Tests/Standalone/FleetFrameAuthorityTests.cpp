#include "AI/FleetFrameAuthority.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Require(bool value,const char* message)
{
    if(!value){std::cerr<<message<<'\n';std::exit(EXIT_FAILURE);}
}
void ExactFrame()
{
    FleetFrameAuthority authority;
    const FleetFrameStamp stamp{1,1,100,0};
    Require(!authority.BeginFrame(stamp),"unconfigured frame denied");
    Require(!authority.Begin(0),"zero attempt denied");
    Require(authority.Begin(1),"first real attempt");
    Require(!authority.IsCurrent(stamp),"begin does not create authority");
    Require(authority.BeginFrame(stamp),"begin exact observation");
    Require(!authority.IsCurrent(stamp),"sampling cannot authorize consumers");
    Require(authority.Commit(stamp,true),"qualified observation commits");
    Require(authority.IsCurrent(stamp),"exact stamp is admitted");
    auto mismatch=stamp;mismatch.Attempt=2;
    Require(!authority.IsCurrent(mismatch),"foreign attempt denied");
    mismatch=stamp;mismatch.Observation=2;
    Require(!authority.IsCurrent(mismatch),"future observation denied");
    mismatch=stamp;mismatch.EngineFrame=101;
    Require(!authority.IsCurrent(mismatch),"next engine frame denies stale permission");
    mismatch=stamp;mismatch.SessionEpoch=1;
    Require(!authority.IsCurrent(mismatch),"changed pause/restart epoch denies stale permission");
    Require(!authority.BeginFrame(stamp),"duplicate observation cannot advance twice");
    Require(!authority.IsCurrent(stamp),"duplicate revokes old permission");
    Require(!authority.Commit(stamp,true),"duplicate cannot recommit");
    Require(authority.BeginFrame({1,2,101,1}),"new frame after duplicate");
    Require(authority.Commit({1,2,101,1},true),"new frame qualified");
    Require(authority.IsCurrent({1,2,101,1}),"new permission exact");
    Require(!authority.BeginFrame({1,3,101,1}),"same engine frame with larger local token denied");
    Require(!authority.IsCurrent({1,2,101,1}),"same-engine-frame denial revokes prior capability");
    Require(!authority.BeginFrame({1,3,99,1}),"backward engine clock denied");
    Require(authority.BeginFrame({1,3,102,1}),"fresh engine frame");
    Require(!authority.Commit({1,3,102,2},true),"epoch changed during collection");
    Require(!authority.IsCurrent({1,3,102,1}),"wrong epoch leaves no permission");
}
void AttemptLifecycle()
{
    FleetFrameAuthority authority;
    Require(authority.Begin(7),"attempt begins");
    Require(authority.BeginFrame({7,1,0,0}),"first engine frame may be zero");
    Require(!authority.Commit({7,1,0,0},false),"rejected roster");
    Require(!authority.IsCurrent({7,1,0,0}),"no capability after rejection");
    Require(!authority.Begin(7),"same attempt cannot reset stamp");
    Require(!authority.Begin(6),"backward attempt cannot reset stamp");
    Require(authority.BeginFrame({7,2,1,0}),"next engine frame");
    Require(authority.Commit({7,2,1,0},true),"accepted");
    authority.Revoke();
    Require(!authority.IsCurrent({7,2,1,0}),"finish/pause teardown revoke");
    Require(authority.Begin(8),"new attempt permits new local token");
    Require(authority.BeginFrame({8,1,1,1}),"explicit restart on same engine frame allowed");
    Require(!authority.Commit({7,2,1,0},true),"delayed foreign result refused");
    Require(authority.Commit({8,1,1,1},true),"foreign result cannot disturb new sampling");
    Require(authority.IsCurrent({8,1,1,1}),"new attempt capability");
}
void Sweep()
{
    FleetFrameAuthority authority;
    for(std::uint64_t attempt=1;attempt<=32;++attempt)
    {
        Require(authority.Begin(attempt),"monotonic attempts");
        for(std::uint64_t f=1;f<=2048;++f)
        {
            const FleetFrameStamp stamp{attempt,f,f*2,f/128};
            Require(authority.BeginFrame(stamp),"monotonic observation");
            const bool good=(f%5)!=0;
            Require(authority.Commit(stamp,good)==good,"commit actual outcome");
            Require(authority.IsCurrent(stamp)==good,"exact per-frame outcome");
            auto stale=stamp;--stale.EngineFrame;
            Require(!authority.IsCurrent(stale),"prior engine frame never admitted");
            stale=stamp;++stale.SessionEpoch;
            Require(!authority.IsCurrent(stale),"future epoch never admitted");
        }
    }
    const auto maximum=std::numeric_limits<std::uint64_t>::max();
    Require(authority.Begin(maximum),"largest attempt token");
    Require(authority.BeginFrame({maximum,maximum,maximum,maximum}),"largest complete stamp");
    Require(authority.Commit({maximum,maximum,maximum,maximum},true),"max stamp accepted");
    Require(!authority.BeginFrame({maximum,1,0,0}),"wrapped counters rejected");
    Require(!authority.IsCurrent({maximum,maximum,maximum,maximum}),"wrap revokes old permission");
}
}
int main()
{
    ExactFrame();AttemptLifecycle();Sweep();
    std::cout<<"Experimental frame authority: exact stamps, epochs and 65536 observations passed\n";
}
