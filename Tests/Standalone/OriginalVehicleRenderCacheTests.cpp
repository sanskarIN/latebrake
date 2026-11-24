#include "Vehicles/OriginalVehicleRenderCache.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake::vehiclemesh;
void Check(bool good,const char* why)
{
    if(!good){std::cerr<<"RenderCache: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
OriginalPreparedVehicleRender Build(OriginalRenderKey key)
{
    const auto built=OriginalVehicleSurfaceComposer{}.Build(key.VehicleId,key.RimStyle);
    if(!built.Valid)return {};
    const auto shading=BuildOriginalSurfaceShading(built.Mesh);
    const auto animated=MakeOriginalVehicleAnimatedParts(built.Mesh);
    return OriginalPreparedVehicleRender::Prepare(built.Mesh,shading,animated);
}
void LruAndFailure()
{
    OriginalVehicleRenderCache cache;
    unsigned calls=0;
    auto build=[&](OriginalRenderKey key){++calls;return Build(key);};
    const OriginalRenderKey a{0,0},b{1,1},c{2,2},d{3,0};
    const auto* first=cache.Acquire(a,build);
    Check(first&&first->Valid()&&first->Key()==a,"first miss creates validated payload");
    Check(cache.Acquire(b,build)&&cache.Acquire(c,build)&&calls==3u,
        "three different car/rim identities fill bounded cache");
    Check(cache.Acquire(a,build)==first&&calls==3u,
        "hit returns identical immutable prepared payload and promotes");
    Check(cache.Acquire(d,build)&&calls==4u&&!cache.Contains(b)&&
        cache.Contains(a)&&cache.Contains(c)&&cache.Contains(d),
        "least recently used entry evicted, promoted entry survives");
    auto state=cache.Stats();
    Check(state.Hits==1u&&state.Misses==4u&&state.Prepared==4u&&
        state.Evictions==1u&&state.ResidentEntries==3u,
        "telemetry separates actual preparations from hits and evictions");
    const auto failure=[&](OriginalRenderKey){++calls;return OriginalPreparedVehicleRender{};};
    Check(!cache.Acquire({5,2},failure)&&calls==5u,
        "failed preparation returns no payload");
    const auto wrong=[&](OriginalRenderKey){++calls;return Build({4,1});};
    Check(!cache.Acquire({5,2},wrong)&&calls==6u,
        "prepared payload for wrong identity is refused");
    Check(!cache.Acquire({-1,0},build)&&!cache.Acquire({0,3},build)&&calls==6u,
        "invalid keys never call builder");
    const auto after=cache.Stats();
    Check(after.Failed==2u&&after.Rejected==2u&&after.Evictions==1u&&
        after.ResidentEntries==3u&&after.ResidentVertices==state.ResidentVertices&&
        after.ResidentIndices==state.ResidentIndices&&
        after.ResidentCameraPoints==state.ResidentCameraPoints,
        "failed and invalid calls leave resident buffers unchanged");
    Check(cache.Acquire({5,2},build)&&!cache.Contains(c)&&cache.Contains(a),
        "failures did not alter least-recently-used order");
    cache.Reset();
    state=cache.Stats();
    Check(state.ResidentEntries==0u&&state.ResidentVertices==0u&&
        state.ResidentIndices==0u&&state.ResidentCameraPoints==0u&&
        state.Hits==0u&&state.Prepared==0u&&!cache.Contains(a),
        "reset releases all resident buffers and diagnostics");
}
void AllCarRimPairs()
{
    OriginalVehicleRenderCache cache;
    unsigned calls=0;
    const auto build=[&](OriginalRenderKey key){++calls;return Build(key);};
    for(int car=0;car<OriginalRenderCacheVehicles;++car)
    for(int rim=0;rim<OriginalVehicleRimStyleCount;++rim)
    {
        const OriginalRenderKey key{car,rim};
        const auto* data=cache.Acquire(key,build);
        Check(data&&data->Valid()&&data->Key()==key&&
            data->Packed().VehicleId==car&&data->Packed().RimStyle==rim&&
            data->CameraGeometry().VehicleId()==car&&
            data->Packed().SourceCorners==data->CameraGeometry().Points().size(),
            "packed render and camera agree for all 18 car/rim identities");
        const auto packedVertices=data->Packed().PackedVertices;
        for(int paint=0;paint<latebrake::GaragePaintOptions;++paint)
        for(int stripe=0;stripe<latebrake::GarageStripeOptions;++stripe)
        {
            const latebrake::GarageLook look{
                static_cast<std::uint8_t>(paint),static_cast<std::uint8_t>(stripe),
                static_cast<std::uint8_t>(rim)};
            const auto* same=cache.Acquire({car,static_cast<int>(look.Wheels)},build);
            Check(same==data&&same->Packed().PackedVertices==packedVertices,
                "paint and stripe identity do not trigger CPU geometry preparation");
        }
        const auto s=cache.Stats();
        Check(s.ResidentEntries<=OriginalRenderCacheCapacity&&
            s.ResidentVertices<=OriginalRenderCacheCapacity*OriginalVehicleMaxVertices&&
            s.ResidentIndices<=OriginalRenderCacheCapacity*OriginalVehicleMaxTriangles*3u&&
            s.ResidentCameraPoints<=OriginalRenderCacheCapacity*OriginalVehicleMaxVertices,
            "logical memory footprint bounded across all geometry choices");
    }
    const auto s=cache.Stats();
    Check(calls==18u&&s.Prepared==18u&&s.Misses==18u&&
        s.Hits==18u*static_cast<unsigned>(latebrake::GaragePaintOptions)*
            static_cast<unsigned>(latebrake::GarageStripeOptions)&&
        s.Evictions==18u-OriginalRenderCacheCapacity,
        "all 18 geometry keys and cosmetic variants counted accurately");
    std::cout<<"18 original car/rim preparations; "<<s.Hits
        <<" cosmetic cache hits; "<<s.Evictions<<" bounded LRU evictions\n";
}
void PayloadAndCounters()
{
    auto source=OriginalVehicleSurfaceComposer{}.Build(2,1);
    const auto shading=BuildOriginalSurfaceShading(source.Mesh);
    const auto parts=MakeOriginalVehicleAnimatedParts(source.Mesh);
    auto good=OriginalPreparedVehicleRender::Prepare(source.Mesh,shading,parts);
    Check(good.Valid(),"prepared data is valid before move");
    auto moved=std::move(good);
    Check(moved.Valid()&&!good.Valid(),"moved-from payload cannot masquerade as valid");
    source.Mesh.Bounds.Maximum.X+=1.0;
    Check(!OriginalPreparedVehicleRender::Prepare(source.Mesh,shading,parts).Valid(),
        "forged camera bounds do not produce a cacheable payload");
    std::uint32_t count=std::numeric_limits<std::uint32_t>::max()-1u;
    IncrementOriginalRenderCounter(count);IncrementOriginalRenderCounter(count);
    Check(count==std::numeric_limits<std::uint32_t>::max(),"diagnostics saturate");
}
}
int main()
{
    LruAndFailure();AllCarRimPairs();PayloadAndCounters();
    return EXIT_SUCCESS;
}
