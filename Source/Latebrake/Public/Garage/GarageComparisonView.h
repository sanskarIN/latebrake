#pragma once

#include "Garage/GarageCatalogView.h"

#include <cmath>
#include <cstddef>

namespace latebrake
{
// UI-only relative figures for *already-sanitized* GarageCatalogView rows.
// This is a comparison of configured arcade multipliers, NOT measured vehicle
// acceleration, real top speed, new physics authority, or owned-car rights.
struct GaragePerformanceComparison final
{
    bool Valid = false;
    double AccelerationPercent = 0.0;
    double GripPercent = 0.0;
    double TopSpeedPercent = 0.0;
};

inline bool ValidComparisonPerformance(VehiclePerformance p) noexcept
{
    return std::isfinite(p.Acceleration) && p.Acceleration > 0.0 &&
           std::isfinite(p.Grip) && p.Grip > 0.0 &&
           std::isfinite(p.TopSpeed) && p.TopSpeed > 0.0;
}

inline GaragePerformanceComparison CompareGaragePerformance(
    const GarageCatalogView& catalog, int vehicleId) noexcept
{
    if (!GarageState::IsVehicle(vehicleId) ||
        !GarageState::IsVehicle(catalog.SelectedVehicle))
    {
        return {};
    }
    const GarageCatalogEntry& equipped =
        catalog.Cars[static_cast<std::size_t>(catalog.SelectedVehicle)];
    const GarageCatalogEntry& candidate =
        catalog.Cars[static_cast<std::size_t>(vehicleId)];
    // A caller with a mixed/corrupt snapshot cannot label unrelated
    // performance data as a known car's tuning preview.
    if (equipped.VehicleId != catalog.SelectedVehicle ||
        candidate.VehicleId != vehicleId ||
        !ValidComparisonPerformance(equipped.Effective) ||
        !ValidComparisonPerformance(candidate.Effective))
    {
        return {};
    }

    const double acceleration =
        (candidate.Effective.Acceleration / equipped.Effective.Acceleration - 1.0) * 100.0;
    const double grip =
        (candidate.Effective.Grip / equipped.Effective.Grip - 1.0) * 100.0;
    const double topSpeed =
        (candidate.Effective.TopSpeed / equipped.Effective.TopSpeed - 1.0) * 100.0;
    if (!std::isfinite(acceleration) || !std::isfinite(grip) ||
        !std::isfinite(topSpeed))
    {
        return {};
    }
    return {true, acceleration, grip, topSpeed};
}

// A cost preview cannot authorize upgrades. Actual purchases still require
// the existing GameMode confirmation, GarageState checks and save rollback.
enum class GarageUpgradeOfferStatus
{
    Invalid, Affordable, NeedCredits, Maxed
};

struct GarageUpgradeOffer final
{
    GarageUpgradeOfferStatus Status = GarageUpgradeOfferStatus::Invalid;
    int Cost = 0;
    int CreditsShort = 0;
};

inline GarageUpgradeOffer PreviewGarageUpgrade(
    int credits, int currentLevel, UpgradeKind kind) noexcept
{
    if (credits < 0 || currentLevel < 0 || currentLevel > 3 ||
        (kind != UpgradeKind::Engine && kind != UpgradeKind::Grip))
    {
        return {};
    }
    const int cost = GarageState::UpgradeCost(kind, currentLevel);
    if (cost == 0)
    {
        return {GarageUpgradeOfferStatus::Maxed, 0, 0};
    }
    if (credits >= cost)
    {
        return {GarageUpgradeOfferStatus::Affordable, cost, 0};
    }
    return {GarageUpgradeOfferStatus::NeedCredits, cost, cost - credits};
}
} // namespace latebrake
