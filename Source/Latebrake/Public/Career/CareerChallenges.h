#pragma once

#include "Career/CareerCatalog.h"
#include <cmath>

namespace latebrake
{
// Passed by the authoritative player race state only; the rival route's
// approximate lap telemetry must NEVER feed career awards.
struct CareerFinish final
{
    double ElapsedSeconds = 0.0;
    double BestLapSeconds = 0.0;
    double SlowestLapSeconds = 0.0;
    int CompletedLaps = 0;
    int Recoveries = 0;
    int VehicleId = 0;
    bool OrderedCheckpoints = false;
};

inline bool IsValidCareerFinish(const CareerEvent& event,
                                const CareerFinish& result) noexcept
{
    if (!std::isfinite(result.ElapsedSeconds) || result.ElapsedSeconds <= 0.0 ||
        !std::isfinite(result.BestLapSeconds) || result.BestLapSeconds <= 0.0 ||
        !std::isfinite(result.SlowestLapSeconds) ||
        result.SlowestLapSeconds < result.BestLapSeconds ||
        result.SlowestLapSeconds > result.ElapsedSeconds ||
        result.CompletedLaps != event.Laps ||
        result.Recoveries < 0 || result.VehicleId < 0 || result.VehicleId > 2 ||
        !result.OrderedCheckpoints)
    {
        return false;
    }
    return event.RequiredVehicle < 0 || event.RequiredVehicle == result.VehicleId;
}

inline CareerMedal EvaluateCareerMedal(const CareerEvent& event,
                                      const CareerFinish& result) noexcept
{
    if (!IsValidCareerFinish(event, result))
    {
        return CareerMedal::None;
    }
    const double measured =
        event.Objective == CareerObjective::Clean
        ? static_cast<double>(result.Recoveries) :
        event.Objective == CareerObjective::Consistency
        ? result.SlowestLapSeconds - result.BestLapSeconds :
        result.ElapsedSeconds;

    if (measured <= event.GoldTarget) return CareerMedal::Gold;
    if (measured <= event.SilverTarget) return CareerMedal::Silver;
    if (measured <= event.BronzeTarget) return CareerMedal::Bronze;
    return CareerMedal::None;
}
} // namespace latebrake
