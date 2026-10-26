#include "Racing/LatebrakeCheckpoint.h"
#include "Racing/CircuitGateCrossing.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "Vehicles/LatebrakeCarPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

// Entirely opt-in and only available for development/editor builds.
// No network telemetry; no per-frame output when disabled (default).
namespace
{
TAutoConsoleVariable<int32> CVarLatebrakeCheckpointDiagnostics(
    TEXT("latebrake.CheckpointDiagnostics"), 0,
    TEXT("0 (default): silent; 1: local event/rejection transitions; "
         "2: aggregated 10-second QA counters. Non-shipping builds only."),
    ECVF_Default);
} // namespace
#endif

ALatebrakeCheckpoint::ALatebrakeCheckpoint()
{
    // True plane crossing is decided from successive player positions,
    // not the BEGIN OVERLAP event at the leading rim of a 600cm sphere.
    // Four lightweight actor ticks (no allocations or traces) evaluate
    // the shared engine-neutral circuit gate model.
    PrimaryActorTick.bCanEverTick = true;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("CheckpointTrigger"));
    RootComponent = Trigger;
    Trigger->InitSphereRadius(600.f);
    // Retain the legacy root component for actor transform/visual marker;
    // a geometric crossing no longer needs an overlap collision event.
    Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Trigger->SetGenerateOverlapEvents(false);

    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointMarker"));
    Marker->SetupAttachment(Trigger);
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetRelativeScale3D(FVector(11.f, 11.f, 0.08f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded())
    {
        Marker->SetStaticMesh(Cylinder.Object);
    }
}

void ALatebrakeCheckpoint::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ALatebrakeCarPawn* Car =
        Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!IsValid(Car) || !Car->IsRacePhysicsReady())
    {
        // A chassis that stopped simulating or lost its blocking mesh
        // cannot provide a meaningful gate crossing. Clear the sampled
        // trajectory so a repaired/replaced car starts from a fresh
        // position rather than interpolating across an invalid interval.
        LastObservedPlayer.Reset();
        MotionTrace.Reset();
#if !UE_BUILD_SHIPPING
        bHadLoggedDecision = false;
        DiagnosticWindow.Reset();
#endif
        return;
    }

    if (LastObservedPlayer.Get() != Car)
    {
        // A new pawn is not a continuation of the previous car's trajectory.
        LastObservedPlayer = Car;
        MotionTrace.Reset();
#if !UE_BUILD_SHIPPING
        bHadLoggedDecision = false;
        DiagnosticWindow.Reset();
#endif
    }

    // Session revisions are bumped by GameMode on both pause and resume;
    // an actor need not tick while paused to reject pre-pause movement.
    ALatebrakeRaceGameMode* Mode =
        GetWorld() ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!IsValid(Mode) || !Mode->IsCircuitReadyForGameplay() ||
        !Mode->IsRaceStartGreen() ||
        UGameplayStatics::IsGamePaused(this) || Mode->IsMenuOpen() ||
        (Mode->IsFocusLossGuardActive() || Mode->IsManualRacePauseActive()))
    {
        MotionTrace.Reset();
#if !UE_BUILD_SHIPPING
        bHadLoggedDecision = false;
        DiagnosticWindow.Reset();
#endif
        return;
    }
    const uint64 SessionRevision = Mode->GetCheckpointSessionRevision();
#if !UE_BUILD_SHIPPING
    if (MotionTrace.IsNewSession(SessionRevision))
    {
        // Diagnostic evidence must not span unrelated pause/resume windows.
        bHadLoggedDecision = false;
        DiagnosticWindow.Reset();
    }
#endif
    const FVector Current = Car->GetActorLocation();
    const FVector Velocity = Car->GetVelocity();
    const latebrake::CheckpointTraceResult Result = MotionTrace.SampleDetailed(
        CheckpointIndex, Car->GetTeleportRevision(), SessionRevision,
        {{Current.X, Current.Y}, Current.Z}, Velocity.X, Velocity.Y);

#if !UE_BUILD_SHIPPING
    const int32 DiagnosticLevel =
        CVarLatebrakeCheckpointDiagnostics.GetValueOnGameThread();
    if (DiagnosticLevel == 1)
    {
        const bool bEvent = Result.Crossed() ||
            Result.Trace == latebrake::CheckpointTraceDecision::TeleportRebase ||
            Result.Trace == latebrake::CheckpointTraceDecision::InvalidPosition;
        const bool bNewUsefulRejection =
            Result.Trace == latebrake::CheckpointTraceDecision::GateRejected &&
            Result.Gate != latebrake::CircuitGateDecision::NoForwardPlaneCrossing &&
            (!bHadLoggedDecision || Result.Gate != LastLoggedDecision);
        if (bEvent || bNewUsefulRejection)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Latebrake checkpoint=%d trace=%s reason=%s dt=%.4f "
                     "vx=%.1f vy=%.1f generation=%llu"),
                CheckpointIndex,
                UTF8_TO_TCHAR(latebrake::CheckpointTraceDecisionName(Result.Trace)),
                UTF8_TO_TCHAR(latebrake::CircuitGateDecisionName(Result.Gate)),
                DeltaSeconds, Velocity.X, Velocity.Y,
                static_cast<unsigned long long>(Car->GetTeleportRevision()));
        }
        bHadLoggedDecision = true;
        LastLoggedDecision = Result.Gate;
        // A change from aggregate mode starts a clean new summary window.
        DiagnosticWindow.Reset();
    }
    else if (DiagnosticLevel == 2)
    {
        // Level 2 records bounded aggregated categories only. There are no
        // frame-by-frame logs, coordinate traces or remote telemetry.
        // Portable bounded-window logic is covered by CTest. An invalid
        // frame delta or a single huge hitch cannot advance a full window.
        if (DiagnosticWindow.Record(Result, static_cast<double>(DeltaSeconds)))
        {
            UE_LOG(LogTemp, Display,
                TEXT("Latebrake checkpoint=%d qa-samples=%u crossed=%u "
                     "teleport-rebase=%u invalid-position=%u wrong-way=%u "
                     "frame-travel=%u lane-miss=%u height-miss=%u no-plane=%u "
                     "invalid-velocity=%u"),
                CheckpointIndex, DiagnosticWindow.Counts().TotalSamples(),
                DiagnosticWindow.Counts().CountTrace(
                    latebrake::CheckpointTraceDecision::Crossed),
                DiagnosticWindow.Counts().CountTrace(
                    latebrake::CheckpointTraceDecision::TeleportRebase),
                DiagnosticWindow.Counts().CountTrace(
                    latebrake::CheckpointTraceDecision::InvalidPosition),
                DiagnosticWindow.Counts().CountGateRejection(
                    latebrake::CircuitGateDecision::NotMovingForward),
                DiagnosticWindow.Counts().CountGateRejection(
                    latebrake::CircuitGateDecision::ExcessiveFrameTravel),
                DiagnosticWindow.Counts().CountGateRejection(
                    latebrake::CircuitGateDecision::OutsideLateralCorridor),
                DiagnosticWindow.Counts().CountGateRejection(
                    latebrake::CircuitGateDecision::OutsideVerticalCorridor),
                DiagnosticWindow.Counts().CountGateRejection(
                    latebrake::CircuitGateDecision::NoForwardPlaneCrossing),
                DiagnosticWindow.Counts().CountGateRejection(
                    latebrake::CircuitGateDecision::InvalidVelocity));
            DiagnosticWindow.Reset();
        }
        bHadLoggedDecision = false;
    }
    else
    {
        // Leaving diagnostic mode never carries stale counters or reason
        // state into a future QA session.
        DiagnosticWindow.Reset();
        bHadLoggedDecision = false;
    }
#endif

    // The shared 3D plane geometry rejects wrong-way, airborne and large
    // jumps. A changed teleport generation always rebases the trace first,
    // so even a short, paid RECOVER or RESTART cannot credit a checkpoint.
    // RaceProgress still requires the correct ORDERED gate and lap count.
    if (Result.Crossed())
    {
        Mode->RegisterCheckpoint(CheckpointIndex);
    }
}
