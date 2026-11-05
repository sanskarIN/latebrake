#pragma once

#include "Vehicles/AxleTractionBudget.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Suspension-dependent tire force MUST fall as soon as measured support is
// lost. After physical re-contact, ramp motor/steering/braking authority
// back up over ~1/6 second rather than snapping on for a single ray hit.
// This shapes arcade force AUTHORIZATION only, not Chaos spring forces,
// world collision, tire friction coefficients, or race scoring.
inline constexpr double GripReattachmentRisePerSecond = 6.0;
inline constexpr double GripReattachmentMaxFrameSeconds = 0.10;

struct GripReattachmentFrame final
{
    bool Valid = false;
    ChassisGroundContactOutput Ground{};
    AxleTractionBudget Axles{};
};

class SprungGripReattachment final
{
public:
    void Reset() noexcept
    {
        WholeGrip = 0.0;
        RearGrip = 0.0;
        FrontGrip = 0.0;
    }

    GripReattachmentFrame Advance(
        double deltaSeconds,
        const ChassisGroundContactOutput& ground,
        const AxleTractionBudget& axles) noexcept
    {
        // Reject inconsistent or corrupt caller inputs before allowing any
        // previous frame's traction to influence the current frame.
        if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0 ||
            deltaSeconds > GripReattachmentMaxFrameSeconds ||
            !ground.Valid || !axles.Valid ||
            ground.SupportingProbes < 0 || ground.SupportingProbes > 4 ||
            axles.FrontWheelContacts < 0 || axles.FrontWheelContacts > 2 ||
            axles.RearWheelContacts < 0 || axles.RearWheelContacts > 2 ||
            axles.FrontWheelContacts + axles.RearWheelContacts !=
                ground.SupportingProbes ||
            !ValidScale(ground.ContactTractionScale) ||
            !ValidScale(axles.RearDriveTractionScale) ||
            !ValidScale(axles.FrontSteeringTractionScale) ||
            axles.RearDriveTractionScale >
                ground.ContactTractionScale + 1.0e-9 ||
            axles.FrontSteeringTractionScale >
                ground.ContactTractionScale + 1.0e-9 ||
            ground.HasDriveContact !=
                (ground.SupportingProbes >= 2 &&
                 ground.ContactTractionScale > AxleGripLoadEpsilon) ||
            axles.RearDriveReady !=
                (ground.HasDriveContact && axles.RearWheelContacts > 0 &&
                 axles.RearDriveTractionScale > AxleGripLoadEpsilon) ||
            axles.FrontSteeringReady !=
                (ground.HasDriveContact && axles.FrontWheelContacts > 0 &&
                 axles.FrontSteeringTractionScale > AxleGripLoadEpsilon))
        {
            Reset();
            return {};
        }

        const double rise = GripReattachmentRisePerSecond * deltaSeconds;
        GripReattachmentFrame result;
        result.Valid = true;
        result.Ground = ground;
        result.Axles = axles;

        // Falling edges are IMMEDIATE: this cannot preserve or invent
        // force after the production contact/axle policy says it is gone.
        WholeGrip = std::min(ground.ContactTractionScale, WholeGrip + rise);
        RearGrip = std::min(axles.RearDriveTractionScale, RearGrip + rise);
        FrontGrip = std::min(axles.FrontSteeringTractionScale, FrontGrip + rise);
        result.Ground.ContactTractionScale = WholeGrip;
        result.Ground.HasDriveContact =
            ground.SupportingProbes >= 2 && WholeGrip > AxleGripLoadEpsilon;
        result.Axles.RearDriveTractionScale = std::min(RearGrip, WholeGrip);
        result.Axles.FrontSteeringTractionScale = std::min(FrontGrip, WholeGrip);
        result.Axles.RearDriveReady = result.Ground.HasDriveContact &&
            axles.RearWheelContacts > 0 &&
            result.Axles.RearDriveTractionScale > AxleGripLoadEpsilon;
        result.Axles.FrontSteeringReady = result.Ground.HasDriveContact &&
            axles.FrontWheelContacts > 0 &&
            result.Axles.FrontSteeringTractionScale > AxleGripLoadEpsilon;
        return result;
    }

private:
    static bool ValidScale(double value) noexcept
    {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    }

    double WholeGrip = 0.0;
    double RearGrip = 0.0;
    double FrontGrip = 0.0;
};
} // namespace latebrake
