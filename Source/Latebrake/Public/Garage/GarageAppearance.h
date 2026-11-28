#pragma once

#include "Garage/GarageState.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Production Unreal starter garage: six stable SaveGame vehicle IDs.
// This is deliberately independent of the separate twelve-car 2D trial.
// Appearance has NO physics, purchase, award, ownership or race authority.
enum class GarageLookLayer : std::uint8_t
{
    Paint, Stripe, Wheels, Preset
};

inline constexpr int GaragePaintOptions = 8;
inline constexpr int GarageStripeOptions = 3;
inline constexpr int GarageWheelOptions = 3;
inline constexpr int GaragePresetOptions = 6;

inline constexpr const char* GaragePaintNames[GaragePaintOptions] = {
    "SKY BLUE", "EMBER ORANGE", "ELECTRIC VIOLET", "RACING SCARLET",
    "SEA GLASS", "SOLAR YELLOW", "GRAPHITE", "PEARL WHITE"
};
inline constexpr const char* GarageStripeNames[GarageStripeOptions] = {
    "CLEAN", "CENTER", "TWIN"
};
inline constexpr const char* GarageWheelNames[GarageWheelOptions] = {
    "STREET", "SPORT", "TRACK"
};

struct GarageLook final
{
    std::uint8_t Paint = 0;
    std::uint8_t Stripe = 0;
    std::uint8_t Wheels = 0;
};

inline constexpr bool ValidGarageLook(GarageLook look) noexcept
{
    return look.Paint < GaragePaintOptions &&
           look.Stripe < GarageStripeOptions &&
           look.Wheels < GarageWheelOptions;
}

inline constexpr GarageLook FactoryGarageLook(int id) noexcept
{
    return {static_cast<std::uint8_t>(
        GarageState::IsVehicle(id) ? id : 0), 0u, 0u};
}
// Value equality is shared by factory-reset idempotence and by the UI
// presentation. Never use bytewise struct comparisons (padding may differ).
inline constexpr bool SameGarageLook(GarageLook a, GarageLook b) noexcept
{
    return a.Paint == b.Paint && a.Stripe == b.Stripe &&
           a.Wheels == b.Wheels;
}
// Complete cosmetic combinations use existing persisted indices. No new
// save fields, materials, unlocks, costs, physics or tuning are introduced.
struct GarageLookPreset final
{
    const char* Name;
    const char* ShortName;
    GarageLook Look;
};
inline constexpr std::array<GarageLookPreset,GaragePresetOptions> GarageLookPresets{{
    {"MIDNIGHT TRACK","MIDNIGHT",{6,2,2}},
    {"PEARL TOURING","PEARL",{7,0,1}},
    {"SCARLET SPRINT","SCARLET",{3,1,1}},
    {"SOLAR CLUB","SOLAR",{5,2,0}},
    {"SEA GLASS GT","SEA GLASS",{4,1,2}},
    {"VIOLET SPORT","VIOLET",{2,0,2}}
}};

inline constexpr int GarageLookPresetIndex(GarageLook look) noexcept
{
    if(!ValidGarageLook(look))return -1;
    for(int i=0;i<GaragePresetOptions;++i)
        if(SameGarageLook(look,GarageLookPresets[static_cast<std::size_t>(i)].Look))
            return i;
    return -1; // hand-customized or factory look may not match a preset
}

inline constexpr bool ValidGarageLookPresets() noexcept
{
    for(std::size_t i=0;i<GarageLookPresets.size();++i)
    {
        if(!ValidGarageLook(GarageLookPresets[i].Look))return false;
        for(std::size_t j=0;j<i;++j)
            if(SameGarageLook(GarageLookPresets[i].Look,GarageLookPresets[j].Look))return false;
    }
    return true;
}
static_assert(ValidGarageLookPresets());

inline constexpr int GarageLookChoiceCount(GarageLookLayer layer) noexcept
{
    switch (layer)
    {
    case GarageLookLayer::Paint: return GaragePaintOptions;
    case GarageLookLayer::Stripe: return GarageStripeOptions;
    case GarageLookLayer::Wheels: return GarageWheelOptions;
    case GarageLookLayer::Preset: return GaragePresetOptions;
    default: return 0;
    }
}
inline constexpr const char* GarageLookChoiceName(
    GarageLookLayer layer, int index) noexcept
{
    if (index < 0 || index >= GarageLookChoiceCount(layer)) return "INVALID";
    switch (layer)
    {
    case GarageLookLayer::Paint: return GaragePaintNames[index];
    case GarageLookLayer::Stripe: return GarageStripeNames[index];
    case GarageLookLayer::Wheels: return GarageWheelNames[index];
    case GarageLookLayer::Preset: return GarageLookPresets[static_cast<std::size_t>(index)].Name;
    default: return "INVALID";
    }
}

class GarageAppearanceState final
{
public:
    static constexpr int SaveVersion = 1;
    static constexpr std::size_t SerializedBytes =
        static_cast<std::size_t>(VehicleCount) * 3u;
    using Packed = std::array<std::uint8_t, SerializedBytes>;

    GarageAppearanceState() noexcept { Reset(); }

    void Reset() noexcept
    {
        for (int id = 0; id < VehicleCount; ++id)
            Styles[static_cast<std::size_t>(id)] = FactoryGarageLook(id);
    }

    GarageLook Get(int id) const noexcept
    {
        return GarageState::IsVehicle(id)
            ? Styles[static_cast<std::size_t>(id)] : FactoryGarageLook(0);
    }

    bool Choose(int id, GarageLookLayer layer, int choice) noexcept
    {
        if (!GarageState::IsVehicle(id) || choice < 0 ||
            choice >= GarageLookChoiceCount(layer)) return false;
        GarageLook& look = Styles[static_cast<std::size_t>(id)];
        if (layer == GarageLookLayer::Paint)
            look.Paint = static_cast<std::uint8_t>(choice);
        else if (layer == GarageLookLayer::Stripe)
            look.Stripe = static_cast<std::uint8_t>(choice);
        else if (layer == GarageLookLayer::Wheels)
            look.Wheels = static_cast<std::uint8_t>(choice);
        else if (layer == GarageLookLayer::Preset)
            look = GarageLookPresets[static_cast<std::size_t>(choice)].Look;
        else return false;
        return true;
    }

    // Restore exactly one vehicle's original visual preset. Return true
    // ONLY for an actual change, so a repeated factory action does not
    // cause needless SaveGame writes or false success messages.
    // Ownership checks belong to GameMode; this is pure cosmetic state.
    bool ResetVehicle(int id) noexcept
    {
        if (!GarageState::IsVehicle(id)) return false;
        GarageLook& current = Styles[static_cast<std::size_t>(id)];
        const GarageLook factory = FactoryGarageLook(id);
        if (SameGarageLook(current, factory)) return false;
        current = factory;
        return true;
    }

    bool Cycle(int id, GarageLookLayer layer) noexcept
    {
        const int count = GarageLookChoiceCount(layer);
        if (!GarageState::IsVehicle(id) || count <= 0) return false;
        const GarageLook look = Get(id);
        const int selected = layer == GarageLookLayer::Paint ? look.Paint :
            layer == GarageLookLayer::Stripe ? look.Stripe :
            layer == GarageLookLayer::Wheels ? look.Wheels : GarageLookPresetIndex(look);
        return Choose(id, layer, (selected + 1) % count);
    }

    Packed Serialize() const noexcept
    {
        Packed result{};
        for (int id = 0; id < VehicleCount; ++id)
        {
            const GarageLook look = Styles[static_cast<std::size_t>(id)];
            const std::size_t offset = static_cast<std::size_t>(id) * 3u;
            result[offset] = look.Paint;
            result[offset + 1u] = look.Stripe;
            result[offset + 2u] = look.Wheels;
        }
        return result;
    }

    // Reject the ENTIRE corrupt/partial record and restore ONLY cosmetics.
    // Callers must also check the schema version and checksum before Load.
    bool Load(const std::uint8_t* raw, std::size_t count) noexcept
    {
        if (!raw || count != SerializedBytes)
        {
            Reset();
            return false;
        }
        std::array<GarageLook, VehicleCount> candidate{};
        for (int id = 0; id < VehicleCount; ++id)
        {
            const std::size_t offset = static_cast<std::size_t>(id) * 3u;
            const GarageLook look{raw[offset], raw[offset+1u], raw[offset+2u]};
            if (!ValidGarageLook(look))
            {
                Reset();
                return false;
            }
            candidate[static_cast<std::size_t>(id)] = look;
        }
        Styles = candidate;
        return true;
    }

    static constexpr std::uint32_t Checksum(const Packed& raw) noexcept
    {
        std::uint32_t hash = (2166136261u ^ SaveVersion) * 16777619u;
        for (std::uint8_t byte : raw)
            hash = (hash ^ byte) * 16777619u;
        return hash;
    }

private:
    std::array<GarageLook, VehicleCount> Styles{};
};
static_assert(GarageAppearanceState::SerializedBytes == 18u,
    "Review appearance migration when Unreal vehicle ID count changes.");
} // namespace latebrake
