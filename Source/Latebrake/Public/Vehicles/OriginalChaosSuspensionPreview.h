#pragma once

class AActor;
class UStaticMeshComponent;

namespace latebrake::vehiclepreview
{
enum class OriginalSprungPreviewFault
{
    None, InvalidEngineState, InvalidPhysicsSource,
    UntrustedRoadContact, InvalidPointVelocity,
    MissingCornerSamples, UntrustedSpringResponse
};
struct OriginalSprungPreviewReceipt final
{
    OriginalSprungPreviewFault Fault=
        OriginalSprungPreviewFault::InvalidEngineState;
    int PhysicalRaysCast=0;
    int RealBlockingHits=0;
    int RealSprungContacts=0;
    int ForcesSubmitted=0;
    int RateLimitedCorners=0;
    double TotalAppliedUpForce=0.0;
    double ChassisWeightForce=0.0;
    double MaximumCornerForce=0.0;
    bool OriginalChaosChassisUsed=false;
    bool WheelChaosIntegrationValidated=false;
    bool NativeInstalledRaceValidated=false;
    bool Valid=false;
};
// Non-shipping optical/handling experiment with REAL chassis and static
// blocking road rays; no dedicated Chaos wheel solver is claimed.
OriginalSprungPreviewReceipt ApplyOriginalChaosSprungPreview(
    AActor* owner,
    UStaticMeshComponent* chassis,
    float frameSeconds) noexcept;
}
