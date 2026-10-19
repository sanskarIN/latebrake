#include "Garage/GarageTuningKey.h"
#include <cstdlib>
#include <iostream>

static void Require(bool value, const char* description)
{
    if (!value)
    {
        std::cerr << "Garage tuning FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    GarageState garage;
    const auto base = MakeGarageTuningKey(garage.Snapshot());
    Require(base == MakeGarageTuningKey(garage.Snapshot()), "same garage key");
    garage.EarnCredits(1800);
    Require(base == MakeGarageTuningKey(garage.Snapshot()),
            "awarded credits do not require a physics reapply");
    Require(garage.Unlock(1), "unlock Kestrel");
    Require(base == MakeGarageTuningKey(garage.Snapshot()),
            "unlocks alone do not affect current handling");
    Require(garage.Select(1), "select Kestrel");
    const auto selected = MakeGarageTuningKey(garage.Snapshot());
    Require(!(base == selected), "selection changes physics tuning");
    Require(garage.Upgrade(UpgradeKind::Engine), "engine upgrade");
    const auto engine = MakeGarageTuningKey(garage.Snapshot());
    Require(!(engine == selected), "engine upgrade changes physics tuning");
    Require(garage.Upgrade(UpgradeKind::Grip), "grip upgrade");
    Require(!(engine == MakeGarageTuningKey(garage.Snapshot())),
            "grip upgrade changes physics tuning");
    GarageTuningKey unset;
    Require(!(unset == base), "unset sentinel never matches default car");
    std::cout << "Garage tuning cache tests passed.\n";
    return EXIT_SUCCESS;
}
