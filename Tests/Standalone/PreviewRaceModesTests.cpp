#include <initializer_list>
#include "PreviewRaceModes.h"

using latebrake_preview::GetRaceModePolicy;
using latebrake_preview::IsSoloMode;

static_assert(GetRaceModePolicy(0).RequiredLaps == 3);
static_assert(GetRaceModePolicy(0).RivalCount == 3);
static_assert(GetRaceModePolicy(1).RequiredLaps == 1);
static_assert(GetRaceModePolicy(1).RivalCount == 0);
static_assert(GetRaceModePolicy(2).RequiredLaps == 2);
static_assert(GetRaceModePolicy(2).RivalCount == 3);
static_assert(GetRaceModePolicy(3).RequiredLaps == 2);
static_assert(GetRaceModePolicy(3).RivalCount == 0);
static_assert(IsSoloMode(GetRaceModePolicy(1)));
static_assert(IsSoloMode(GetRaceModePolicy(3)));
static_assert(!IsSoloMode(GetRaceModePolicy(0)));
static_assert(!IsSoloMode(GetRaceModePolicy(-1)));

int main()
{
    for (int event = 0; event < 4; ++event)
    {
        const auto policy = GetRaceModePolicy(event);
        if (!policy.Valid || policy.RequiredLaps < 1 ||
            policy.RequiredLaps > 3 || policy.RivalCount < 0 ||
            policy.RivalCount > 3)
            return 1;
        if ((event == 1 || event == 3) != IsSoloMode(policy))
            return 2;
        if ((event == 0 || event == 2) != (policy.RivalCount == 3))
            return 3;
    }

    for (const int invalid : {-100, -1, 4, 99, 100000})
    {
        const auto policy = GetRaceModePolicy(invalid);
        if (policy.Valid || policy.RequiredLaps != 0 ||
            policy.RivalCount != 0 || IsSoloMode(policy))
            return 4;
    }
    return 0;
}
