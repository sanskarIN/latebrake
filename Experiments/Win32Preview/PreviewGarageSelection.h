#pragma once
#include "PreviewGarageStyle.h"

namespace latebrake_preview
{
// V3/V4 settings persist EQUIPPED only. Showroom VIEWED is ephemeral.
// These pure functions have no file, money, ownership mutation or Win32 IO.
constexpr bool OwnsPreviewCar(unsigned int mask,int id) noexcept
{
    return ValidPreviewCar(id) &&
        (mask & (1u << static_cast<unsigned int>(id))) != 0u;
}
constexpr int SafeEquippedPreviewCar(int desired,unsigned int ownedMask) noexcept
{
    if (OwnsPreviewCar(ownedMask,desired)) return desired;
    for(int car=0;car<PreviewCarCount;++car)
        if (OwnsPreviewCar(ownedMask,car)) return car;
    // Real valid careers always own 0. Empty corrupt masks fall back to
    // the first free car, never an invented paid profile.
    return 0;
}
struct PreviewGarageSelection final
{
    int Equipped=0;
    int Viewed=0;
    constexpr void Restore(unsigned int ownedMask,int savedCar) noexcept
    {
        Equipped=SafeEquippedPreviewCar(savedCar,ownedMask);
        Viewed=Equipped;
    }
    constexpr bool Browse(int id) noexcept
    {
        if (!ValidPreviewCar(id)) return false;
        Viewed=id;
        return true;
    }
    constexpr bool BrowseDelta(int delta) noexcept
    {
        if (!ValidPreviewCar(Viewed) || (delta!=1 && delta!=-1))
            return false;
        return Browse((Viewed+PreviewCarCount+delta)%PreviewCarCount);
    }
    constexpr bool EquipViewed(unsigned int ownedMask) noexcept
    {
        if (!OwnsPreviewCar(ownedMask,Viewed)) return false;
        Equipped=Viewed;
        return true;
    }
    constexpr bool Invariant(unsigned int ownedMask) const noexcept
    {
        return ValidPreviewCar(Viewed) &&
            OwnsPreviewCar(ownedMask,Equipped);
    }
};
static_assert(SafeEquippedPreviewCar(99,1u)==0,
    "Invalid future car IDs cannot become purchased selections.");
static_assert(SafeEquippedPreviewCar(8,1u<<8)==8,
    "Appended car IDs keep their legacy save identity.");
} // namespace latebrake_preview
