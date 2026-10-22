#include "Garage/CareerGaragePolicy.h"
#include "Garage/GarageState.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool yes, const char* message)
{
    if (!yes)
    {
        std::cerr << "Career garage policy FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Same(const latebrake::GarageSnapshot& a, const latebrake::GarageSnapshot& b)
{
    return a.Credits == b.Credits &&
           a.OwnedVehicleMask == b.OwnedVehicleMask &&
           a.SelectedVehicle == b.SelectedVehicle &&
           a.EngineLevel == b.EngineLevel &&
           a.GripLevel == b.GripLevel;
}

enum class Action { NextVehicle, BuyVehicle, Engine, Grip };

bool Attempt(latebrake::GarageState& garage, bool career,
             const latebrake::RaceProgress& race, Action action)
{
    // Same early guard as the four Unreal game-mode hotkey handlers. A
    // rejected attempt MUST NOT even call the underlying garage mutator.
    if (!latebrake::CanEditGarageDuringRace(career, race)) return false;
    switch (action)
    {
    case Action::NextVehicle:
        return garage.Select(garage.NextOwnedVehicle());
    case Action::BuyVehicle:
        return garage.Unlock(garage.NextLockedVehicle());
    case Action::Engine:
        return garage.Upgrade(latebrake::UpgradeKind::Engine);
    case Action::Grip:
        return garage.Upgrade(latebrake::UpgradeKind::Grip);
    }
    return false;
}
}

int main()
{
    using namespace latebrake;
    GarageState garage;
    GarageSnapshot initial;
    initial.Credits = 3000;
    initial.OwnedVehicleMask = 3; // both Arrow and Kestrel already owned
    initial.SelectedVehicle = 0;
    garage.Load(initial);
    const GarageSnapshot before = garage.Snapshot();

    RaceProgress activeCareer(4, 2);
    Require(!CanEditGarageDuringRace(true, activeCareer),
            "live career event locks garage for race integrity");
    for (const auto action : {Action::NextVehicle, Action::BuyVehicle,
                             Action::Engine, Action::Grip})
    {
        Require(!Attempt(garage, true, activeCareer, action),
                "all four mutations denied in active career");
        Require(Same(garage.Snapshot(), before),
                "rejected garage shortcut never changes credits, ownership or tuning");
    }

    // Pausing/menu navigation doesn't finish a race; the source policy
    // must not accidentally unlock garage simply because HUD/menu is open.
    Require(!CanEditGarageDuringRace(true, activeCareer),
            "opening a pause/garage menu cannot bypass the active-race lock");

    // One gate is not the finish. The lock lasts across completed laps.
    for (int i = 0; i < 4; ++i)
        Require(activeCareer.PassCheckpoint(i), "complete ordered first lap");
    Require(activeCareer.GetCompletedLaps() == 1 &&
            !CanEditGarageDuringRace(true, activeCareer),
            "garage stays locked between laps");
    for (int i = 0; i < 4; ++i)
        Require(activeCareer.PassCheckpoint(i), "complete second lap");
    Require(activeCareer.IsFinished() &&
            CanEditGarageDuringRace(true, activeCareer),
            "career finish unlocks garage");

    // After the finish, all four same actions can change eligible state.
    Require(Attempt(garage, true, activeCareer, Action::NextVehicle) &&
            garage.SelectedVehicle() == 1,
            "post-finish owned vehicle change is available");
    Require(Attempt(garage, true, activeCareer, Action::BuyVehicle) &&
            garage.Owns(2) && garage.Credits() == 1800,
            "post-finish purchase performs normal debit");
    Require(Attempt(garage, true, activeCareer, Action::Engine) &&
            garage.EngineLevel() == 1 && garage.Credits() == 1600,
            "post-finish engine upgrade performs normal debit");
    Require(Attempt(garage, true, activeCareer, Action::Grip) &&
            garage.GripLevel() == 1 && garage.Credits() == 1450,
            "post-finish grip upgrade performs normal debit");

    const GarageSnapshot after = garage.Snapshot();
    activeCareer.Reset();
    Require(!CanEditGarageDuringRace(true, activeCareer),
            "restarting finished career event engages lock again");
    Require(!Attempt(garage, true, activeCareer, Action::NextVehicle) &&
            Same(garage.Snapshot(), after),
            "restart cannot switch vehicle even if others are now owned");

    // Standard quick-race garage controls are intentionally unchanged.
    Require(CanEditGarageDuringRace(false, activeCareer) &&
            Attempt(garage, false, activeCareer, Action::NextVehicle),
            "non-career quick race preserves previous garage controls");
    Require(garage.SelectedVehicle() != after.SelectedVehicle,
            "quick-race garage can change owned vehicle");

    std::cout << "Career garage transaction guards passed.\n";
    return EXIT_SUCCESS;
}
