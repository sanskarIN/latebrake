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
        !std::isfinite(result.minimumWheelN)) {
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

} // namespace LateBrake::PhysicsResearch
