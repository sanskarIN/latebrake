#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace LateBrake::PhysicsResearch {

// Source-only sensor plausibility review. This does not authenticate vehicle
// telemetry or command Chaos brakes. Wheel order is FL, FR, RL, RR.
struct WheelSpeedSample {
    double frameSeconds = 0.0;
    double vehicleSpeedMps = 0.0;
    std::array<double, 4> angularSpeedRadPerSecond{};
    std::array<double, 4> rollingRadiusMeters{};
    std::array<bool, 4> inContact{};
};

struct WheelSpeedLimits {
    double minimumRadiusMeters = 0.15;
    double maximumRadiusMeters = 0.65;
    double maximumSurfaceSpeedMps = 140.0;
    double maximumVehicleSpeedMps = 140.0;
    double maximumWheelAccelerationMps2 = 140.0;
    double maximumVehicleAccelerationMps2 = 65.0;
    double maximumFrameSeconds = 0.1;
};

struct WheelSpeedHistory {
    bool initialized = false;
    double vehicleSpeedMps = 0.0;
    std::array<double, 4> surfaceSpeedMps{};
    std::array<bool, 4> inContact{};
};

struct WheelSpeedReview {
    bool valid = false;
    std::array<bool, 4> plausible{};
    std::array<double, 4> surfaceSpeedMps{};
    WheelSpeedHistory next{};
};

inline bool ValidWheelSpeedLimits(const WheelSpeedLimits& c) noexcept {
    return std::isfinite(c.minimumRadiusMeters) &&
           std::isfinite(c.maximumRadiusMeters) &&
           std::isfinite(c.maximumSurfaceSpeedMps) &&
           std::isfinite(c.maximumVehicleSpeedMps) &&
           std::isfinite(c.maximumWheelAccelerationMps2) &&
           std::isfinite(c.maximumVehicleAccelerationMps2) &&
           std::isfinite(c.maximumFrameSeconds) &&
           c.minimumRadiusMeters > 0.0 &&
           c.maximumRadiusMeters >= c.minimumRadiusMeters &&
           c.maximumSurfaceSpeedMps > 0.0 &&
           c.maximumVehicleSpeedMps > 0.0 &&
           c.maximumWheelAccelerationMps2 > 0.0 &&
           c.maximumVehicleAccelerationMps2 > 0.0 &&
           c.maximumFrameSeconds > 0.0 && c.maximumFrameSeconds <= 0.1;
}

// Invalid frames leave history untouched. A suspect wheel does not advance
// its history: a single sensor spike cannot poison subsequent valid frames.
// A wheel returning from loss of contact gets a new temporal reference.
// The first accepted frame initializes the temporal baseline, but does not
// establish external sensor authenticity.
inline WheelSpeedReview ReviewWheelSpeedSample(
    const WheelSpeedSample& sample,
    const WheelSpeedLimits& limits,
    const WheelSpeedHistory& history) noexcept {
    WheelSpeedReview out{};
    if (!ValidWheelSpeedLimits(limits) ||
        !std::isfinite(sample.frameSeconds) ||
        sample.frameSeconds <= 0.0 ||
        sample.frameSeconds > limits.maximumFrameSeconds ||
        !std::isfinite(sample.vehicleSpeedMps) ||
        std::abs(sample.vehicleSpeedMps) > limits.maximumVehicleSpeedMps)
        return out;

    if (history.initialized) {
        if (!std::isfinite(history.vehicleSpeedMps) ||
            std::abs(history.vehicleSpeedMps) > limits.maximumVehicleSpeedMps)
            return out;
        const double vehicleDelta = std::abs(
            sample.vehicleSpeedMps - history.vehicleSpeedMps);
        const double allowed = limits.maximumVehicleAccelerationMps2 *
                               sample.frameSeconds;
        if (!std::isfinite(allowed) || vehicleDelta > allowed)
            return out;
    }

    // A corrupted trusted wheel history must not silently become a plausible
    // observation when contact temporarily drops. Reject the entire frame
    // without advancing vehicle history, so callers can reset explicitly.
    if (history.initialized) {
        for (std::size_t i = 0; i < 4; ++i) {
            if (history.inContact[i] &&
                (!std::isfinite(history.surfaceSpeedMps[i]) ||
                 std::abs(history.surfaceSpeedMps[i]) >
                     limits.maximumSurfaceSpeedMps))
                return out;
        }
    }

    out.next = history;
    out.next.initialized = true;
    out.next.vehicleSpeedMps = sample.vehicleSpeedMps;
    for (std::size_t i = 0; i < 4; ++i) {
        const double radius = sample.rollingRadiusMeters[i];
        const double angular = sample.angularSpeedRadPerSecond[i];
        if (!std::isfinite(radius) || !std::isfinite(angular) ||
            radius < limits.minimumRadiusMeters ||
            radius > limits.maximumRadiusMeters)
            return WheelSpeedReview{};
        // Preserve rotation direction: reverse travel and direction changes are valid.
        // Temporal acceleration checks still reject impossible sign flips.
        const double surface = radius * angular;
        if (!std::isfinite(surface) ||
            std::abs(surface) > limits.maximumSurfaceSpeedMps)
            return WheelSpeedReview{};
        out.surfaceSpeedMps[i] = surface;
        bool plausible = true;
        if (history.initialized && history.inContact[i] &&
            sample.inContact[i]) {
            const double delta = std::abs(surface - history.surfaceSpeedMps[i]);
            const double allowed = limits.maximumWheelAccelerationMps2 *
                                   sample.frameSeconds;
            plausible = std::isfinite(history.surfaceSpeedMps[i]) &&
                        std::isfinite(allowed) && delta <= allowed;
        }
        // Airborne wheels are not a trusted road-slip observation.
        out.plausible[i] = sample.inContact[i] && plausible;
        if (plausible) {
            out.next.surfaceSpeedMps[i] = surface;
            out.next.inContact[i] = sample.inContact[i];
        } else {
            // Retain the last trusted contact speed for a later comparison.
            out.next.inContact[i] = history.inContact[i];
        }
    }
    out.valid = true;
    return out;
}
} // namespace LateBrake::PhysicsResearch
