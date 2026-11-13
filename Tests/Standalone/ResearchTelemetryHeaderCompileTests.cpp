// Compile the coherent first-generation research telemetry domain together.
// The stacked strategy generation has incompatible coaching names and has
// been isolated from this mainline port instead of hiding ODR conflicts.
#include "Telemetry/TelemetryBrakeTrace.h"
#include "Telemetry/TelemetryBraking.h"
#include "Telemetry/TelemetryCornerSequence.h"
#include "Telemetry/TelemetryCornering.h"
#include "Telemetry/TelemetryDistancePace.h"
#include "Telemetry/TelemetryDriverCoach.h"
#include "Telemetry/TelemetryFilters.h"
#include "Telemetry/TelemetryFleetGapReview.h"
#include "Telemetry/TelemetryFuelStint.h"
#include "Telemetry/TelemetryGrip.h"
#include "Telemetry/TelemetryIncidentJournal.h"
#include "Telemetry/TelemetryKinematics.h"
#include "Telemetry/TelemetryLapComparison.h"
#include "Telemetry/TelemetryLapJournal.h"
#include "Telemetry/TelemetryMath.h"
#include "Telemetry/TelemetryOpponents.h"
#include "Telemetry/TelemetryPitReview.h"
#include "Telemetry/TelemetryRaceLineProfile.h"
#include "Telemetry/TelemetryReplayBookmarks.h"
#include "Telemetry/TelemetryReplaySampler.h"
#include "Telemetry/TelemetrySectorMatrix.h"
#include "Telemetry/TelemetrySessionIntegrity.h"
#include "Telemetry/TelemetrySessionRecorder.h"
#include "Telemetry/TelemetrySignalSeries.h"
#include "Telemetry/TelemetrySuspensionReview.h"
#include "Telemetry/TelemetryTireStint.h"
#include "Telemetry/TelemetryTrack.h"
#include "Telemetry/TelemetryTransmissionReview.h"
#include "Telemetry/TelemetryTypes.h"
#include "Telemetry/TelemetryValidation.h"
#include "Telemetry/TelemetryVehicleBalance.h"
#include "Telemetry/TelemetryWeatherTrend.h"
#include <iostream>
int main()
{
    std::cout << "Telemetry: all 32 interoperable headers compiled.\n";
    return 0;
}
