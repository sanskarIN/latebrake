#pragma once

#include "Vehicles/ChassisGroundContactTraction.h"

#include <cmath>

namespace latebrake
{
// Run-local, read-only summary of the actual four WorldStatic chassis
// traces. UI never infers an authoritative wheel load or suspension state.
enum class ChassisContactStatus
{
    Unavailable,
    Airborne,
    Partial,
    Full
};

struct ChassisContactReadout final
{
    ChassisContactStatus Status = ChassisContactStatus::Unavailable;
    int QualifiedProbes = 0;
    int DriveTractionPercent = 0;
    bool Live = false;
};

inline ChassisContactReadout DescribeChassisContact(
    ChassisGroundContactOutput sample) noexcept
{
    if (!sample.Valid ||
        sample.SupportingProbes < 0 || sample.SupportingProbes > 4 ||
        !std::isfinite(sample.ContactTractionScale) ||
        sample.ContactTractionScale < 0.0 ||
        sample.ContactTractionScale > 1.0 ||
        sample.HasDriveContact != (sample.SupportingProbes >= 2))
    {
        return {};
    }

    // The HUD consumes only ACTUAL accepted force-multiplier states.
    // Reject inconsistent state rather than advertising fake contact.
    constexpr double scales[] = {0.0, 0.0, 0.45, 0.75, 1.0};
    const double scale = scales[sample.SupportingProbes];
    if (std::abs(sample.ContactTractionScale - scale) > 1.0e-9)
    {
        return {};
    }
    return {
        sample.SupportingProbes == 4 ? ChassisContactStatus::Full :
        sample.SupportingProbes >= 2 ? ChassisContactStatus::Partial :
        ChassisContactStatus::Airborne,
        sample.SupportingProbes,
        static_cast<int>(scale * 100.0 + 0.5),
        true
    };
}
} // namespace latebrake
