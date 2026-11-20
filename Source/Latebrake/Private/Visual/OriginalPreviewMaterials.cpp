#include "Visual/OriginalPreviewMaterials.h"

#include "CoreMinimal.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

namespace latebrake::visual
{
UMaterialInterface* FindOriginalPreviewMaterial(
    OriginalPreviewMaterialKind kind,
    UMaterialInterface* fallback) noexcept
{
    const TCHAR* path=nullptr;
    switch(kind)
    {
    case OriginalPreviewMaterialKind::Road:
        path=TEXT("/Game/Latebrake/Materials/M_LatebrakeRoadPbr.M_LatebrakeRoadPbr");
        break;
    case OriginalPreviewMaterialKind::Steel:
        path=TEXT("/Game/Latebrake/Materials/M_LatebrakeSteelPbr.M_LatebrakeSteelPbr");
        break;
    case OriginalPreviewMaterialKind::CarPaint:
        path=TEXT("/Game/Latebrake/Materials/M_LatebrakeCarPaintPbr.M_LatebrakeCarPaintPbr");
        break;
    case OriginalPreviewMaterialKind::Glass:
        path=TEXT("/Game/Latebrake/Materials/M_LatebrakeGlassPbr.M_LatebrakeGlassPbr");
        break;
    case OriginalPreviewMaterialKind::Rubber:
        path=TEXT("/Game/Latebrake/Materials/M_LatebrakeRubberPbr.M_LatebrakeRubberPbr");
        break;
    case OriginalPreviewMaterialKind::Emissive:
        path=TEXT("/Game/Latebrake/Materials/M_LatebrakeEmissionPbr.M_LatebrakeEmissionPbr");
        break;
    }
    if(!path)
        return fallback;
    // This runs during opt-in scene construction, never in Tick. Loading
    // package assets from a dedicated Editor-generated /Game/ path avoids
    // inventing transient material graphs in a shipping runtime.
    if(UMaterialInterface* material=LoadObject<UMaterialInterface>(
        nullptr,path))
        return material;
    // The engine cube fallback may not consume vertex color. Emit a
    // persistent owner-visible diagnostic at scene construction rather
    // than silently claiming accurate colored car/kerb/track materials.
    // This cannot be a runtime shipping / verified-renderer test.
    UE_LOG(LogTemp,Warning,
        TEXT("LATEBRAKE_PREVIEW_PBR_MISSING asset=%s fallback=unverified"),
        path);
    return fallback;
}
}
