#pragma once

namespace latebrake
{
// An input/UI preference, NOT an authorization to record best laps,
// modify RaceProgress, change physics or create personal-best records.
class GhostDisplayPreference final
{
public:
    bool IsEnabled() const noexcept { return Enabled; }

    // Ignore a ghost-toggle command while menu, OS focus or race input is
    // unavailable. Replay recording is deliberately independent.
    bool Toggle(bool authorizedHumanInput) noexcept
    {
        if (!authorizedHumanInput)
        {
            return false;
        }
        Enabled = !Enabled;
        return true;
    }

    bool CanRender(bool replayEligible) const noexcept
    {
        return Enabled && replayEligible;
    }

private:
    bool Enabled = true;
};
} // namespace latebrake
