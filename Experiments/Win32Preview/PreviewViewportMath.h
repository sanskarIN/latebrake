#pragma once

// Allocation-free coordinate transformation for the separate Win32/GDI
// LateBrake V5 2D trial, NOT the Unreal 5.8 3D game.
// Win32 client geometry uses signed 32-bit integers; intermediate products
// must use 64-bit arithmetic or a large/resized desktop can overflow
// client mouse hit testing and accidentally select a menu option.
namespace latebrake_preview
{
inline constexpr int PreviewCanvasWidth = 960;
inline constexpr int PreviewCanvasHeight = 640;

struct CanvasViewport final
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

// Uniformly fit the fixed 3:2 canvas into a client rectangle without
// cropping gameplay or triggering UI clicks in the letterbox/pillarbox.
// All intermediates are bounded by INT_MAX * 960 (below signed 64-bit).
inline CanvasViewport FitPreviewViewport(int clientW, int clientH) noexcept
{
    if (clientW <= 0 || clientH <= 0)
    {
        return {};
    }

    const long long scaledW =
        static_cast<long long>(clientW) * PreviewCanvasHeight;
    const long long scaledH =
        static_cast<long long>(clientH) * PreviewCanvasWidth;

    int width = clientW;
    int height = clientH;
    if (scaledW > scaledH)
    {
        width = static_cast<int>(
            static_cast<long long>(clientH) * PreviewCanvasWidth /
            PreviewCanvasHeight);
    }
    else
    {
        height = static_cast<int>(
            static_cast<long long>(clientW) * PreviewCanvasHeight /
            PreviewCanvasWidth);
    }
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    return {(clientW - width) / 2, (clientH - height) / 2,
            width, height};
}

// Reject invalid rectangles and black-bar input before writing outputs.
// Do NOT perform (viewport.x + viewport.w) or (dx * canvasW) as signed int;
// either can overflow even when the original caller supplied finite HWND
// client dimensions. Preserve the caller's prior coordinates on rejection.
inline bool TranslatePreviewMouse(
    const CanvasViewport& viewport,
    int clientX, int clientY,
    int& outX, int& outY) noexcept
{
    if (viewport.x < 0 || viewport.y < 0 ||
        viewport.w <= 0 || viewport.h <= 0)
    {
        return false;
    }

    const long long relativeX =
        static_cast<long long>(clientX) - viewport.x;
    const long long relativeY =
        static_cast<long long>(clientY) - viewport.y;
    if (relativeX < 0 || relativeY < 0 ||
        relativeX >= viewport.w || relativeY >= viewport.h)
    {
        return false;
    }

    const long long canvasX =
        relativeX * PreviewCanvasWidth / viewport.w;
    const long long canvasY =
        relativeY * PreviewCanvasHeight / viewport.h;
    if (canvasX < 0 || canvasX >= PreviewCanvasWidth ||
        canvasY < 0 || canvasY >= PreviewCanvasHeight)
    {
        return false;
    }
    outX = static_cast<int>(canvasX);
    outY = static_cast<int>(canvasY);
    return true;
}
} // namespace latebrake_preview
