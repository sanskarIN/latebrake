#include "Vehicles/OriginalChaosSuspensionPreview.h"

#include "Vehicles/ChassisWheelAnchors.h"
#include "Vehicles/SprungChassisDynamics.h"
#include "Vehicles/SuspensionRayVelocity.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "CollisionQueryParams.h"

#include <array>
#include <cmath>

namespace latebrake::vehiclepreview
{
namespace
{
bool SourceFinite(const FVector& vector) noexcept
{
    return FMath::IsFinite(vector.X) &&
        FMath::IsFinite(vector.Y) &&
        FMath::IsFinite(vector.Z);
}
bool GenuineWorldStaticHit(
    const FHitResult& hit,UStaticMeshComponent* chassis) noexcept
{
    const UPrimitiveComponent* component=hit.GetComponent();
    return hit.bBlockingHit &&
        IsValid(component) &&
        component!=chassis &&
        component->GetCollisionObjectType()==ECC_WorldStatic &&
        component->GetMobility()==EComponentMobility::Static &&
        SourceFinite(hit.ImpactPoint) &&
        SourceFinite(hit.ImpactNormal) &&
        FMath::IsFinite(hit.Distance) &&
        hit.Distance>=0.0f;
}
}

OriginalSprungPreviewReceipt ApplyOriginalChaosSprungPreview(
    AActor* owner,
    UStaticMeshComponent* chassis,
    float frameSeconds) noexcept
{
    OriginalSprungPreviewReceipt receipt;
    UWorld* world=IsValid(owner)?owner->GetWorld():nullptr;
    if(!IsValid(world) || !IsValid(chassis) ||
       !chassis->IsSimulatingPhysics() ||
       chassis->GetCollisionEnabled()==ECollisionEnabled::NoCollision ||
       !FMath::IsFinite(frameSeconds) ||
       frameSeconds<=0.0f ||
       frameSeconds>latebrake::SuspensionMaximumFrameSeconds)
    {
        receipt.Fault=OriginalSprungPreviewFault::InvalidEngineState;
        return receipt;
    }
    const FVector center=chassis->GetComponentLocation();
    const FVector forward=chassis->GetForwardVector();
    const FVector right=chassis->GetRightVector();
    const FVector up=chassis->GetUpVector();
    const double mass=chassis->GetMass();
    const double gravity=-static_cast<double>(world->GetGravityZ());
    if(!SourceFinite(center) || !SourceFinite(forward) ||
       !SourceFinite(right) || !SourceFinite(up) ||
       !std::isfinite(mass) || mass<=0.0 ||
       !std::isfinite(gravity) || gravity<=0.0)
    {
        receipt.Fault=OriginalSprungPreviewFault::InvalidPhysicsSource;
        return receipt;
    }
    latebrake::SuspensionFrame frame;
    frame.DeltaSeconds=static_cast<double>(frameSeconds);
    frame.MassKg=mass;
    frame.GravityCmPerSecondSquared=gravity;
    frame.ChassisUpZ=up.Z;
    frame.PhysicsReady=true;

    std::array<FVector,latebrake::ChassisWheelCount> origins{};
    FCollisionQueryParams query(SCENE_QUERY_STAT(LatebrakeOriginalSprungPreview),
                                false,owner);
    query.bReturnPhysicalMaterial=false;
    // Every corner is sampled independently against real blocking STATIC
    // WorldStatic. No inferred wheel contact, fake tire mesh or race code.
    for(std::size_t i=0;i<latebrake::ChassisWheelCount;++i)
    {
        const auto anchor=latebrake::ChassisProbeOffset(
            static_cast<unsigned>(i));
        const FVector origin=center+
            forward*anchor.ForwardCm+
            right*anchor.RightCm;
        origins[i]=origin;
        if(!SourceFinite(origin))
        {
            receipt.Fault=OriginalSprungPreviewFault::InvalidPhysicsSource;
            return receipt;
        }
        const FVector end=origin+
            FVector(0.0,0.0,-latebrake::SuspensionMaxTraceDistanceCm);
        if(!SourceFinite(end))
        {
            receipt.Fault=OriginalSprungPreviewFault::InvalidPhysicsSource;
            return receipt;
        }
        FHitResult hit;
        const bool blocked=world->LineTraceSingleByChannel(
            hit,origin,end,ECC_WorldStatic,query);
        ++receipt.PhysicalRaysCast;
        auto& corner=frame.Corners[i];
        if(!blocked)
            continue;
        if(!GenuineWorldStaticHit(hit,chassis))
        {
            receipt.Fault=OriginalSprungPreviewFault::UntrustedRoadContact;
            return receipt;
        }
        corner.Hit=true;
        corner.DistanceCm=static_cast<double>(hit.Distance);
        corner.NormalZ=static_cast<double>(hit.ImpactNormal.Z);
        const FVector pointVelocity=
            chassis->GetPhysicsLinearVelocityAtPoint(origin);
        if(!SourceFinite(pointVelocity))
        {
            receipt.Fault=OriginalSprungPreviewFault::InvalidPointVelocity;
            return receipt;
        }
        const auto rate=latebrake::MeasureSuspensionRayVelocity({
            static_cast<double>(pointVelocity.X),
            static_cast<double>(pointVelocity.Y),
            static_cast<double>(pointVelocity.Z),
            static_cast<double>(hit.ImpactNormal.X),
            static_cast<double>(hit.ImpactNormal.Y),
            static_cast<double>(hit.ImpactNormal.Z)});
        if(!rate.Valid)
        {
            receipt.Fault=OriginalSprungPreviewFault::InvalidPointVelocity;
            return receipt;
        }
        corner.RayDistanceSpeedCmPerSecond=
            rate.DistanceSpeedCmPerSecond;
        if(rate.RateLimited)
            ++receipt.RateLimitedCorners;
        ++receipt.RealBlockingHits;
    }
    if(receipt.PhysicalRaysCast!=4)
    {
        receipt.Fault=OriginalSprungPreviewFault::MissingCornerSamples;
        return receipt;
    }

    const auto response=latebrake::CalculateSprungChassis(frame);
    if(!response.Valid ||
       !std::isfinite(response.TotalUpForce) ||
       !std::isfinite(response.WeightForce) ||
       response.ContactCount<0 ||
       response.ContactCount>4 ||
       response.ContactCount>receipt.RealBlockingHits ||
       response.TotalUpForce<0.0 ||
       response.WeightForce<=0.0)
    {
        receipt.Fault=OriginalSprungPreviewFault::UntrustedSpringResponse;
        return receipt;
    }
    // Do not apply ANY spring before all four independent blocking rays
    // and the complete conservative portable force calculation passed.
    // Forces change ONLY the existing Chaos chassis, never fictitious
    // wheel rigid bodies, installed road collision, or race classification.
    for(std::size_t i=0;i<latebrake::ChassisWheelCount;++i)
    {
        const auto& corner=response.Corners[i];
        if(corner.Contact && corner.UpForce>0.0)
        {
            chassis->AddForceAtLocation(
                FVector(0.0,0.0,corner.UpForce),
                origins[i]);
            ++receipt.ForcesSubmitted;
        }
    }
    receipt.Fault=OriginalSprungPreviewFault::None;
    receipt.Valid=true;
    receipt.RealSprungContacts=response.ContactCount;
    receipt.TotalAppliedUpForce=response.TotalUpForce;
    receipt.ChassisWeightForce=response.WeightForce;
    receipt.MaximumCornerForce=0.0;
    for(const auto& corner:response.Corners)
    {
        receipt.MaximumCornerForce=std::max(
            receipt.MaximumCornerForce,corner.UpForce);
    }
    receipt.OriginalChaosChassisUsed=true;
    receipt.WheelChaosIntegrationValidated=false;
    receipt.NativeInstalledRaceValidated=false;
    return receipt;
}
} // namespace latebrake::vehiclepreview
