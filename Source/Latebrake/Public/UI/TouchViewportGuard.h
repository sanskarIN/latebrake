#pragma once

namespace latebrake
{
// A held finger is expressed in pixel coordinates from an *earlier* viewport.
// When screen size changes, that finger cannot safely keep steering or
// accelerating against a newly positioned layout. This portable guard
// owns only the viewport epoch; TouchPressTracker retains physical fingers.
enum class TouchViewportStatus
{
    Ready,
    Changed,
    Unavailable
};

class TouchViewportGuard final
{
public:
    // A viewport is useful only if the shared touch pad geometry can be
    // calculated. Invalid dimensions cancel drive, and the next valid
    // viewport starts with a fresh baseline (without reacquiring old fingers).
    TouchViewportStatus Observe(int width, int height) noexcept
    {
        if (width < 240 || height < 240)
        {
            Reset();
            return TouchViewportStatus::Unavailable;
        }
        if (!HasViewport)
        {
            Width = width;
            Height = height;
            HasViewport = true;
            return TouchViewportStatus::Ready;
        }
        if (Width != width || Height != height)
        {
            Width = width;
            Height = height;
            return TouchViewportStatus::Changed;
        }
        return TouchViewportStatus::Ready;
    }

    void Reset() noexcept
    {
        Width = 0;
        Height = 0;
        HasViewport = false;
    }

    bool HasBaseline() const noexcept { return HasViewport; }

private:
    int Width = 0;
    int Height = 0;
    bool HasViewport = false;
};

inline bool TouchViewportNeedsCancellation(TouchViewportStatus status) noexcept
{
    return status != TouchViewportStatus::Ready;
}
} // namespace latebrake
