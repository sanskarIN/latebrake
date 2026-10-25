#include "UI/FocusSuspendedTouchPolicy.h"

#include <array>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Focus-suspended touch policy FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    // No other visible driving/menu pad can wake a race with its clock
    // frozen; a repeat/move from PAUSE must not be treated as a new press.
    constexpr std::array Forbidden = {
        TouchControl::None,
        TouchControl::Steering,
        TouchControl::Throttle,
        TouchControl::Brake,
        TouchControl::Restart,
        TouchControl::Recover,
        TouchControl::CareerNext,
        TouchControl::CareerStart,
        TouchControl::MenuBrowse
    };
    for (const auto control : Forbidden)
    {
        Require(!CanResumeFocusSuspendedRaceFromTouch(true, control),
                "only PAUSE may explicitly resume a suspended race");
        Require(!CanResumeFocusSuspendedRaceFromTouch(false, control),
                "dragged or repeated touches cannot resume");
    }
    Require(!CanResumeFocusSuspendedRaceFromTouch(false, TouchControl::Pause),
            "a held PAUSE finger cannot resume via move or repeat");
    Require(CanResumeFocusSuspendedRaceFromTouch(true, TouchControl::Pause),
            "a genuinely new PAUSE press may resume intentionally");

    std::cout << "Focus-suspended touch resume policy passed.\n";
    return EXIT_SUCCESS;
}
