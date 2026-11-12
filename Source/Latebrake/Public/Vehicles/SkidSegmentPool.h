#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake
{
struct SkidSlotIdentity final
{
    bool Valid = false;
    unsigned Slot = 0;
    std::uint64_t Generation = 0;
};

struct SkidExpiryBatch final
{
    unsigned Count = 0;
    std::array<SkidSlotIdentity, 64> Retired{};
};

// GPU slot identities remain stable: no instance removal/compaction and
// no heap growth after the fixed capacity is reached. Expiry scans at most
// 64 slots per accepted frame; expired visuals can linger one scan cycle.
template<unsigned Capacity = 2048>
class SkidSegmentPool final
{
    static_assert(Capacity >= 16 && Capacity <= 8192);
public:
    static constexpr double LifetimeSeconds = 12.0;

    void Reset() noexcept
    {
        Slots = {};
        Time = 0.0;
        NextSlot = 0;
        ScanSlot = 0;
        Active = 0;
        // Generation survives reset, invalidating any old renderer work.
    }

    bool Advance(double seconds, SkidExpiryBatch& expired) noexcept
    {
        expired = {};
        if (!std::isfinite(seconds) || seconds <= 0.0 || seconds > 0.10 ||
            !std::isfinite(Time + seconds) || Time + seconds <= Time) return false;
        Time += seconds;
        const unsigned scanned = Capacity < 64 ? Capacity : 64;
        for (unsigned i = 0; i < scanned; ++i)
        {
            auto& item = Slots[ScanSlot];
            if (item.Active && item.Expires <= Time)
            {
                item.Active = false;
                --Active;
                expired.Retired[expired.Count++] = {true, ScanSlot, item.Generation};
            }
            ScanSlot = (ScanSlot + 1u) % Capacity;
        }
        return true;
    }

    SkidSlotIdentity Insert() noexcept
    {
        if (Generation == std::numeric_limits<std::uint64_t>::max()) return {};
        const unsigned slot = NextSlot;
        auto& item = Slots[slot];
        if (!item.Active) ++Active;
        item = {true, ++Generation, Time + LifetimeSeconds};
        NextSlot = (NextSlot + 1u) % Capacity;
        return {true, slot, item.Generation};
    }

    bool IsActive(SkidSlotIdentity id) const noexcept
    {
        return IsCurrent(id) && Slots[id.Slot].Active;
    }

    bool IsCurrent(SkidSlotIdentity id) const noexcept
    {
        return id.Valid && id.Slot < Capacity && id.Generation != 0 &&
            Slots[id.Slot].Generation == id.Generation;
    }

    unsigned ActiveCount() const noexcept { return Active; }
    static constexpr unsigned MaximumCount() noexcept { return Capacity; }

private:
    struct Slot final
    {
        bool Active = false;
        std::uint64_t Generation = 0;
        double Expires = 0.0;
    };
    std::array<Slot, Capacity> Slots{};
    double Time = 0.0;
    unsigned NextSlot = 0;
    unsigned ScanSlot = 0;
    unsigned Active = 0;
    std::uint64_t Generation = 0;
};
} // namespace latebrake
