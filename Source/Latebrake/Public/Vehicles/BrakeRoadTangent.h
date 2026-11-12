#pragma once

#include "Vehicles/SprungChassisDynamics.h"

#include <cmath>

namespace latebrake
{
struct BrakeRoadTangentSample final
{
    double ForwardX = 0.0;
    double ForwardY = 0.0;
    double NormalX = 0.0;
    double NormalY = 0.0;
    double NormalZ = 0.0;
    double PointVelocityX = 0.0;
    double PointVelocityY = 0.0;
    double PointVelocityZ = 0.0;
};

struct BrakeRoadTangentOutput final
{
    bool Valid = false;
    double DirectionX = 0.0;
    double DirectionY = 0.0;
    double DirectionZ = 0.0;
    // Positive projection of the unit tangent on the planar forward axis.
    double PlanarForwardScale = 0.0;
    double LongitudinalCmPerSecond = 0.0;
};

// Intersection of the measured road tangent plane with the vertical plane
// through the actual planar chassis heading. This preserves that heading's
// XY direction while adding only the grade required by the genuine normal.
// Orthogonal projection of the heading onto a banked road would also add a
// sideways force, silently bypassing the existing lateral friction budget.
//
// Unit tangent T = (Fx,Fy,-dot(Nxy,F)/Nz) / length(...).
// Consequently T dot N = 0 and T dot bodyRight = 0. The brake magnitude and
// the existing horizontal lateral request remain orthogonal in 3D. This
// does not make the lateral controller itself a road-tangent tire solver.
inline BrakeRoadTangentOutput MeasureBrakeRoadTangent(
    const BrakeRoadTangentSample& s) noexcept
{
    if (!std::isfinite(s.ForwardX) || !std::isfinite(s.ForwardY) ||
        !std::isfinite(s.NormalX) || !std::isfinite(s.NormalY) ||
        !std::isfinite(s.NormalZ) ||
        !std::isfinite(s.PointVelocityX) ||
        !std::isfinite(s.PointVelocityY) ||
        !std::isfinite(s.PointVelocityZ) ||
        std::abs(s.ForwardX) > 1.0 || std::abs(s.ForwardY) > 1.0 ||
        std::abs(s.NormalX) > 1.0 || std::abs(s.NormalY) > 1.0 ||
        s.NormalZ < SuspensionMinNormalZ || s.NormalZ > 1.0 ||
        std::abs(s.PointVelocityX) > 100000.0 ||
        std::abs(s.PointVelocityY) > 100000.0 ||
        std::abs(s.PointVelocityZ) > 100000.0)
    {
        return {};
    }
    const double headingSquared = s.ForwardX * s.ForwardX +
        s.ForwardY * s.ForwardY;
    const double normalSquared = s.NormalX * s.NormalX +
        s.NormalY * s.NormalY + s.NormalZ * s.NormalZ;
    if (std::abs(headingSquared - 1.0) > 1.0e-3 ||
        std::abs(normalSquared - 1.0) > 1.0e-3)
    {
        return {};
    }
    // Normalize the accepted heading, rather than allowing its small
    // measurement error to scale the eventual physical force magnitude.
    const double headingLength = std::sqrt(headingSquared);
    const double fx = s.ForwardX / headingLength;
    const double fy = s.ForwardY / headingLength;
    const double grade = -(s.NormalX * fx + s.NormalY * fy) / s.NormalZ;
    const double horizontal = 1.0 / std::hypot(1.0, grade);
    const double x = fx * horizontal;
    const double y = fy * horizontal;
    const double z = grade * horizontal;
    const double speed = x * s.PointVelocityX + y * s.PointVelocityY +
        z * s.PointVelocityZ;
    if (!std::isfinite(speed) || std::abs(speed) > 100000.0)
    {
        return {};
    }
    return {true, x, y, z, horizontal, speed};
}
} // namespace latebrake
