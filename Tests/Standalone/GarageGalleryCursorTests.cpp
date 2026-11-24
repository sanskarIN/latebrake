#include "Garage/GarageGalleryCursor.h"
#include "Garage/GarageCatalogView.h"
#include "Garage/GarageAppearance.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Verify(bool result, const char* name)
{
    if (!result)
    {
        std::cerr << "Garage gallery: " << name << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    GarageGalleryCursor cursor;
    GarageState authority;
    authority.Load({155, 1, 0, 2, 1});
    const GarageSnapshot before = authority.Snapshot();
    Verify(cursor.Viewed() == 0 && cursor.IsEquipped(0) && cursor.IsValid(),
           "starts with owned factory starter");
    Verify(CanEquipGalleryVehicle(authority, cursor),
           "owned starter can be equipped from gallery");
    for (int i = 0; i < VehicleCount; ++i)
    {
        Verify(cursor.Viewed() == i, "forward browsing views exact immutable IDs");
        const auto catalog = BuildGarageCatalogView(authority.Snapshot());
        const auto& entry = catalog.Cars[static_cast<std::size_t>(cursor.Viewed())];
        Verify(entry.VehicleId == i && entry.Name[0] != '\0',
               "every locked or owned row has readable catalogue identity");
        if (i > 0)
        {
            Verify(entry.Ownership != GarageCatalogOwnership::Equipped,
                   "viewing locked cars does not equip them");
            Verify(!CanEquipGalleryVehicle(authority, cursor),
                   "locked car preview cannot be equipped or charged");
        }
        cursor.Next();
    }
    Verify(cursor.Viewed() == 0 && cursor.Previous() == VehicleCount-1,
           "last/first wrap without out-of-bounds access");
    for (int i = VehicleCount-1; i >= 0; --i)
    {
        Verify(cursor.Viewed() == i, "reverse browsing retains ordering");
        cursor.Previous();
    }
    Verify(cursor.Viewed() == VehicleCount-1,
           "reverse cursor wraps to final row");

    for (int invalid : {-200, -1, VehicleCount,
                        std::numeric_limits<int>::max()})
    {
        cursor.Reset(invalid);
        Verify(cursor.Viewed() == 0, "bad saved/current IDs fall back to starter");
    }
    for (int id = 0; id < VehicleCount; ++id)
    {
        cursor.Reset(id);
        Verify(cursor.Viewed() == id && cursor.IsEquipped(id),
               "valid owned selection can synchronize showroom focus");
    }

    // Six direct showroom jumps must not change selection, credits or tuning.
    for(int id=0;id<VehicleCount;id++)
    {
        Verify(cursor.Browse(id) && cursor.Viewed()==id,
            "valid direct car preview picks exact fixed vehicle ID");
        const auto now=authority.Snapshot();
        Verify(now.SelectedVehicle==before.SelectedVehicle &&
               now.Credits==before.Credits &&
               now.OwnedVehicleMask==before.OwnedVehicleMask,
            "direct preview never equips, charges or unlocks");
    }
    for(int invalid:{-3,-1,VehicleCount,9999})
    {
        const int old=cursor.Viewed();
        Verify(!cursor.Browse(invalid) && cursor.Viewed()==old,
            "invalid direct preview ID leaves prior view unchanged");
    }

    // Partial genuine ownership: viewer can directly equip ID 4 while
    // IDs 1, 2, 3 and 5 stay locked. Browsing alone still must not equip.
    GarageState partiallyOwned;
    partiallyOwned.Load({255, (1 << 4) | 1, 0, 0, 0});
    cursor.Reset(4);
    Verify(CanEquipGalleryVehicle(partiallyOwned, cursor) &&
           partiallyOwned.SelectedVehicle() == 0,
           "owned browse target is eligible but not yet equipped");
    // Parked-car styling is not car selection. The UI can target the
    // owned browsed ID while the real selected car remains unchanged.
    GarageAppearanceState appearance;
    const GarageLook equippedBefore = appearance.Get(0);
    const GarageSnapshot styleBefore = partiallyOwned.Snapshot();
    Verify(CanStyleGalleryVehicle(partiallyOwned, cursor),
           "owned unequipped showroom car is style-eligible");
    Verify(appearance.Choose(cursor.Viewed(), GarageLookLayer::Paint, 7) &&
           appearance.Choose(cursor.Viewed(), GarageLookLayer::Stripe, 2) &&
           appearance.Choose(cursor.Viewed(), GarageLookLayer::Wheels, 2),
           "owned unequipped car can hold an independent complete style");
    Verify(appearance.Get(4).Paint == 7 &&
           appearance.Get(4).Stripe == 2 &&
           appearance.Get(4).Wheels == 2,
           "all three cosmetic layers target viewed owned car");
    Verify(appearance.Get(0).Paint == equippedBefore.Paint &&
           appearance.Get(0).Stripe == equippedBefore.Stripe &&
           appearance.Get(0).Wheels == equippedBefore.Wheels,
           "styling unequipped owned vehicle leaves equipped look intact");
    const GarageSnapshot styleAfter = partiallyOwned.Snapshot();
    Verify(styleAfter.Credits == styleBefore.Credits &&
           styleAfter.OwnedVehicleMask == styleBefore.OwnedVehicleMask &&
           styleAfter.SelectedVehicle == styleBefore.SelectedVehicle &&
           styleAfter.EngineLevel == styleBefore.EngineLevel &&
           styleAfter.GripLevel == styleBefore.GripLevel,
           "viewed-car customization never edits economy or selected driving car");
    GarageAppearanceState restoredAppearance;
    const auto packedAppearance = appearance.Serialize();
    Verify(restoredAppearance.Load(packedAppearance.data(), packedAppearance.size()) &&
           restoredAppearance.Get(4).Paint == 7 &&
           restoredAppearance.Get(0).Paint == equippedBefore.Paint,
           "parked-car look survives existing 18-byte save format");
    cursor.Reset(3);
    Verify(!CanEquipGalleryVehicle(partiallyOwned, cursor) &&
           !CanStyleGalleryVehicle(partiallyOwned, cursor) &&
           partiallyOwned.SelectedVehicle() == 0,
           "neighboring locked target can neither equip nor style");
    cursor.Reset(4);
    Verify(partiallyOwned.Select(cursor.Viewed()) &&
           partiallyOwned.SelectedVehicle() == 4,
           "authoritative GarageState still performs the actual selection");
    cursor.Reset(0);
    for (int frame = 0; frame < 300000; ++frame)
    {
        if ((frame & 1) == 0) cursor.Next();
        else cursor.Previous();
        Verify(cursor.IsValid(), "300k cycles never exceed fixed IDs");
        Verify(CanStyleGalleryVehicle(authority, cursor) ==
               (cursor.Viewed() == 0),
               "300k browse transitions only authorize style of owned car");
        const GarageSnapshot now = authority.Snapshot();
        Verify(now.Credits == before.Credits &&
               now.OwnedVehicleMask == before.OwnedVehicleMask &&
               now.SelectedVehicle == before.SelectedVehicle &&
               now.EngineLevel == before.EngineLevel &&
               now.GripLevel == before.GripLevel,
               "view-only browsing cannot mutate economy or tuning");
    }
    Verify(cursor.Viewed() == 0 && cursor.IsEquipped(0),
           "alternating gallery navigation is reversible");
    std::cout << "Garage gallery pure read-only policy passed.\n";
    return EXIT_SUCCESS;
}
