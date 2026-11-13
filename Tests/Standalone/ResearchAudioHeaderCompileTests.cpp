// Compile every standalone Audio domain header together under
// -Werror / /WX. No Unreal SDK, native actors, hardware or assets used.
// This guards transitive includes, ODR/type collisions and missing headers;
// separate behavioral suites test runtime values and safety decisions.
#include "Audio/OriginalSixCarPcmSynth.h"
#include "Audio/PhysicalSixCarAudioStage.h"
#include "Audio/PhysicalVehicleSoundMix.h"

#include <iostream>
int main()
{
    std::cout << "Audio: all 3 standalone headers parsed.\\n";
    return 0;
}
