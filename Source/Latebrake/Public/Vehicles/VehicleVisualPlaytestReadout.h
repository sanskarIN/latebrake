#pragma once

namespace latebrake
{
// Reports only the native Unreal code's OWN observation/command bits.
// These are useful to a human inspecting the real screen, but cannot
// establish whether UE actually rasterized the car or lit a physical road.
enum class VisualPlaytestSurface
{
    NoDriver,
    LegacyCubeFallback,
    SourceGeneratedSurface,
    SourceMaterialIncomplete
};

struct VisualPlaytestReport final
{
    VisualPlaytestSurface Surface = VisualPlaytestSurface::NoDriver;
    bool PaintParameterDiscovered = false;
    bool FrontLampsRequested = false;
    bool DuskPreviewSelected = false;
    // Source-observed initialized native wheel meshes, never GPU pixels.
    int AnimatedWheelPartsCommitted = 0;
    bool VerifiedByEngineRendering = false;
};

constexpr VisualPlaytestReport ReportVisualPlaytest(
    bool hasDriver, bool generatedMeshCommitted,
    bool tintParameterDiscovered, bool frontLampsRequested,
    bool duskPreviewSelected, int nativeWheelCount=0) noexcept
{
    VisualPlaytestReport report{};
    report.PaintParameterDiscovered =
        hasDriver && tintParameterDiscovered;
    report.FrontLampsRequested = hasDriver && frontLampsRequested;
    report.DuskPreviewSelected = duskPreviewSelected;
    report.AnimatedWheelPartsCommitted =
        hasDriver && generatedMeshCommitted && tintParameterDiscovered &&
        nativeWheelCount>=0 && nativeWheelCount<=4
            ? nativeWheelCount : 0;
    report.VerifiedByEngineRendering = false;
    if (!hasDriver) return report;
    report.Surface = generatedMeshCommitted
        ? (tintParameterDiscovered
            ? VisualPlaytestSurface::SourceGeneratedSurface
            : VisualPlaytestSurface::SourceMaterialIncomplete)
        : VisualPlaytestSurface::LegacyCubeFallback;
    return report;
}

constexpr const char* VisualPlaytestSurfaceLabel(
    VisualPlaytestSurface surface) noexcept
{
    switch (surface)
    {
    case VisualPlaytestSurface::NoDriver: return "NO PLAYER PAWN";
    case VisualPlaytestSurface::LegacyCubeFallback:
        return "LEGACY CUBE FALLBACK";
    case VisualPlaytestSurface::SourceGeneratedSurface:
        return "SOURCE MESH COMMITTED";
    case VisualPlaytestSurface::SourceMaterialIncomplete:
        return "SOURCE MESH / PAINT INCOMPLETE";
    }
    return "UNKNOWN";
}
static_assert(!ReportVisualPlaytest(true,true,true,true,true)
    .VerifiedByEngineRendering);
static_assert(ReportVisualPlaytest(false,true,true,true,false)
    .Surface==VisualPlaytestSurface::NoDriver);
} // namespace latebrake
