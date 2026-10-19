#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Original fictional starter vehicles. The v0.3.0 milestone supplies tuning
// profiles only: original car meshes, wheel rigs and art are not yet shipped.
struct VehicleSpec final
{
    int Id;
    const char* Name;
    int UnlockCost;
    double Acceleration;
    double Grip;
    double TopSpeed;
};

inline constexpr VehicleSpec Vehicles[] =
{
    {0, "Arrow", 0,    1.00, 1.00, 1.00},
    {1, "Kestrel", 600, 1.12, 0.94, 0.98},
    {2, "Phantom", 1200, 0.96, 1.12, 1.08}
};

enum class UpgradeKind { Engine, Grip };

struct GarageSnapshot final
{
    int Credits = 0;
    int OwnedVehicleMask = 1;
    int SelectedVehicle = 0;
    int EngineLevel = 0;
    int GripLevel = 0;
};

struct VehiclePerformance final
{
    double Acceleration = 1.0;
    double Grip = 1.0;
    double TopSpeed = 1.0;
};

class GarageState final
{
public:
    void Load(GarageSnapshot saved) noexcept
    {
        Data.Credits = std::max(0, saved.Credits);
        Data.OwnedVehicleMask = (saved.OwnedVehicleMask & 7) | 1;
        Data.SelectedVehicle = Owns(saved.SelectedVehicle) ? saved.SelectedVehicle : 0;
        Data.EngineLevel = std::clamp(saved.EngineLevel, 0, 3);
        Data.GripLevel = std::clamp(saved.GripLevel, 0, 3);
    }

    GarageSnapshot Snapshot() const noexcept { return Data; }
    int Credits() const noexcept { return Data.Credits; }
    int SelectedVehicle() const noexcept { return Data.SelectedVehicle; }
    int EngineLevel() const noexcept { return Data.EngineLevel; }
    int GripLevel() const noexcept { return Data.GripLevel; }

    static bool IsVehicle(int id) noexcept { return id >= 0 && id < 3; }
    bool Owns(int id) const noexcept
    {
        return IsVehicle(id) && (Data.OwnedVehicleMask & (1 << id)) != 0;
    }

    static VehicleSpec Definition(int id) noexcept
    {
        return Vehicles[IsVehicle(id) ? id : 0];
    }

    void EarnCredits(int amount) noexcept
    {
        if (amount <= 0) return;
        Data.Credits += std::min(amount, std::numeric_limits<int>::max() - Data.Credits);
    }

    bool Unlock(int id) noexcept
    {
        if (!IsVehicle(id) || Owns(id) || Data.Credits < Definition(id).UnlockCost)
            return false;
        Data.Credits -= Definition(id).UnlockCost;
        Data.OwnedVehicleMask |= 1 << id;
        return true;
    }

    bool Select(int id) noexcept
    {
        if (!Owns(id)) return false;
        Data.SelectedVehicle = id;
        return true;
    }

    static int UpgradeCost(UpgradeKind kind, int currentLevel) noexcept
    {
        if (currentLevel < 0 || currentLevel >= 3) return 0;
        if (kind == UpgradeKind::Engine) return 200 + currentLevel * 150;
        if (kind == UpgradeKind::Grip) return 150 + currentLevel * 125;
        return 0;
    }

    bool Upgrade(UpgradeKind kind) noexcept
    {
        if (kind != UpgradeKind::Engine && kind != UpgradeKind::Grip) return false;
        int& level = kind == UpgradeKind::Engine ? Data.EngineLevel : Data.GripLevel;
        const int cost = UpgradeCost(kind, level);
        if (cost <= 0 || Data.Credits < cost) return false;
        Data.Credits -= cost;
        ++level;
        return true;
    }

    VehiclePerformance Performance() const noexcept
    {
        const VehicleSpec v = Definition(Data.SelectedVehicle);
        return {v.Acceleration * (1.0 + 0.06 * Data.EngineLevel),
                v.Grip * (1.0 + 0.05 * Data.GripLevel),
                v.TopSpeed * (1.0 + 0.02 * Data.EngineLevel)};
    }

private:
    GarageSnapshot Data;
};
} // namespace latebrake
