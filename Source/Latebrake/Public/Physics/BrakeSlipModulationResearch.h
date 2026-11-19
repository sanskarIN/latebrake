#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace LateBrake::PhysicsResearch {

// Advisory four-wheel ABS pressure modulation; not wired to Chaos brakes.
// Speeds are wheel-ground circumferential speeds in m/s, in FL/FR/RL/RR order.
// Positive vehicle speed is forward. No braking is requested while reversing.
struct BrakeSlipObservation {
    double vehicleSpeedMps = 0.0;
    std::array<double, 4> wheelSurfaceSpeedMps{};
    std::array<bool, 4> contactValid{};
    double sampleSeconds = 0.0;
};

struct BrakeSlipCalibration {
    double entrySlip = 0.18;
    double recoverySlip = 0.10;
    double minimumVehicleSpeedMps = 2.0;
    double releaseRatePerSecond = 6.0;
    double restoreRatePerSecond = 1.5;
    double minimumPressureFraction = 0.15;
};

struct BrakeSlipState {
    std::array<double, 4> pressureFraction{1.0, 1.0, 1.0, 1.0};
    std::array<bool, 4> releasing{};
};

struct BrakeSlipResult {
    BrakeSlipState next{};
    std::array<double, 4> slip{};
    bool valid = false;
    bool active = false;
};

inline bool ValidBrakeSlipCalibration(const BrakeSlipCalibration& c) noexcept {
    return std::isfinite(c.entrySlip) && std::isfinite(c.recoverySlip) &&
           std::isfinite(c.minimumVehicleSpeedMps) &&
           std::isfinite(c.releaseRatePerSecond) &&
           std::isfinite(c.restoreRatePerSecond) &&
           std::isfinite(c.minimumPressureFraction) &&
           c.entrySlip > 0.0 && c.entrySlip < 1.0 &&
           c.recoverySlip >= 0.0 && c.recoverySlip < c.entrySlip &&
           c.minimumVehicleSpeedMps > 0.0 &&
           c.releaseRatePerSecond > 0.0 && c.restoreRatePerSecond > 0.0 &&
           c.minimumPressureFraction >= 0.0 && c.minimumPressureFraction <= 1.0;
}

// Invalid input is fail-closed: no new modulation command is issued.
// Contact loss clears the corresponding hysteresis latch and restores pressure
// gradually rather than interpreting an airborne wheel as locked.
inline BrakeSlipResult AdvanceBrakeSlip(
    const BrakeSlipObservation& observation,
    const BrakeSlipCalibration& calibration,
    const BrakeSlipState& previous) noexcept {
    BrakeSlipResult result{};
    if (!ValidBrakeSlipCalibration(calibration) ||
        !std::isfinite(observation.vehicleSpeedMps) ||
        !std::isfinite(observation.sampleSeconds) ||
        observation.sampleSeconds <= 0.0 || observation.sampleSeconds > 0.1)
        return result;

    for (std::size_t i = 0; i < 4; ++i) {
        if (!std::isfinite(observation.wheelSurfaceSpeedMps[i]) ||
            observation.wheelSurfaceSpeedMps[i] < 0.0 ||
            !std::isfinite(previous.pressureFraction[i]) ||
            previous.pressureFraction[i] < calibration.minimumPressureFraction ||
            previous.pressureFraction[i] > 1.0)
            return result;
    }

    result.next = previous;
    const bool moving = observation.vehicleSpeedMps >= calibration.minimumVehicleSpeedMps;
    const double releaseStep = calibration.releaseRatePerSecond * observation.sampleSeconds;
    const double restoreStep = calibration.restoreRatePerSecond * observation.sampleSeconds;
    for (std::size_t i = 0; i < 4; ++i) {
        const bool supported = moving && observation.contactValid[i];
        if (supported) {
            // Clamping prevents wheel overspeed from masquerading as brake lock.
            const double ratio = observation.wheelSurfaceSpeedMps[i] /
                                 observation.vehicleSpeedMps;
            result.slip[i] = std::clamp(1.0 - ratio, 0.0, 1.0);
            if (result.next.releasing[i]) {
                if (result.slip[i] <= calibration.recoverySlip)
                    result.next.releasing[i] = false;
            } else if (result.slip[i] >= calibration.entrySlip) {
                result.next.releasing[i] = true;
            }
        } else {
            result.next.releasing[i] = false;
        }

        const double oldPressure = previous.pressureFraction[i];
        if (result.next.releasing[i]) {
            result.next.pressureFraction[i] =
                std::max(calibration.minimumPressureFraction, oldPressure - releaseStep);
            result.active = true;
        } else {
            result.next.pressureFraction[i] = std::min(1.0, oldPressure + restoreStep);
        }
    }
    result.valid = true;
    return result;
}
} // namespace LateBrake::PhysicsResearch
