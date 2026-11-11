#pragma once
#include "Racecraft/RacecraftMath.h"
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class ImpactMaterial : std::uint8_t
{
    Unknown, Steel, Concrete, ArmcoBarrier,
    TireBarrier, PlasticCone, VehicleBody, Gravel
};
struct CollisionMaterial final
{
    ImpactMaterial Kind = ImpactMaterial::Unknown;
    double Restitution = 0.15;
    double Friction = 0.60;
    double EnergyAbsorptionFraction = 0.25;
    double PermanentDeformationThresholdJ = 2500.0;
    double DamageSofteningFraction = 0.20;
    double TangentialImpulseLimit = 1.0;
};
struct CollisionBody final
{
    Vec2 PositionCm{};
    Vec2 VelocityCmS{};
    double HeadingRadians = 0.0;
    double YawRateRadS = 0.0;
    double MassKg = 1250.0;
    double YawInertiaKgM2 = 1800.0;
    Vec2 HalfSizeCm{210.0, 95.0};
    double CollisionDamageFraction = 0.0;
    std::uint64_t ActorToken = 0;
    bool RealRigidBodyVerified = false;
};
struct CollisionObservation final
{
    CollisionBody A{};
    CollisionBody B{};
    Vec2 WorldContactPointCm{};
    Vec2 WorldContactNormal{};
    double MeasuredPenetrationCm = 0.0;
    double RelativeNormalSpeedCmS = 0.0;
    double DeltaSeconds = 0.0;
    ImpactMaterial MaterialA = ImpactMaterial::VehicleBody;
    ImpactMaterial MaterialB = ImpactMaterial::VehicleBody;
    bool OriginalEngineContactReported = false;
    bool NativeCollisionImpulsesReadOnly = true;
};
struct CollisionImpulseResult final
{
    Vec2 AppliedImpulseToANs{};
    Vec2 AppliedImpulseToBNs{};
    Vec2 NormalImpulseNs{};
    Vec2 TangentialImpulseNs{};
    Vec2 PredictedAVelocityCmS{};
    Vec2 PredictedBVelocityCmS{};
    double PredictedAYawRateRadS = 0.0;
    double PredictedBYawRateRadS = 0.0;
    double RelativeNormalSpeedCmS = 0.0;
    double RelativeTangentialSpeedCmS = 0.0;
    double NormalImpulseMagnitudeNs = 0.0;
    double TangentImpulseMagnitudeNs = 0.0;
    double AbsorbedEnergyJ = 0.0;
    double PredictedDamageIncreaseA = 0.0;
    double PredictedDamageIncreaseB = 0.0;
    double RecommendedPositionCorrectionCm = 0.0;
    double EffectiveRestitution = 0.0;
    double EffectiveFriction = 0.0;
    bool Separating = false;
    bool EngineContactVerified = false;
    bool PhysicallyResolvedByChaos = false;
    bool Valid = false;
};
inline CollisionMaterial CollisionMaterialFor(
    ImpactMaterial kind) noexcept
{
    switch (kind)
    {
    case ImpactMaterial::Steel:
        return {kind,0.20,0.55,0.15,4500.0,0.15,1.0};
    case ImpactMaterial::Concrete:
        return {kind,0.08,0.75,0.05,6500.0,0.08,1.0};
    case ImpactMaterial::ArmcoBarrier:
        return {kind,0.22,0.65,0.35,4000.0,0.30,1.0};
    case ImpactMaterial::TireBarrier:
        return {kind,0.15,0.85,0.78,2500.0,0.66,1.0};
    case ImpactMaterial::PlasticCone:
        return {kind,0.30,0.50,0.90,50.0,0.90,1.0};
    case ImpactMaterial::VehicleBody:
        return {kind,0.14,0.80,0.50,1600.0,0.45,1.0};
    case ImpactMaterial::Gravel:
        return {kind,0.04,0.95,0.88,200.0,0.75,1.0};
    default:
        return {kind,0.0,0.0,0.0,0.0,0.0,0.0};
    }
}
inline bool ValidCollisionMaterial(CollisionMaterial s) noexcept
{
    return s.Kind != ImpactMaterial::Unknown &&
        Finite(s.Restitution) && s.Restitution >= 0.0 &&
        s.Restitution <= 1.0 &&
        Finite(s.Friction) && s.Friction >= 0.0 &&
        s.Friction <= 2.0 &&
        Finite(s.EnergyAbsorptionFraction) &&
        s.EnergyAbsorptionFraction >= 0.0 &&
        s.EnergyAbsorptionFraction <= 1.0 &&
        Finite(s.PermanentDeformationThresholdJ) &&
        s.PermanentDeformationThresholdJ >= 0.0 &&
        Finite(s.DamageSofteningFraction) &&
        s.DamageSofteningFraction >= 0.0 &&
        s.DamageSofteningFraction <= 1.0;
}
inline bool ValidCollisionBody(CollisionBody b) noexcept
{
    return b.RealRigidBodyVerified &&
        b.ActorToken != 0 &&
        Finite(b.PositionCm) &&
        Finite(b.VelocityCmS) &&
        Finite(b.HeadingRadians) &&
        Finite(b.YawRateRadS) &&
        Finite(b.MassKg) &&
        Finite(b.YawInertiaKgM2) &&
        Finite(b.HalfSizeCm) &&
        Finite(b.CollisionDamageFraction) &&
        b.MassKg >= 1.0 &&
        b.MassKg <= 50000.0 &&
        b.YawInertiaKgM2 > 0.0 &&
        b.HalfSizeCm.X > 1.0 &&
        b.HalfSizeCm.Y > 1.0 &&
        b.CollisionDamageFraction >= 0.0 &&
        b.CollisionDamageFraction <= 1.0 &&
        Length(b.VelocityCmS) <=
            MaximumPhysicalSpeedCmS;
}
inline bool ValidCollisionObservation(
    CollisionObservation s) noexcept
{
    return s.OriginalEngineContactReported &&
        s.NativeCollisionImpulsesReadOnly &&
        ValidCollisionBody(s.A) &&
        ValidCollisionBody(s.B) &&
        s.A.ActorToken != s.B.ActorToken &&
        Finite(s.WorldContactPointCm) &&
        Finite(s.WorldContactNormal) &&
        Finite(s.MeasuredPenetrationCm) &&
        s.MeasuredPenetrationCm >= 0.0 &&
        s.MeasuredPenetrationCm <= 300.0 &&
        Finite(s.RelativeNormalSpeedCmS) &&
        ValidFrameDelta(s.DeltaSeconds) &&
        Length(s.WorldContactNormal) >
            PhysicsEpsilon;
}
inline Vec2 ContactPointVelocity(
    CollisionBody body, Vec2 pointWorldCm) noexcept
{
    const Vec2 rMeters = (pointWorldCm -
                         body.PositionCm) /
        CentimetersPerMeter;
    return body.VelocityCmS /
        CentimetersPerMeter +
        PerpendicularLeft(rMeters) *
            body.YawRateRadS;
}
inline double ContactEffectiveMassInverse(
    CollisionBody body,
    Vec2 relativeContactMeters,
    Vec2 contactDirection) noexcept
{
    const double angularLever =
        Cross(relativeContactMeters, contactDirection);
    return SafeDivision(1.0, body.MassKg) +
        SafeDivision(Square(angularLever),
                     body.YawInertiaKgM2);
}
inline double PredictVehicleDamage(
    double energyJ,
    CollisionMaterial material,
    double priorDamage) noexcept
{
    if (!Finite(energyJ) || energyJ <= 0.0 ||
        !Finite(priorDamage) ||
        priorDamage < 0.0 ||
        priorDamage > 1.0) return 0.0;
    const double absorbed =
        energyJ * material.EnergyAbsorptionFraction;
    const double excessive = std::max(
        0.0,
        absorbed - material.PermanentDeformationThresholdJ);
    const double damage = excessive /
        (material.PermanentDeformationThresholdJ + 10000.0);
    return std::clamp(
        damage * (1.0 +
            priorDamage * material.DamageSofteningFraction),
        0.0, 1.0 - priorDamage);
}
inline CollisionImpulseResult ResolveMeasuredCollision(
    CollisionObservation sample) noexcept
{
    CollisionImpulseResult out;
    if (!ValidCollisionObservation(sample)) return out;
    const CollisionMaterial a =
        CollisionMaterialFor(sample.MaterialA);
    const CollisionMaterial b =
        CollisionMaterialFor(sample.MaterialB);
    if (!ValidCollisionMaterial(a) ||
        !ValidCollisionMaterial(b))
        return out;
    const Vec2 normal =
        Normalized(sample.WorldContactNormal);
    const Vec2 tangent =
        PerpendicularLeft(normal);
    const Vec2 velocityA = ContactPointVelocity(
        sample.A, sample.WorldContactPointCm);
    const Vec2 velocityB = ContactPointVelocity(
        sample.B, sample.WorldContactPointCm);
    const Vec2 relative = velocityB - velocityA;
    const double relativeNormal = Dot(relative, normal);
    const double relativeTangent = Dot(relative, tangent);
    out.RelativeNormalSpeedCmS =
        relativeNormal * CentimetersPerMeter;
    out.RelativeTangentialSpeedCmS =
        relativeTangent * CentimetersPerMeter;
    out.Separating = relativeNormal >= 0.0;
    const Vec2 rA = (sample.WorldContactPointCm -
        sample.A.PositionCm) / CentimetersPerMeter;
    const Vec2 rB = (sample.WorldContactPointCm -
        sample.B.PositionCm) / CentimetersPerMeter;
    const double inverseMassNormal =
        ContactEffectiveMassInverse(sample.A, rA, normal) +
        ContactEffectiveMassInverse(sample.B, rB, normal);
    const double inverseMassTangent =
        ContactEffectiveMassInverse(sample.A, rA, tangent) +
        ContactEffectiveMassInverse(sample.B, rB, tangent);
    const double restitution =
        std::min(a.Restitution, b.Restitution) *
        (1.0 - 0.5 *
            (sample.A.CollisionDamageFraction +
             sample.B.CollisionDamageFraction));
    const double friction =
        std::sqrt(a.Friction * b.Friction);
    out.EffectiveRestitution = restitution;
    out.EffectiveFriction = friction;
    if (!out.Separating)
    {
        const double normalImpulse = std::max(0.0,
            SafeDivision(
                -(1.0 + restitution) * relativeNormal,
                inverseMassNormal));
        const double uncappedTangent = SafeDivision(
            -relativeTangent, inverseMassTangent);
        const double tangentImpulse = std::clamp(
            uncappedTangent,
            -friction * normalImpulse,
            friction * normalImpulse);
        out.NormalImpulseNs =
            normal * normalImpulse;
        out.TangentialImpulseNs =
            tangent * tangentImpulse;
        out.AppliedImpulseToBNs =
            out.NormalImpulseNs +
            out.TangentialImpulseNs;
        out.AppliedImpulseToANs =
            out.AppliedImpulseToBNs * -1.0;
        out.NormalImpulseMagnitudeNs =
            normalImpulse;
        out.TangentImpulseMagnitudeNs =
            std::abs(tangentImpulse);
    }
    out.PredictedAVelocityCmS =
        sample.A.VelocityCmS +
        out.AppliedImpulseToANs *
        (CentimetersPerMeter / sample.A.MassKg);
    out.PredictedBVelocityCmS =
        sample.B.VelocityCmS +
        out.AppliedImpulseToBNs *
        (CentimetersPerMeter / sample.B.MassKg);
    out.PredictedAYawRateRadS =
        sample.A.YawRateRadS +
        SafeDivision(
            Cross(rA, out.AppliedImpulseToANs),
            sample.A.YawInertiaKgM2);
    out.PredictedBYawRateRadS =
        sample.B.YawRateRadS +
        SafeDivision(
            Cross(rB, out.AppliedImpulseToBNs),
            sample.B.YawInertiaKgM2);
    const double reducedMassKg =
        SafeDivision(sample.A.MassKg * sample.B.MassKg,
                     sample.A.MassKg + sample.B.MassKg);
    const double relativeSpeed =
        std::max(0.0, -relativeNormal);
    const double kineticEnergyJ =
        0.5 * reducedMassKg * Square(relativeSpeed);
    out.AbsorbedEnergyJ = kineticEnergyJ *
        (1.0 - Square(restitution));
    out.PredictedDamageIncreaseA =
        PredictVehicleDamage(
            0.5 * out.AbsorbedEnergyJ, a,
            sample.A.CollisionDamageFraction);
    out.PredictedDamageIncreaseB =
        PredictVehicleDamage(
            0.5 * out.AbsorbedEnergyJ, b,
            sample.B.CollisionDamageFraction);
    out.RecommendedPositionCorrectionCm =
        std::max(0.0,
            sample.MeasuredPenetrationCm - 1.0) *
        0.25;
    out.EngineContactVerified = true;
    out.PhysicallyResolvedByChaos = false;
    out.Valid = Finite(out.PredictedAVelocityCmS) &&
        Finite(out.PredictedBVelocityCmS) &&
        Finite(out.PredictedAYawRateRadS) &&
        Finite(out.PredictedBYawRateRadS) &&
        Finite(out.AbsorbedEnergyJ);
    return out;
}
struct CollisionHealthReview final
{
    double MeanContactEnergyJ = 0.0;
    double MaximumContactEnergyJ = 0.0;
    double TotalAbsorbedEnergyJ = 0.0;
    double MaximumDamageDelta = 0.0;
    double MeanPenetrationCm = 0.0;
    double MaximumTangentialImpulseNs = 0.0;
    int VerifiedContacts = 0;
    int SeparatingContacts = 0;
    int UnqualifiedContacts = 0;
    bool AnyRealCarPhysicsModified = false;
};
class CollisionImpactLedger final
{
public:
    void Reset() noexcept { Review = {}; }
    bool Observe(CollisionObservation sample) noexcept
    {
        const auto result = ResolveMeasuredCollision(sample);
        if (!result.Valid)
        {
            ++Review.UnqualifiedContacts;
            return false;
        }
        ++Review.VerifiedContacts;
        Review.SeparatingContacts +=
            result.Separating ? 1 : 0;
        const double count =
            static_cast<double>(Review.VerifiedContacts);
        Review.MeanContactEnergyJ +=
            (result.AbsorbedEnergyJ -
             Review.MeanContactEnergyJ) / count;
        Review.MeanPenetrationCm +=
            (sample.MeasuredPenetrationCm -
             Review.MeanPenetrationCm) / count;
        Review.TotalAbsorbedEnergyJ +=
            result.AbsorbedEnergyJ;
        Review.MaximumContactEnergyJ = std::max(
            Review.MaximumContactEnergyJ,
            result.AbsorbedEnergyJ);
        Review.MaximumDamageDelta = std::max({
            Review.MaximumDamageDelta,
            result.PredictedDamageIncreaseA,
            result.PredictedDamageIncreaseB
        });
        Review.MaximumTangentialImpulseNs = std::max(
            Review.MaximumTangentialImpulseNs,
            result.TangentImpulseMagnitudeNs);
        Review.AnyRealCarPhysicsModified = false;
        return true;
    }
    CollisionHealthReview Snapshot() const noexcept
    {
        return Review;
    }
private:
    CollisionHealthReview Review{};
};
} // namespace latebrake::racecraft
