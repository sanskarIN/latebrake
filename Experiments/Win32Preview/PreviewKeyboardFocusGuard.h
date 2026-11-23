#pragma once

// Keyboard focus recovery for the optional native Win32 2D preview.
// Unlike a controller's neutral packet, GetAsyncKeyState observes global key
// state and may report a key held before this game's window regained focus.
// Require TWO consecutive focused samples with no gameplay/UI key held before
// accepting keyboard edges or continuous throttle, steering and braking.
// Pointer input and controller input are governed independently.
namespace latebrake_preview
{
class PreviewKeyboardFocusGuard final
{
public:
    void OnFocusLost() noexcept
    {
        needsNeutral_ = true;
        neutralSamples_ = 0;
    }

    bool Sample(bool focused, bool trackedKeyHeld) noexcept
    {
        if (!focused)
        {
            OnFocusLost();
            return false;
        }
        if (!needsNeutral_)
        {
            return true;
        }
        if (trackedKeyHeld)
        {
            neutralSamples_ = 0;
            return false;
        }
        if (++neutralSamples_ < 2)
        {
            return false;
        }
        neutralSamples_ = 0;
        needsNeutral_ = false;
        return true;
    }

    bool IsArmed() const noexcept { return !needsNeutral_; }

private:
    bool needsNeutral_ = false;
    unsigned neutralSamples_ = 0;
};
} // namespace latebrake_preview
