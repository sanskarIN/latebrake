#pragma once

#include "Progression/RecordValidation.h"
#include "Vehicles/ArcadeCollisionDamage.h"
#include "Vehicles/ArcadeTireCondition.h"
#include "Vehicles/ArcadeNitroBoost.h"
#include <array>
#include <cmath>
#include <cstddef>

namespace latebrake
{
// Observations of the actual arcade model state immediately before and
// after a completed pit visit. These fractions are not physical sensors.
struct PitVehicleCondition final
{
    double Damage = 0.0;
    double Wear = 0.0;
    double Charge = 0.0;
    double BoostHeat = 0.0;
};

inline PitVehicleCondition ObservePitVehicleCondition(
    const ArcadeCollisionDamage& chassis, const ArcadeTireCondition& tires,
    const ArcadeNitroBoost& nitro) noexcept
{
    return {chassis.DamageFraction(), tires.WearFraction(),
        nitro.ChargeFraction(), nitro.HeatFraction()};
}

struct PitServiceReceipt final
{
    int Stop = 0;
    int Lap = 0;
    double RaceSeconds = 0.0;
    PitVehicleCondition Before{};
    PitVehicleCondition After{};
    double DamageRestored = 0.0;
    double WearRemoved = 0.0;
    double ChargeRestored = 0.0;
    double BoostHeatRemoved = 0.0;
};

struct PitServiceSummary final
{
    bool Valid = false;
    int Count = 0;
    double DamageRestored = 0.0;
    double WearRemoved = 0.0;
    double ChargeRestored = 0.0;
    double BoostHeatRemoved = 0.0;
};

// The receipt is advisory, never permission to repair, spend, advance a
// clock, change lap status or persist a record. A missing or malformed
// observation revokes completeness for this attempt instead of silently
// presenting an incomplete total. Eight most recent visits are retained;
// running totals include every accepted visit, bounded by StopCount.
class PitServiceJournal final
{
public:
    static constexpr int Capacity = 8;
    static constexpr int StopLimit = 1000;

    void Reset() noexcept
    {
        Entries.fill({});
        Next = 0;
        Retained = 0;
        Total = {};
        Complete = true;
    }

    bool RecordCompleted(int stop, int lap, double raceSeconds,
        PitVehicleCondition before, PitVehicleCondition after) noexcept
    {
        if (!Complete) return false;
        const auto previous = GetRecent(0);
        if (stop < 1 || stop > StopLimit || stop != Total.Count + 1 ||
            lap < 1 || !IsValidFinishedRaceTime(raceSeconds) ||
            (previous.Stop != 0 && (lap < previous.Lap ||
                raceSeconds <= previous.RaceSeconds)) ||
            !ValidCondition(before) || !ValidCondition(after) ||
            after.Damage > before.Damage || after.Wear > before.Wear ||
            after.Charge < before.Charge || after.BoostHeat > before.BoostHeat)
        {
            Complete = false;
            return false;
        }

        PitServiceReceipt receipt;
        receipt.Stop = stop;
        receipt.Lap = lap;
        receipt.RaceSeconds = raceSeconds;
        receipt.Before = before;
        receipt.After = after;
        receipt.DamageRestored = before.Damage - after.Damage;
        receipt.WearRemoved = before.Wear - after.Wear;
        receipt.ChargeRestored = after.Charge - before.Charge;
        receipt.BoostHeatRemoved = before.BoostHeat - after.BoostHeat;

        // Retention and aggregate are committed together. Each individual
        // change is at most one, so at most 1000 bounded finite additions.
        Entries[static_cast<std::size_t>(Next)] = receipt;
        Next = (Next + 1) % Capacity;
        if (Retained < Capacity) ++Retained;
        ++Total.Count;
        Total.DamageRestored += receipt.DamageRestored;
        Total.WearRemoved += receipt.WearRemoved;
        Total.ChargeRestored += receipt.ChargeRestored;
        Total.BoostHeatRemoved += receipt.BoostHeatRemoved;
        return true;
    }

    int RetainedCount() const noexcept { return Complete ? Retained : 0; }

    PitServiceReceipt GetRecent(int offset) const noexcept
    {
        if (!Complete || offset < 0 || offset >= Retained) return {};
        const int index = (Next + Capacity - 1 - offset) % Capacity;
        return Entries[static_cast<std::size_t>(index)];
    }

    PitServiceSummary Review(int actualStopCount) const noexcept
    {
        // The real service latch remains the authority for number of
        // visits. A saturated, missing or out-of-sequence log is not
        // presented as a complete service history.
        if (!Complete || Total.Count == 0 ||
            actualStopCount != Total.Count)
            return {};
        auto result = Total;
        result.Valid = true;
        return result;
    }

private:
    static bool Fraction(double v) noexcept
    {
        return std::isfinite(v) && v >= 0.0 && v <= 1.0;
    }

    static bool ValidCondition(PitVehicleCondition v) noexcept
    {
        return Fraction(v.Damage) &&
            v.Damage <= ArcadeCollisionDamage::MaxDamageFraction &&
            Fraction(v.Wear) && Fraction(v.Charge) && Fraction(v.BoostHeat);
    }

    std::array<PitServiceReceipt, Capacity> Entries{};
    int Next = 0;
    int Retained = 0;
    PitServiceSummary Total{};
    bool Complete = true;
};
} // namespace latebrake
