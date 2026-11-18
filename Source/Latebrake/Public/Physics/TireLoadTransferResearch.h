#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace LateBrake::PhysicsResearch {

// Source-only steady-state wheel-load estimate. No Chaos actuator authority.
// Coordinates: x forward, y left; acceleration is in chassis coordinates.
// Positive longitudinal acceleration transfers load rearward; positive lateral
// acceleration transfers load to right-side tires.
struct LoadTransferInput {
    double massKg = 0.0;
    double gravityMps2 = 9.80665;
    double wheelbaseM = 0.0;
    double frontTrackM = 0.0;
    double rearTrackM = 0.0;
    double centerOfMassHeightM = 0.0;
    double frontStaticFraction = 0.0;
    double frontRollTransferFraction = 0.0;
    double longitudinalAccelerationMps2 = 0.0;
    double lateralAccelerationMps2 = 0.0;
};

enum class LoadTransferStatus {
    Valid,
    InvalidInput,
    WheelLift
};

struct WheelLoadEstimate {
    // FL, FR, RL, RR, in newtons.
    std::array<double, 4> normalN{};
    double frontAxleN = 0.0;
    double rearAxleN = 0.0;
    double totalN = 0.0;
    double minimumWheelN = 0.0;
    LoadTransferStatus status = LoadTransferStatus::InvalidInput;
};

inline bool FiniteLoadInput(const LoadTransferInput& v) noexcept {
    return std::isfinite(v.massKg) && std::isfinite(v.gravityMps2) &&
           std::isfinite(v.wheelbaseM) && std::isfinite(v.frontTrackM) &&
           std::isfinite(v.rearTrackM) && std::isfinite(v.centerOfMassHeightM) &&
           std::isfinite(v.frontStaticFraction) &&
           std::isfinite(v.frontRollTransferFraction) &&
           std::isfinite(v.longitudinalAccelerationMps2) &&
           std::isfinite(v.lateralAccelerationMps2);
}

// Rejects wheel lift instead of silently clamping loads and falsely
// representing a fully grounded four-contact-patch vehicle.
inline WheelLoadEstimate EstimateWheelLoads(const LoadTransferInput& v) noexcept {
    WheelLoadEstimate result{};
    if (!FiniteLoadInput(v) || v.massKg <= 0.0 || v.gravityMps2 <= 0.0 ||
        v.wheelbaseM <= 0.0 || v.frontTrackM <= 0.0 || v.rearTrackM <= 0.0 ||
        v.centerOfMassHeightM < 0.0 || v.frontStaticFraction <= 0.0 ||
        v.frontStaticFraction >= 1.0 || v.frontRollTransferFraction < 0.0 ||
        v.frontRollTransferFraction > 1.0) {
        return result;
    }

    const double weight = v.massKg * v.gravityMps2;
    const double longitudinal = v.massKg * v.longitudinalAccelerationMps2 *
                                v.centerOfMassHeightM / v.wheelbaseM;
    const double front = weight * v.frontStaticFraction - longitudinal;
    const double rear = weight - front;
    const double frontSide = v.massKg * v.lateralAccelerationMps2 *
                             v.centerOfMassHeightM * v.frontRollTransferFraction /
                             v.frontTrackM;
    const double rearSide = v.massKg * v.lateralAccelerationMps2 *
                            v.centerOfMassHeightM * (1.0 - v.frontRollTransferFraction) /
                            v.rearTrackM;
    result.normalN = {front * 0.5 - frontSide, front * 0.5 + frontSide,
                      rear * 0.5 - rearSide, rear * 0.5 + rearSide};
    result.frontAxleN = front;
    result.rearAxleN = rear;
    result.totalN = weight;
    result.minimumWheelN = *std::min_element(result.normalN.begin(),
                                              result.normalN.end());
    if (!std::isfinite(weight) || !std::isfinite(front) || !std::isfinite(rear) ||
        !std::isfinite(frontSide) || !std::isfinite(rearSide) ||
        !std::isfinite(result.minimumWheelN) ||
        !std::all_of(result.normalN.begin(), result.normalN.end(),
                     [](double load) { return std::isfinite(load); })) {
        return WheelLoadEstimate{};
    }
    result.status = result.minimumWheelN <= 0.0 ? LoadTransferStatus::WheelLift
                                                 : LoadTransferStatus::Valid;
    return result;
}

// Load-sensitive friction proxy, not a tire-force or combined-slip model.
// The reference load and exponent are calibration inputs, never guessed from
// the wheel-load estimator. Output is advisory only.
struct TireLoadSensitivity {
    double referenceLoadN = 0.0;
    double referenceMu = 0.0;
    double loadExponent = 0.0;
};

inline double EstimatePeakLongitudinalForceN(
    double wheelLoadN, const TireLoadSensitivity& tire) noexcept {
    if (!std::isfinite(wheelLoadN) || !std::isfinite(tire.referenceLoadN) ||
        !std::isfinite(tire.referenceMu) || !std::isfinite(tire.loadExponent) ||
        wheelLoadN <= 0.0 || tire.referenceLoadN <= 0.0 ||
        tire.referenceMu <= 0.0 || tire.loadExponent < 0.0 ||
        tire.loadExponent >= 1.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double ratio = wheelLoadN / tire.referenceLoadN;
    const double force = tire.referenceMu * wheelLoadN *
                         std::pow(ratio, -tire.loadExponent);
    return std::isfinite(force) ? force :
           std::numeric_limits<double>::quiet_NaN();
}

// Upper bound on steady lateral acceleration before a wheel unloads.
// The bound is computed from the longitudinally transferred axle loads;
// it is not a tire-friction or roll-dynamics prediction.
inline double EstimateWheelLiftLateralLimitMps2(
    const LoadTransferInput& input) noexcept {
    if (!FiniteLoadInput(input)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    auto neutral = input;
    neutral.lateralAccelerationMps2 = 0.0;
    const auto loads = EstimateWheelLoads(neutral);
    if (loads.status != LoadTransferStatus::Valid) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (input.centerOfMassHeightM == 0.0) {
        return std::numeric_limits<double>::infinity();
    }
    double limit = std::numeric_limits<double>::infinity();
    if (input.frontRollTransferFraction > 0.0) {
        const double frontLimit = loads.frontAxleN * input.frontTrackM /
            (2.0 * input.massKg * input.centerOfMassHeightM *
             input.frontRollTransferFraction);
        limit = std::min(limit, frontLimit);
    }
    if (input.frontRollTransferFraction < 1.0) {
        const double rearLimit = loads.rearAxleN * input.rearTrackM /
            (2.0 * input.massKg * input.centerOfMassHeightM *
             (1.0 - input.frontRollTransferFraction));
        limit = std::min(limit, rearLimit);
    }
    return std::isfinite(limit) && limit > 0.0
        ? limit : std::numeric_limits<double>::quiet_NaN();
}

// Summed longitudinal peak-force proxy, only for a fully grounded car.
// No friction ellipse or brake/drive distribution is implied.
inline double EstimateFourWheelPeakLongitudinalForceN(
    const WheelLoadEstimate& loads,
    const TireLoadSensitivity& tire) noexcept {
    if (loads.status != LoadTransferStatus::Valid) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double total = 0.0;
    for (double load : loads.normalN) {
        const double force = EstimatePeakLongitudinalForceN(load, tire);
        if (!std::isfinite(force) || force <= 0.0) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        total += force;
    }
    return std::isfinite(total) && total > 0.0
        ? total : std::numeric_limits<double>::quiet_NaN();
}

// Conservative friction-circle proxy for available longitudinal tire force
// after a measured per-wheel lateral-force demand. This is an advisory bound,
// not a combined-slip tire model or an allocation of actuator brake torque.
// The caller must supply physical lateral demand at each contact patch.
inline double EstimateRemainingLongitudinalForceN(
    const WheelLoadEstimate& loads,
    const TireLoadSensitivity& tire,
    const std::array<double, 4>& lateralDemandN) noexcept {
    if (loads.status != LoadTransferStatus::Valid ||
        !std::isfinite(loads.totalN) || loads.totalN <= 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    double remaining = 0.0;
    double verifiedNormal = 0.0;
    for (std::size_t wheel = 0; wheel < loads.normalN.size(); ++wheel) {
        const double normal = loads.normalN[wheel];
        const double lateral = lateralDemandN[wheel];
        if (!std::isfinite(normal) || normal <= 0.0 ||
            !std::isfinite(lateral)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        const double peak = EstimatePeakLongitudinalForceN(normal, tire);
        if (!std::isfinite(peak) || peak <= 0.0 ||
            std::abs(lateral) > peak) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        const double utilization = lateral / peak;
        const double available = peak *
            std::sqrt(std::max(0.0, 1.0 - utilization * utilization));
        if (!std::isfinite(available)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        remaining += available;
        verifiedNormal += normal;
    }
    if (!std::isfinite(remaining) || !std::isfinite(verifiedNormal) ||
        std::abs(verifiedNormal - loads.totalN) >
            1e-8 * std::max(verifiedNormal, loads.totalN)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return remaining;
}

} // namespace LateBrake::PhysicsResearch
