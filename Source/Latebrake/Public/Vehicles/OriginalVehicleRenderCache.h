#pragma once
#include "Vehicles/OriginalVehicleIndexedRender.h"
#include "Vehicles/OriginalVehicleShowroomCamera.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

namespace latebrake::vehiclemesh
{
// CPU-only, per-pawn, bounded preparation cache. No Unreal objects,
// material instances, race state or live GPU resources are retained.
// A hit skips the expensive composer/shading/index audit, not the native
// component upload. Geometry keys deliberately exclude paint and stripes.
inline constexpr std::size_t OriginalRenderCacheCapacity=3u;
inline constexpr int OriginalRenderCacheVehicles=6;
static_assert(OriginalRenderCacheVehicles==latebrake::VehicleCount);

struct OriginalRenderKey final
{
    int VehicleId=-1;
    int RimStyle=-1;
    constexpr bool Valid() const noexcept
    {
        return VehicleId>=0&&VehicleId<OriginalRenderCacheVehicles&&
            RimStyle>=0&&RimStyle<OriginalVehicleRimStyleCount;
    }
    constexpr bool operator==(const OriginalRenderKey&) const noexcept=default;
};

struct OriginalRenderCacheStats final
{
    std::uint32_t Hits=0;
    std::uint32_t Misses=0;
    std::uint32_t Prepared=0;
    std::uint32_t Failed=0;
    std::uint32_t Evictions=0;
    std::uint32_t Rejected=0;
    std::size_t ResidentEntries=0;
    // Logical active elements, not allocator capacity, process RSS or VRAM.
    std::size_t ResidentVertices=0;
    std::size_t ResidentIndices=0;
    std::size_t ResidentCameraPoints=0;
};

inline void IncrementOriginalRenderCounter(std::uint32_t& value) noexcept
{
    if(value<std::numeric_limits<std::uint32_t>::max())++value;
}

// Factory-only payload. Callers cannot modify its validated vertex
// stream or swap geometry across identities after it has been prepared.
class OriginalPreparedVehicleRender final
{
public:
    OriginalPreparedVehicleRender()=default;
    OriginalPreparedVehicleRender(const OriginalPreparedVehicleRender&)=delete;
    OriginalPreparedVehicleRender& operator=(const OriginalPreparedVehicleRender&)=delete;
    OriginalPreparedVehicleRender(OriginalPreparedVehicleRender&&)=default;
    OriginalPreparedVehicleRender& operator=(OriginalPreparedVehicleRender&&)=default;

    static OriginalPreparedVehicleRender Prepare(
        const OriginalVehicleMesh& mesh,const OriginalSurfaceShading& shading,
        const OriginalVehicleAnimatedParts& animated)
    {
        OriginalPreparedVehicleRender candidate;
        auto packed=BuildOriginalIndexedRender(mesh,shading,animated);
        if(!packed.Valid||packed.IndexCount!=mesh.Triangles.size()*3u)return {};
        OriginalVehicleShowroomGeometry camera;
        if(!camera.Build(mesh)||camera.VehicleId()!=mesh.VehicleId)return {};
        candidate.key_={mesh.VehicleId,mesh.RimStyle};
        candidate.packed_=std::move(packed);
        candidate.camera_=std::move(camera);
        return candidate;
    }

    bool Valid() const noexcept
    {
        // Also detects moved-from payloads. A valid source has nonempty
        // camera points; the private fields cannot be forged by a caller.
        return key_.Valid()&&packed_.Valid&&packed_.VehicleId==key_.VehicleId&&
            packed_.RimStyle==key_.RimStyle&&packed_.PackedVertices>0u&&
            packed_.IndexCount>0u&&camera_.Ready()&&
            camera_.VehicleId()==key_.VehicleId&&!camera_.Points().empty();
    }
    OriginalRenderKey Key() const noexcept {return key_;}
    const OriginalIndexedRender& Packed() const noexcept {return packed_;}
    const OriginalVehicleShowroomGeometry& CameraGeometry() const noexcept
    {return camera_;}

private:
    OriginalRenderKey key_{};
    OriginalIndexedRender packed_{};
    OriginalVehicleShowroomGeometry camera_{};
};

class OriginalVehicleRenderCache final
{
public:
    OriginalVehicleRenderCache()=default;
    OriginalVehicleRenderCache(const OriginalVehicleRenderCache&)=delete;
    OriginalVehicleRenderCache& operator=(const OriginalVehicleRenderCache&)=delete;

    // Builder executes synchronously exactly once on a valid miss. A
    // failed/mismatched preparation leaves the resident entries and
    // recency order unchanged. Returned immutable pointer is for the
    // current synchronous upload ONLY (may be evicted on a future call).
    // No cross-thread sharing, callbacks to gameplay or disk persistence.
    template<class Builder>
    const OriginalPreparedVehicleRender* Acquire(OriginalRenderKey key,Builder&& build)
    {
        if(!key.Valid())
        {
            IncrementOriginalRenderCounter(stats_.Rejected);
            return nullptr;
        }
        for(std::size_t i=0;i<entries_.size();++i)
        {
            if(!entries_[i]||entries_[i]->Key()!=key)continue;
            IncrementOriginalRenderCounter(stats_.Hits);
            Promote(i);
            return entries_[0].get();
        }
        IncrementOriginalRenderCounter(stats_.Misses);
        auto candidate=std::forward<Builder>(build)(key);
        if(!candidate.Valid()||candidate.Key()!=key)
        {
            IncrementOriginalRenderCounter(stats_.Failed);
            return nullptr;
        }
        // Allocate the replacement BEFORE evicting; validation failure
        // cannot destroy a working preview. The last slot is the LRU.
        auto prepared=std::make_unique<const OriginalPreparedVehicleRender>(
            std::move(candidate));
        if(entries_.back())IncrementOriginalRenderCounter(stats_.Evictions);
        for(std::size_t i=entries_.size()-1u;i>0u;--i)
            entries_[i]=std::move(entries_[i-1u]);
        entries_[0]=std::move(prepared);
        IncrementOriginalRenderCounter(stats_.Prepared);
        return entries_[0].get();
    }

    bool Contains(OriginalRenderKey key) const noexcept
    {
        if(!key.Valid())return false;
        for(const auto& entry:entries_)
            if(entry&&entry->Key()==key)return true;
        return false;
    }

    OriginalRenderCacheStats Stats() const noexcept
    {
        auto out=stats_;
        for(const auto& entry:entries_)
        {
            if(!entry)continue;
            ++out.ResidentEntries;
            out.ResidentVertices+=entry->Packed().PackedVertices;
            out.ResidentIndices+=entry->Packed().IndexCount;
            out.ResidentCameraPoints+=entry->CameraGeometry().Points().size();
        }
        return out;
    }

    void Reset() noexcept
    {
        for(auto& entry:entries_)entry.reset();
        stats_={};
    }

private:
    void Promote(std::size_t index) noexcept
    {
        if(index==0u)return;
        auto hit=std::move(entries_[index]);
        for(std::size_t i=index;i>0u;--i)
            entries_[i]=std::move(entries_[i-1u]);
        entries_[0]=std::move(hit);
    }
    std::array<std::unique_ptr<const OriginalPreparedVehicleRender>,
        OriginalRenderCacheCapacity> entries_{};
    OriginalRenderCacheStats stats_{};
};
} // namespace latebrake::vehiclemesh
