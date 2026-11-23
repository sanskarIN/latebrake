#include "PreviewKeyboardFocusGuard.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Preview keyboard focus failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestStartAndFocusRecovery()
{
    latebrake_preview::PreviewKeyboardFocusGuard guard{};
    Check(guard.IsArmed() && guard.Sample(true, true),
          "foreground gameplay may hold a normal key before a focus loss");
    guard.OnFocusLost();
    Check(!guard.IsArmed(), "focus loss immediately disarms keyboard");
    for (int i = 0; i < 1000; ++i)
    {
        Check(!guard.Sample(false, false),
              "keyboard cannot arm while window is in background");
    }
    Check(!guard.Sample(true, true),
          "a key held through Alt-Tab must not accelerate or choose a menu");
    Check(!guard.Sample(true, false),
          "one neutral frame is not sufficient to resume input");
    Check(!guard.Sample(true, true),
          "a newly held key resets the neutral frame counter");
    Check(!guard.Sample(true, false),
          "the first clean sample after a held key remains blocked");
    Check(guard.Sample(true, false) && guard.IsArmed(),
          "two successive neutral samples permit fresh input");
    Check(guard.Sample(true, true),
          "new player input is accepted after a genuine neutral release");
}

void TestRepeatedTransitions()
{
    latebrake_preview::PreviewKeyboardFocusGuard guard{};
    guard.OnFocusLost();
    Check(!guard.Sample(true, false), "first neutral frame blocked");
    guard.OnFocusLost();
    Check(!guard.Sample(true, false), "repeated loss cancels pending rearm");
    Check(guard.Sample(true, false), "second clean sample reactivates");
    guard.OnFocusLost();
    Check(!guard.Sample(false, true), "background held keys cannot arm");
    Check(!guard.Sample(true, false), "clean frame one after another focus");
    Check(guard.Sample(true, false), "clean frame two after another focus");
}

void TestDeterministicStress()
{
    latebrake_preview::PreviewKeyboardFocusGuard guard{};
    // Independent three-state reference: 0=armed; 1=disarmed; 2=one
    // clean foreground sample. Cover rapid focus changes and held keys.
    unsigned reference = 0;
    unsigned seed = 0x9e3779b9u;
    for (int i = 0; i < 250000; ++i)
    {
        seed = seed * 1664525u + 1013904223u;
        const bool focused = (seed & 7u) != 0u;
        const bool held = (seed & 32u) != 0u;
        if (!focused || held)
        {
            if (!focused || reference != 0) reference = 1;
        }
        else if (reference == 1) reference = 2;
        else if (reference == 2) reference = 0;
        const bool allowed = guard.Sample(focused, held);
        Check(allowed == (focused && reference == 0),
              "250k policy decisions match independent reference machine");
        Check(guard.IsArmed() == (reference == 0),
              "internal arming state matches reference machine");
    }
}
} // namespace

int main()
{
    TestStartAndFocusRecovery();
    TestRepeatedTransitions();
    TestDeterministicStress();
    std::cout << "Native preview keyboard focus rearm QA passed.\n";
    return EXIT_SUCCESS;
}
