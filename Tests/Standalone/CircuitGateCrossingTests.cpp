#include "Racing/CircuitGateCrossing.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "Gate crossing FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    constexpr double Z = 150.0;

    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const CircuitPoint center = CircuitGate(gate);
        const double dx = gate == 0 ? 1.0 : gate == 2 ? -1.0 : 0.0;
        const double dy = gate == 1 ? 1.0 : gate == 3 ? -1.0 : 0.0;
        const CircuitGateSample before{{center.X - dx * 180.0,
                                        center.Y - dy * 180.0}, Z};
        const CircuitGateSample after{{center.X + dx * 75.0,
                                       center.Y + dy * 75.0}, Z};
        Require(CrossesCircuitGatePlane(gate, before, after,
                                       dx * 250.0, dy * 250.0),
                "correct forward plane crossing passes in all four sectors");
        Require(!CrossesCircuitGatePlane(gate, before, before,
                                        dx * 250.0, dy * 250.0),
                "touch on approach side cannot grant checkpoint");
        Require(!CrossesCircuitGatePlane(gate, after, before,
                                        dx * 250.0, dy * 250.0),
                "reverse positional crossing rejected even with forward speed");
        Require(!CrossesCircuitGatePlane(gate, after, before,
                                        -dx * 250.0, -dy * 250.0),
                "wrong-way velocity cannot claim gate");
        Require(!CrossesCircuitGatePlane(gate, before, after, 0.0, 0.0),
                "stationary gate contact rejected");
        Require(!CrossesCircuitGatePlane(gate, before, after,
                                        dx * 149.0, dy * 149.0),
                "below threshold movement rejected");
        Require(!CrossesCircuitGatePlane(gate, after, after,
                                        dx * 250.0, dy * 250.0),
                "remaining behind gate after crossing cannot double count");

        const CircuitGateSample ahead{{center.X - dx * 350.0,
                                       center.Y - dy * 350.0}, Z};
        Require(!CrossesCircuitGatePlane(gate, ahead, before,
                                        dx * 200.0, dy * 200.0),
                "entry into sphere well ahead of line not counted");
        const CircuitGateSample edge{{center.X - dx * 600.0,
                                      center.Y - dy * 600.0}, Z};
        Require(CrossesCircuitGatePlane(gate, edge, after,
                                       dx * 250.0, dy * 250.0),
                "near-approach crossing allowed");
        // A low-FPS sample can begin just outside the old 600cm trigger
        // sphere yet sweep through the valid gate plane. That is genuine
        // forward driving, not a skipped checkpoint or a teleport.
        const CircuitGateSample oldSphereMiss{{center.X - dx * 601.0,
                                               center.Y - dy * 601.0}, Z};
        Require(CrossesCircuitGatePlane(gate, oldSphereMiss, after,
                                       dx * 250.0, dy * 250.0),
                "forward sweep across old trigger boundary now counts");

        const CircuitGateSample lagBefore{{center.X - dx * 900.0,
                                           center.Y - dy * 900.0}, Z};
        const CircuitGateSample lagAfter{{center.X + dx * 200.0,
                                          center.Y + dy * 200.0}, Z};
        Require(CrossesCircuitGatePlane(gate, lagBefore, lagAfter,
                                       dx * 900.0, dy * 900.0),
                "1100cm same-direction frame hitch counts on all four gates");
        Require(!CrossesCircuitGatePlane(gate, lagAfter, lagBefore,
                                        dx * 900.0, dy * 900.0),
                "large reverse sweep cannot use positive velocity to claim gate");
        Require(!CrossesCircuitGatePlane(gate, lagBefore, lagAfter,
                                        dx * -900.0, dy * -900.0),
                "large sweep still rejects a wrong-way velocity");
        const CircuitGateSample exactlyCapBefore{{center.X - dx * 1100.0,
                                                  center.Y - dy * 1100.0}, Z};
        const CircuitGateSample exactlyCapAfter{{center.X + dx * 100.0,
                                                 center.Y + dy * 100.0}, Z};
        Require(CrossesCircuitGatePlane(gate, exactlyCapBefore,
                                       exactlyCapAfter, dx * 250.0, dy * 250.0),
                "exact 1200cm displacement bound is permitted");
        const CircuitGateSample beyondCap{{center.X - dx * 1101.0,
                                           center.Y - dy * 1101.0}, Z};
        Require(!CrossesCircuitGatePlane(gate, beyondCap,
                                         exactlyCapAfter,
                                         dx * 250.0, dy * 250.0),
                "over-1200cm sweep still rejects movement discontinuities");

        // Lateral axis is orthogonal to the direction of travel.
        const double lx = dy;
        const double ly = dx;
        const CircuitGateSample laneBefore{{before.XY.X + lx * 450.0,
                                            before.XY.Y + ly * 450.0}, Z};
        const CircuitGateSample laneAfter{{after.XY.X + lx * 450.0,
                                           after.XY.Y + ly * 450.0}, Z};
        Require(CrossesCircuitGatePlane(gate, laneBefore, laneAfter,
                                       dx * 200.0, dy * 200.0),
                "valid crossing may be off center");
        const CircuitGateSample offBefore{{before.XY.X + lx * 510.0,
                                           before.XY.Y + ly * 510.0}, Z};
        const CircuitGateSample offAfter{{after.XY.X + lx * 510.0,
                                          after.XY.Y + ly * 510.0}, Z};
        Require(!CrossesCircuitGatePlane(gate, offBefore, offAfter,
                                        dx * 200.0, dy * 200.0),
                "outside gate lateral width rejected");
    }

    constexpr CircuitGateSample southBefore{{EastX - 150.0, SouthY}, Z};
    constexpr CircuitGateSample southAfter{{EastX + 100.0, SouthY}, Z};
    Require(!CrossesCircuitGatePlane(-1, southBefore, southAfter, 200, 0) &&
            !CrossesCircuitGatePlane(4, southBefore, southAfter, 200, 0),
            "invalid gate index never passes");
    Require(!CrossesCircuitGatePlane(0,
            {{std::numeric_limits<double>::quiet_NaN(), SouthY}, Z},
            southAfter, 200, 0),
            "nonfinite previous coordinate fails closed");
    Require(!CrossesCircuitGatePlane(0, southBefore,
            {{std::numeric_limits<double>::infinity(), SouthY}, Z}, 200, 0),
            "nonfinite current coordinate fails closed");
    Require(!CrossesCircuitGatePlane(0, southBefore, southAfter,
            std::numeric_limits<double>::infinity(), 0),
            "nonfinite velocity fails closed");
    Require(!CrossesCircuitGatePlane(0,
            {{EastX - 3000.0, SouthY}, Z},
            {{EastX + 3000.0, SouthY}, Z}, 400, 0),
            "teleport over full checkpoint rejected");
    Require(!CrossesCircuitGatePlane(0,
            {{EastX - 10.0, SouthY - 3000.0}, Z},
            {{EastX + 10.0, SouthY + 3000.0}, Z}, 200, 0),
            "large lateral teleport crossing rejected");
    Require(!CrossesCircuitGatePlane(0,
            {{EastX - 150.0, SouthY}, GateCenterHeightCm + 450.0},
            {{EastX + 100.0, SouthY}, GateCenterHeightCm + 450.0}, 200, 0),
            "airborne checkpoint skip above height window rejected");
    Require(CrossesCircuitGatePlane(0,
            {{EastX - 100.0, SouthY}, GateCenterHeightCm + 350.0},
            {{EastX + 100.0, SouthY}, GateCenterHeightCm + 350.0},
            220, 0), "legitimate elevated but low vehicle still accepted");
    Require(!CrossesCircuitGatePlane(0, southBefore,
            {{EastX + 100.0, SouthY}, std::numeric_limits<double>::quiet_NaN()},
            200, 0), "nonfinite height rejected");

    // Crossing at the checkpoint plane can be interpolated for a diagonal
    // movement. Merely being inside the old sphere is insufficient.
    Require(CrossesCircuitGatePlane(0,
            {{EastX - 80.0, SouthY - 650.0}, Z},
            {{EastX + 80.0, SouthY + 650.0}, Z}, 200, 0) == false,
            "diagonal jump beyond per-frame displacement rejected");
    Require(CrossesCircuitGatePlane(0,
            {{EastX - 80.0, SouthY - 300.0}, Z},
            {{EastX + 80.0, SouthY + 300.0}, Z}, 200, 0),
            "valid diagonal crossing through center permitted");

    std::cout << "Four-gate plane crossing tests passed.\n";
    return EXIT_SUCCESS;
}
