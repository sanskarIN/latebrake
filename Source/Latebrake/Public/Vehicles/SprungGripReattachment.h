#pragma once

#include "Vehicles/AxleTractionBudget.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Suspension-dependent axle authority MUST fall as soon as measured support
// is lost. After real re-contact, ramp REAR MOTOR and FRONT STEERING only.
// Whole-body braking and lateral grip retain the immediate, physically
// measured ceiling; delaying service braking would compromise control.
// This shapes arcade drive force AUTHORIZATION only, not Chaos spring forces,
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
            !ValidScale(axles.FrontAxleLoadRatio) ||
            !ValidScale(axles.RearAxleLoadRatio) ||
            // Current wheel count limits whole-body grip even when
            // transient spring compression temporarily exceeds weight.
            ground.ContactTractionScale >
                (ground.SupportingProbes == 4 ? 1.0 :
                 ground.SupportingProbes == 3 ? 0.75 :
                 ground.SupportingProbes == 2 ? 0.45 : 0.0) + 1.0e-9 ||
            // An axle can NEVER claim more power/steering than its own
            // physical spring load and 2/1/0-contact eligibility.
            axles.RearDriveTractionScale >
                axles.RearAxleLoadRatio *
                (axles.RearWheelContacts == 2 ? 1.0 :
                 axles.RearWheelContacts == 1 ?
                     OneDrivenWheelGripFraction : 0.0) + 1.0e-9 ||
            axles.FrontSteeringTractionScale >
                axles.FrontAxleLoadRatio *
                (axles.FrontWheelContacts == 2 ? 1.0 :
                 axles.FrontWheelContacts == 1 ?
                     OneSteeringWheelGripFraction : 0.0) + 1.0e-9 ||
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
        RearGrip = std::min(axles.RearDriveTractionScale, RearGrip + rise);
        FrontGrip = std::min(axles.FrontSteeringTractionScale, FrontGrip + rise);
        result.Axles.RearDriveTractionScale =
            std::min(RearGrip, ground.ContactTractionScale);
        result.Axles.FrontSteeringTractionScale =
            std::min(FrontGrip, ground.ContactTractionScale);
        result.Axles.RearDriveReady = ground.HasDriveContact &&
            axles.RearWheelContacts > 0 &&
            result.Axles.RearDriveTractionScale > AxleGripLoadEpsilon;
        result.Axles.FrontSteeringReady = ground.HasDriveContact &&
            axles.FrontWheelContacts > 0 &&
            result.Axles.FrontSteeringTractionScale > AxleGripLoadEpsilon;
        return result;
    }

private:
    static bool ValidScale(double value) noexcept
    {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    }

    double RearGrip = 0.0;
    double FrontGrip = 0.0;
};
} // namespace latebrake
