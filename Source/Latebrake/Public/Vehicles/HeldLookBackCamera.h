#pragma once

namespace latebrake
{
// Engine-independent, input-owned transient camera state. Look-back is a
// *held* view, never a toggle and never changes movement or race authority.
class HeldLookBackCamera final
{
public:
    bool Press(bool eligible) noexcept
    {
        // A press while the local race cannot accept driving must not leave
        // a latent rear-view request for a later unpaused/restarted session.
        Held = eligible;
        return Held;
    }

    void Release() noexcept { Held = false; }
    void Reset() noexcept { Release(); }
    bool IsHeld() const noexcept { return Held; }
    double SpringArmYawDegrees() const noexcept
    {
        return Held ? 180.0 : 0.0;
    }

private:
    bool Held = false;
};
} // namespace latebrake
