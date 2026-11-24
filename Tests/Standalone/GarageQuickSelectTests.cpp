#include "Garage/GarageQuickSelect.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "GarageQuickSelect FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Same(latebrake::GarageSnapshot a, latebrake::GarageSnapshot b)
{
    return a.Credits == b.Credits &&
        a.OwnedVehicleMask == b.OwnedVehicleMask &&
        a.SelectedVehicle == b.SelectedVehicle &&
        a.EngineLevel == b.EngineLevel &&
        a.GripLevel == b.GripLevel;
}
}

int main()
{
    using namespace latebrake;
    const GarageSnapshot starter{};
    const auto choose0 = PreviewGarageQuickSelect(starter, 0, true, true, true);
    const auto locked5 = PreviewGarageQuickSelect(starter, 5, true, true, true);
    Check(choose0.Status == GarageQuickSelectStatus::AlreadyEquipped &&
          choose0.VehicleId == 0, "default Arrow is already equipped");
    Check(locked5.Status == GarageQuickSelectStatus::NotOwned &&
          locked5.VehicleId == 5, "locked Nova is never considered selectable");

    for (int id = -10; id <= 20; ++id)
    {
        Check(PreviewGarageQuickSelect(starter, id, false, true, true).Status ==
              GarageQuickSelectStatus::MenuClosed,
              "closed menu never accepts any slot");
        Check(PreviewGarageQuickSelect(starter, id, true, false, true).Status ==
              GarageQuickSelectStatus::WrongPage,
              "other menus never accept any garage slot");
        Check(PreviewGarageQuickSelect(starter, id, true, true, false).Status ==
              GarageQuickSelectStatus::CareerRaceLocked,
              "active career race refuses car changes");
        if (!GarageState::IsVehicle(id))
        {
            const auto invalid =
                PreviewGarageQuickSelect(starter, id, true, true, true);
            Check(invalid.Status == GarageQuickSelectStatus::InvalidVehicle &&
                  invalid.VehicleId == -1, "invalid slot fails closed");
        }
    }

    // Every six-car ownership combination, including non-contiguous unlocks,
    // all selected IDs and credits extremes must agree with GarageState.
    const int credits[] = {-5, 0, 1000, std::numeric_limits<int>::max()};
    for (int mask = -1; mask <= 127; ++mask)
    for (int savedSelected = -2; savedSelected <= 8; ++savedSelected)
    for (const int credit : credits)
    {
        GarageSnapshot before{
            credit, mask, savedSelected, 2, 3};
        GarageState authority;
        authority.Load(before);
        const GarageSnapshot normalizedBefore = authority.Snapshot();
        for (int id = 0; id < VehicleCount; ++id)
        {
            const auto decision =
                PreviewGarageQuickSelect(before, id, true, true, true);
            const auto expected = !authority.Owns(id)
                ? GarageQuickSelectStatus::NotOwned
                : id == authority.SelectedVehicle()
                    ? GarageQuickSelectStatus::AlreadyEquipped
                    : GarageQuickSelectStatus::Ready;
            Check(decision.Status == expected && decision.VehicleId == id,
                  "owned/equipped outcomes agree with sanitized garage");
        }
        Check(Same(authority.Snapshot(), normalizedBefore),
              "query never mutates normalized garage");
        Check(Same(before, GarageSnapshot{
                  credit, mask, savedSelected, 2, 3}),
              "query cannot mutate the supplied save snapshot");
    }

    Check(PreviewGarageQuickSelect({
              9999, 0x7fffffff, 5, 0, 0}, 5, true, true, true).Status ==
          GarageQuickSelectStatus::AlreadyEquipped,
          "unknown future ownership bits do not corrupt known vehicles");
    Check(PreviewGarageQuickSelect({
              9999, -1, 5, 0, 0}, 5, true, true, true).Status ==
          GarageQuickSelectStatus::NotOwned,
          "negative ownership bits never grant a locked car");
    std::cout << "Garage quick-select tests passed.\n";
    return EXIT_SUCCESS;
}
