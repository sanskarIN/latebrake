#include "Vehicles/SkidSegmentPool.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
void Require(bool pass,const char* why)
{
    if(!pass){std::cerr<<"SkidSegmentPool FAIL: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
}
int main()
{
    using namespace latebrake;
    SkidSegmentPool<16> small;
    const auto original=small.Insert();
    for(unsigned i=1;i<16;++i)Require(small.Insert().Slot==i,"initial renderer slots are contiguous");
    const auto replacement=small.Insert();
    Require(replacement.Slot==0&&replacement.Generation>original.Generation&&
        !small.IsCurrent(original)&&small.IsActive(replacement)&&small.ActiveCount()==16,
        "full ring overwrites one stable index and invalidates its old generation");
    SkidExpiryBatch expired;
    for(unsigned i=0;i<130;++i)Require(small.Advance(0.1,expired),"valid game time advances expiry scanning");
    Require(small.ActiveCount()==0&&!small.IsActive(replacement)&&small.IsCurrent(replacement),
        "expired identities are distinguishable from overwritten identities without index compaction");
    small.Reset();const auto fresh=small.Insert();
    Require(fresh.Generation>replacement.Generation&&!small.IsCurrent(replacement),
        "race reset does not reopen an older renderer generation");
    Require(!small.Advance(std::numeric_limits<double>::quiet_NaN(),expired)&&expired.Count==0,
        "invalid time cannot emit renderer retirement operations");
    SkidSegmentPool<> full;
    for(unsigned i=0;i<full.MaximumCount();++i)Require(full.Insert().Valid,"default fixed pool admits its exact capacity");
    Require(full.ActiveCount()==2048,"per-car pool never needs more than 2048 active strips");
    for(unsigned i=0;i<160;++i)
    {
        Require(full.Advance(0.1,expired)&&expired.Count<=64,"retirement work stays within the per-frame scan budget");
        for(unsigned j=0;j<expired.Count;++j)Require(full.IsCurrent(expired.Retired[j])&&
            !full.IsActive(expired.Retired[j]),"retired renderer operations refer to the current inactive generation");
    }
    Require(full.ActiveCount()==0,"all old marks expire within the bounded twelve-second plus scan-cycle delay");
    struct Ref{bool active=false;std::uint64_t generation=0;double expires=0.0;};
    std::array<Ref,32> reference{};SkidSegmentPool<32> pool;
    std::mt19937 rng(0x5b9811);double time=0;unsigned next=0;std::uint64_t generation=0;
    for(unsigned step=0;step<100000;++step)
    {
        if(step%977==0){pool.Reset();reference={};time=0;next=0;}
        const unsigned additions=rng()%5;
        for(unsigned n=0;n<additions;++n)
        {
            const auto id=pool.Insert();++generation;
            Require(id.Valid&&id.Slot==next&&id.Generation==generation,"randomized ring matches independent slot/generation reference");
            reference[next]={true,generation,time+12.0};next=(next+1)%32;
        }
        const double dt=step%3==0?0.1:0.01;time+=dt;
        unsigned expected=0;
        for(auto& r:reference)if(r.active&&r.expires<=time){r.active=false;++expected;}
        Require(pool.Advance(dt,expired)&&expired.Count==expected,"small-pool expiry matches independent complete reference scan");
        for(const auto& id:expired.Retired)
            if(id.Valid)Require(id.Slot<32&&reference[id.Slot].generation==id.Generation&&
                !reference[id.Slot].active,"random retirements never address an overwritten active mark");
        unsigned active=0;for(const auto& r:reference)if(r.active)++active;
        Require(pool.ActiveCount()==active,"100k interleaved insert/reset/expiry frames preserve exact active accounting");
    }
    std::cout<<"Skid pool: fixed memory, overwrite, expiry generations and 100000 reference frames passed\n";
}
