#include "Garage/GarageGalleryCursor.h"
#include "Garage/GarageCatalogView.h"

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

    // Partial genuine ownership: viewer can directly equip ID 4 while
    // IDs 1, 2, 3 and 5 stay locked. Browsing alone still must not equip.
    GarageState partiallyOwned;
    partiallyOwned.Load({255, (1 << 4) | 1, 0, 0, 0});
    cursor.Reset(4);
    Verify(CanEquipGalleryVehicle(partiallyOwned, cursor) &&
           partiallyOwned.SelectedVehicle() == 0,
           "owned browse target is eligible but not yet equipped");
    cursor.Reset(3);
    Verify(!CanEquipGalleryVehicle(partiallyOwned, cursor) &&
           partiallyOwned.SelectedVehicle() == 0,
           "neighboring locked target is ineligible");
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
