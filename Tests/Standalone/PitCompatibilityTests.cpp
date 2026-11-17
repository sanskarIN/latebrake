#include "Telemetry/TelemetryPitReview.h"
#include "Telemetry/TelemetryPitStrategyReview.h"
#include "Telemetry/TelemetryRaceStrategy.h"

#include <cstdlib>
#include <iostream>
#include <type_traits>

namespace
{
void Require(bool ok, const char* reason)
{
    if (!ok)
    {
        std::cerr << "PitCompatibility FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake::engineering;
    static_assert(!std::is_same_v<PitReviewSettings, PitVisitReviewSettings>);
    const PitReviewSettings strategySettings{};
    const PitVisitReviewSettings visitSettings{};
    const RaceStrategySettings raceSettings{};

    Require(ValidPitReviewSettings(strategySettings),
            "existing pit scenario configuration remains valid");
    Require(ValidPitVisitReviewSettings(visitSettings),
            "imported physical pit visit monitor has distinct valid configuration");
    Require(ValidRaceStrategySettings(raceSettings),
            "overall race strategy remains source-valid with both pit reviews included");

    auto malformedStrategy = strategySettings;
    malformedStrategy.MaximumPitTransitSeconds = -1.0;
    Require(!ValidPitReviewSettings(malformedStrategy) &&
            ValidPitVisitReviewSettings(visitSettings),
            "malformed existing pit recommendation must not poison physical visit settings");

    auto malformedVisit = visitSettings;
    malformedVisit.MaximumFrameGapSeconds = -1.0;
    Require(!ValidPitVisitReviewSettings(malformedVisit) &&
            ValidPitReviewSettings(strategySettings),
            "malformed observed pit visit must not corrupt strategy review policy");

    PitVisitMonitor monitor;
    Require(monitor.Configure(visitSettings),
            "pit monitor accepts only its own explicitly typed observation config");
    Require(!monitor.Configure(malformedVisit),
            "unqualified observation settings cannot silently become valid");

    std::cout << "Both independent pit review APIs coexist safely (portable C++20).\n";
    return EXIT_SUCCESS;
}
