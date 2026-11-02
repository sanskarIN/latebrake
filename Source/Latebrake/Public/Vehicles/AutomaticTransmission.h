#pragma once

#include "Vehicles/ArcadeReverseSpeedLimit.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace latebrake
{
// Simplified AUTOMATIC gear staging for the existing single rigid-body
// player/rival Chaos car. This is not a physical clutch/gearbox, wheel RPM,
// engine sound, skeletal-wheel simulation or certified engine tachometer.
// Actual signed chassis speed chooses ratio bands; every propulsion boost
// remains subject to the existing shared speed-cap and force budgets.
struct AutomaticTransmissionSample final
{
    double DeltaSeconds = 0.0;
    double ForwardSpeedCmPerSecond = 0.0;
    double Throttle = 0.0;
    double ForwardSpeedCapCmPerSecond = 0.0;
    bool ArcadeHandling = true;
    bool DrivingReady = false;
};

struct AutomaticTransmissionState final
{
    bool Valid = false;
    int Gear = 1; // -1 reverse, 1-4 forward
    int EstimatedRpm = 0; // calculated from chassis speed, NOT a measured engine
    double PropulsionFactor = 0.0;
    bool Shifting = false;
};

class AutomaticTransmission final
{
public:
    static constexpr int ForwardGearCount = 4;
    static constexpr double ShiftInterruptionSeconds = 0.18;
    static constexpr double DirectionChangeThresholdCmPerSecond = 110.0;
    static constexpr std::array<double, 3> UpshiftRatios{{0.24, 0.49, 0.74}};
    static constexpr std::array<double, 3> DownshiftRatios{{0.16, 0.39, 0.64}};
    static constexpr std::array<double, 4> GearRedlineFractions{{0.31, 0.56, 0.80, 1.0}};
    static constexpr std::array<double, 4> TorqueFractions{{1.0, 0.98, 0.96, 0.94}};

    void Reset() noexcept
    {
        Gear = 1;
        ShiftRemaining = 0.0;
        Last = {};
    }

    void Suspend() noexcept
    {
        // Never advertise stale RPM or partially completed shifts after a
        // focus loss, race pause or invalid Unreal physics sample.
        Reset();
    }

    AutomaticTransmissionState Readout() const noexcept { return Last; }

    AutomaticTransmissionState Advance(
        AutomaticTransmissionSample sample) noexcept
    {
        if (!sample.DrivingReady ||
            !std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 || sample.DeltaSeconds > 0.10 ||
            !std::isfinite(sample.ForwardSpeedCmPerSecond) ||
            std::abs(sample.ForwardSpeedCmPerSecond) > 20000.0 ||
            !std::isfinite(sample.Throttle) ||
            std::abs(sample.Throttle) > 1.0 ||
            !std::isfinite(sample.ForwardSpeedCapCmPerSecond) ||
            sample.ForwardSpeedCapCmPerSecond < 500.0 ||
            sample.ForwardSpeedCapCmPerSecond > 20000.0)
        {
            Suspend();
            return Last;
        }

        // An opposite-direction pedal first uses the existing motor-braking
        // force. Never jump into reverse while moving forward (or vice versa).
        const double speed = sample.ForwardSpeedCmPerSecond;
        const double requested = sample.Throttle;
        const bool wantsReverse = requested < -0.05;
        const bool wantsForward = requested > 0.05;
        const bool directionOpposed =
            (wantsReverse && speed > DirectionChangeThresholdCmPerSecond) ||
            (wantsForward && speed < -DirectionChangeThresholdCmPerSecond);
        if (ShiftRemaining > 0.0)
        {
            ShiftRemaining = std::max(0.0,
                ShiftRemaining - sample.DeltaSeconds);
        }

        if (ShiftRemaining <= 0.0)
        {
            if (wantsReverse && speed <= DirectionChangeThresholdCmPerSecond)
            {
                if (Gear != -1) ShiftTo(-1);
            }
            else if (wantsForward &&
                     speed >= -DirectionChangeThresholdCmPerSecond &&
                     Gear == -1)
            {
                ShiftTo(1);
            }
            else if (Gear >= 1)
            {
                // Gear staging follows actual forward speed even during
                // coasting; shifts at most ONE step per completed delay.
                const double ratio = std::clamp(
                    std::max(0.0, speed) /
                        sample.ForwardSpeedCapCmPerSecond,
                    0.0, 2.0);
                if (Gear < ForwardGearCount &&
                    ratio >= UpshiftRatios[static_cast<unsigned>(Gear - 1)])
                {
                    ShiftTo(Gear + 1);
                }
                else if (Gear > 1 &&
                         ratio <= DownshiftRatios[static_cast<unsigned>(Gear - 2)])
                {
                    ShiftTo(Gear - 1);
                }
            }
        }

        const bool shifting = ShiftRemaining > 0.0;
        const double reverseCap =
            MaximumReverseSpeedCmPerSecond(sample.ArcadeHandling);
        const double signedGearSpeedCap = Gear == -1
            ? reverseCap
            : sample.ForwardSpeedCapCmPerSecond *
                GearRedlineFractions[static_cast<unsigned>(Gear - 1)];
        // The RPM is an estimated UI/force-model value from effective ratio,
        // not a wheel sensor or engine-mounted tachometer.
        const double normalizedRpm = std::clamp(
            std::abs(speed) / signedGearSpeedCap, 0.0, 1.0);
        const int estimatedRpm =
            static_cast<int>(900.0 + 6300.0 * normalizedRpm);

        // Never weaken opposite-direction motor braking/reversal. The
        // existing GearAwareMotorPowerFactor is still the final speed cap.
        double propulsion = 1.0;
        if (!directionOpposed)
        {
            propulsion = Gear == -1 ? 0.94 :
                TorqueFractions[static_cast<unsigned>(Gear - 1)];
            // Smooth loss at the top of the simulated RPM band rather than
            // an unrealistic instant redline power spike.
            const double bandEnd = std::clamp(
                (normalizedRpm - 0.75) / 0.25, 0.0, 1.0);
            propulsion *= 1.0 - 0.08 * bandEnd;
            if (shifting) propulsion *= 0.22;
        }
        Last = {true, Gear, estimatedRpm, propulsion, shifting};
        return Last;
    }

private:
    void ShiftTo(int nextGear) noexcept
    {
        Gear = nextGear;
        ShiftRemaining = ShiftInterruptionSeconds;
    }

    int Gear = 1;
    double ShiftRemaining = 0.0;
    AutomaticTransmissionState Last{};
};
} // namespace latebrake
