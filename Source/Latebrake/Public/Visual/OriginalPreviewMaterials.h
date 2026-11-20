#pragma once

class UMaterialInterface;

namespace latebrake::visual
{
// Assets must first be created and compiled by REAL Unreal Editor 5.8.
// They use each procedural section's vertex colors (not a missing "Color"
// parameter on the stock engine Cube). Asset lookup never touches collision,
// wheel friction, physics, timing, race classification or saved progression.
enum class OriginalPreviewMaterialKind
{
    Road, Steel, CarPaint, Glass, Rubber, Emissive
};
UMaterialInterface* FindOriginalPreviewMaterial(
    OriginalPreviewMaterialKind kind,
    UMaterialInterface* fallback) noexcept;
}
