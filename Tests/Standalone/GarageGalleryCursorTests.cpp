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
    for (int i = 0; i < VehicleCount; ++i)
    {
        Verify(cursor.Viewed() == i, "forward browsing views exact immutable IDs");
        const auto catalog = BuildGarageCatalogView(authority.Snapshot());
        const auto& entry = catalog.Cars[static_cast<std::size_t>(cursor.Viewed())];
        Verify(entry.VehicleId == i && entry.Name[0] != '\0',
               "every locked or owned row has readable catalogue identity");
        if (i > 0)
            Verify(entry.Ownership != GarageCatalogOwnership::Equipped,
                   "viewing locked cars does not equip them");
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
