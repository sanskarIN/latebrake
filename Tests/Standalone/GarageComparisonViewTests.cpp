#include "Garage/GarageComparisonView.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "GarageComparisonView FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Close(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}
}

int main()
{
    using namespace latebrake;
    const GarageSnapshot fresh{};
    const auto catalog = BuildGarageCatalogView(fresh);
    const auto equipped = CompareGaragePerformance(catalog, 0);
    Check(equipped.Valid && Close(equipped.AccelerationPercent, 0.0) &&
          Close(equipped.GripPercent, 0.0) &&
          Close(equipped.TopSpeedPercent, 0.0),
          "equipped starter compares to itself at zero percent");

    const auto kestrel = CompareGaragePerformance(catalog, 1);
    Check(kestrel.Valid && Close(kestrel.AccelerationPercent, 12.0) &&
          Close(kestrel.GripPercent, -6.0) &&
          Close(kestrel.TopSpeedPercent, -2.0),
          "locked Kestrel compares theoretical multipliers, not ownership");
    const auto phantom = CompareGaragePerformance(catalog, 2);
    Check(phantom.Valid && Close(phantom.AccelerationPercent, -4.0) &&
          Close(phantom.GripPercent, 12.0) &&
          Close(phantom.TopSpeedPercent, 8.0),
          "positive/negative relative tuning is labeled accurately");

    const auto upgraded = BuildGarageCatalogView({123, 0b111111, 4, 3, 2});
    const auto self = CompareGaragePerformance(upgraded, 4);
    Check(self.Valid && Close(self.AccelerationPercent, 0.0) &&
          Close(self.GripPercent, 0.0) &&
          Close(self.TopSpeedPercent, 0.0),
          "owned equipped ID may differ from zero after save load");
    for (int id = 0; id < VehicleCount; ++id)
    {
        const auto comparison = CompareGaragePerformance(upgraded, id);
        const auto& a = upgraded.Cars[static_cast<std::size_t>(id)].Effective;
        const auto& b = upgraded.Cars[4].Effective;
        Check(comparison.Valid &&
              Close(comparison.AccelerationPercent,
                    (a.Acceleration / b.Acceleration - 1.0) * 100.0) &&
              Close(comparison.GripPercent,
                    (a.Grip / b.Grip - 1.0) * 100.0) &&
              Close(comparison.TopSpeedPercent,
                    (a.TopSpeed / b.TopSpeed - 1.0) * 100.0),
              "relative values come from current shared tuning preview");
    }

    Check(!CompareGaragePerformance(catalog, -1).Valid &&
          !CompareGaragePerformance(catalog, VehicleCount).Valid,
          "invalid row IDs cannot access outside the catalogue");
    auto corrupt = catalog;
    corrupt.SelectedVehicle = VehicleCount;
    Check(!CompareGaragePerformance(corrupt, 0).Valid,
          "invalid equipped ID fails closed");
    corrupt = catalog;
    corrupt.Cars[2].VehicleId = 5;
    Check(!CompareGaragePerformance(corrupt, 2).Valid,
          "spoofed candidate identity fails closed");
    corrupt = catalog;
    corrupt.Cars[0].Effective.TopSpeed = 0.0;
    Check(!CompareGaragePerformance(corrupt, 2).Valid,
          "invalid divisor never produces apparent performance");
    corrupt = catalog;
    corrupt.Cars[2].Effective.Grip =
        std::numeric_limits<double>::quiet_NaN();
    Check(!CompareGaragePerformance(corrupt, 2).Valid,
          "NaN performance is suppressed");
    corrupt = catalog;
    corrupt.Cars[0].Effective.Acceleration =
        std::numeric_limits<double>::min();
    corrupt.Cars[2].Effective.Acceleration =
        std::numeric_limits<double>::max();
    Check(!CompareGaragePerformance(corrupt, 2).Valid,
          "overflow ratio is suppressed");
    for (int i = 0; i < 10000; ++i)
    {
        const auto result = CompareGaragePerformance(catalog, i % VehicleCount);
        Check(result.Valid, "repeated UI inspection has no stateful drift");
    }

    const auto engine = PreviewGarageUpgrade(100, 0, UpgradeKind::Engine);
    Check(engine.Status == GarageUpgradeOfferStatus::NeedCredits &&
          engine.Cost == 200 && engine.CreditsShort == 100,
          "starter engine price and shortage are read-only");
    const auto grip = PreviewGarageUpgrade(150, 0, UpgradeKind::Grip);
    Check(grip.Status == GarageUpgradeOfferStatus::Affordable &&
          grip.Cost == 150 && grip.CreditsShort == 0,
          "starter grip offer shows affordability");
    Check(PreviewGarageUpgrade(5000, 2, UpgradeKind::Engine).Cost == 500 &&
          PreviewGarageUpgrade(5000, 2, UpgradeKind::Grip).Cost == 400,
          "next cost follows existing tier price function");
    for (auto kind : {UpgradeKind::Engine, UpgradeKind::Grip})
    {
        const auto maxed = PreviewGarageUpgrade(0, 3, kind);
        Check(maxed.Status == GarageUpgradeOfferStatus::Maxed &&
              maxed.Cost == 0 && maxed.CreditsShort == 0,
              "maxed upgrade is never misleadingly FREE");
        Check(PreviewGarageUpgrade(-1, 0, kind).Status ==
                  GarageUpgradeOfferStatus::Invalid &&
              PreviewGarageUpgrade(0, -1, kind).Status ==
                  GarageUpgradeOfferStatus::Invalid &&
              PreviewGarageUpgrade(0, 4, kind).Status ==
                  GarageUpgradeOfferStatus::Invalid,
              "malformed credits or level never advertises purchasability");
    }
    Check(PreviewGarageUpgrade(999, 0, static_cast<UpgradeKind>(44)).Status ==
              GarageUpgradeOfferStatus::Invalid,
          "unknown upgrade kind fails closed");
    Check(fresh.Credits == 0 && fresh.OwnedVehicleMask == 1 &&
          fresh.SelectedVehicle == 0,
          "comparison never mutates original save snapshot");
    std::cout << "Garage comparison and upgrade preview tests passed.\n";
    return EXIT_SUCCESS;
}
