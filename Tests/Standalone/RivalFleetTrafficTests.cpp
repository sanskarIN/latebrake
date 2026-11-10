#include "AI/RivalFleetTraffic.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "RivalFleetTraffic FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    RivalFleetTrafficSample traffic;
    traffic.SessionQualified = true;
    traffic.Self = {true, 1, 1001, {{0.0, -1800.0}, 0.0},
                    1000.0, 0.0};
    traffic.Blockers[0] = {true, 0, 1000,
        {{1200.0, -1800.0}, 0.0}, 200.0, 0.0};
    traffic.BlockerCount = 1;
    const PilotCommand moving{1.0, 0.35, false, false};
    const auto following = ApplyRivalFleetTraffic(moving, traffic);
    Require(following.Qualified && !following.EmergencyBrake &&
        following.Command.Throttle < 1.0 &&
        following.Command.Throttle > 0.0 &&
        following.Command.Steering == moving.Steering &&
        following.LimitingSlot == 0,
        "measured same-lane approach bounds throttle but never steering");
    traffic.Blockers[0].Pose.Position.X = 600.0;
    traffic.Blockers[0].VelocityX = 0.0;
    const auto approaching = ApplyRivalFleetTraffic(moving, traffic);
    Require(approaching.Qualified && approaching.EmergencyBrake &&
        approaching.Command.Brake && approaching.Command.Throttle == 0.0 &&
        approaching.Command.Steering == moving.Steering,
        "closing physical car inside swept contact interval brakes");
    traffic.Blockers[0].Pose.Position.X = -800.0;
    traffic.Blockers[0].VelocityX = 3500.0;
    const auto behind = ApplyRivalFleetTraffic(moving, traffic);
    Require(behind.Qualified && !behind.EmergencyBrake &&
        !behind.Command.Brake && behind.Command.Throttle == 1.0,
        "rear approaching car cannot make leading rival brake into rear contact");
    traffic.Blockers[0].Pose.Position = {600.0, -1200.0};
    traffic.Blockers[0].VelocityX = 0.0;
    const auto adjacent = ApplyRivalFleetTraffic(moving, traffic);
    Require(adjacent.Qualified && !adjacent.EmergencyBrake &&
        adjacent.Command.Throttle == 1.0,
        "adjacent lane separated by physical car widths is not blocked");

    traffic.Blockers[0] = {true, 0, 1000,
        {{0.0, -1000.0}, 90.0}, 0.0, -2500.0};
    const auto crossing = ApplyRivalFleetTraffic(moving, traffic);
    Require(crossing.Qualified && crossing.EmergencyBrake &&
        crossing.LimitingSlot == 0,
        "moving cross traffic is caught between discrete frames");

    traffic.Blockers[0].Pose.Position = {600.0, -1800.0};
    traffic.Blockers[0].VelocityX = 0.0;
    traffic.SessionQualified = false;
    Require(!ApplyRivalFleetTraffic(moving, traffic).Qualified &&
        ApplyRivalFleetTraffic(moving, traffic).Command.Brake,
        "paused or not ready session forbids any propulsion");
    traffic.SessionQualified = true;
    traffic.BlockerCount = 0;
    Require(!ApplyRivalFleetTraffic(moving, traffic).Qualified,
        "missing human physical opponent is NOT a safe empty grid");
    traffic.BlockerCount = 6;
    Require(!ApplyRivalFleetTraffic(moving, traffic).Qualified,
        "unbounded number of blockers fails closed");
    traffic.BlockerCount = 1;
    traffic.Blockers[0].Verified = false;
    Require(!ApplyRivalFleetTraffic(moving, traffic).Qualified,
        "unverified Chaos blocker cannot authorize acceleration");
    traffic.Blockers[0].Verified = true;
    traffic.Blockers[0].ActorToken = traffic.Self.ActorToken;
    Require(!ApplyRivalFleetTraffic(moving, traffic).Qualified,
        "self in blocker list is malformed");
    traffic.Blockers[0].ActorToken = 1000;
    traffic.Blockers[0].VelocityX =
        std::numeric_limits<double>::quiet_NaN();
    Require(!ApplyRivalFleetTraffic(moving, traffic).Qualified,
        "NaN world velocity fails closed");
    traffic.Blockers[0].VelocityX = 0.0;
    traffic.Self.Pose.HeadingDegrees =
        std::numeric_limits<double>::infinity();
    Require(!ApplyRivalFleetTraffic(moving, traffic).Qualified,
        "nonfinite rival heading fails closed");
    traffic.Self.Pose.HeadingDegrees = 0.0;
    Require(!ApplyRivalFleetTraffic({-2.0, 0.0, false, false},
        traffic).Qualified, "malformed throttle fails closed");

    traffic.Blockers[0] = {true, 0, 1000,
        {{3500.0, -1800.0}, 0.0}, 0.0, 0.0};
    const PilotCommand corner{0.0, -0.7, true, false};
    const auto cornerBrake = ApplyRivalFleetTraffic(corner, traffic);
    Require(cornerBrake.Qualified && cornerBrake.Command.Brake &&
        cornerBrake.Command.Steering == -0.7,
        "existing corner brake authority stays untouched");
    const PilotCommand arrival{0.0, 0.0, false, true};
    Require(ApplyRivalFleetTraffic(arrival, traffic).Command.Arrived,
        "genuine route arrival is not altered");

    // Five distinct, physically verified inputs and one late threat.
    traffic.BlockerCount = 5;
    for (int i = 0; i < 5; ++i)
    {
        traffic.Blockers[static_cast<std::size_t>(i)] =
            {true, i == 0 ? 0 : i + 1,
             static_cast<std::uint64_t>(i + 2000),
             {{5000.0, 5000.0 + i * 500.0}, 0.0}, 0.0, 0.0};
    }
    Require(ApplyRivalFleetTraffic(moving, traffic).Qualified &&
        !ApplyRivalFleetTraffic(moving, traffic).EmergencyBrake,
        "five well separated real cars do not trigger phantom collision");
    traffic.Blockers[4].Pose.Position = {600.0, -1800.0};
    auto fifthThreat = ApplyRivalFleetTraffic(moving, traffic);
    Require(fifthThreat.EmergencyBrake && fifthThreat.LimitingSlot == 5,
        "fifth physical blocker has full veto authority");
    traffic.Blockers[4].ActorToken = traffic.Blockers[3].ActorToken;
    Require(!ApplyRivalFleetTraffic(moving, traffic).Qualified,
        "duplicate actor token cannot be used for safe driving");
    traffic.Blockers[4].ActorToken = 2004;
    traffic.Blockers[4].Slot = traffic.Blockers[3].Slot;
    Require(!ApplyRivalFleetTraffic(moving, traffic).Qualified,
        "duplicate grid slot cannot be counted as new physical opponent");

    // Symmetric measurable pose rotation: no fixed-axis approximation.
    traffic.BlockerCount = 1;
    traffic.Self = {true, 1, 1001, {{2800.0, -500.0}, 90.0}, 0.0, 950.0};
    traffic.Blockers[0] = {true, 0, 1000,
        {{2800.0, 80.0}, 90.0}, 0.0, 0.0};
    Require(ApplyRivalFleetTraffic(moving, traffic).EmergencyBrake,
        "north-facing traffic applies the same swept collision policy");

    std::cout << "Real-observation five-blocker traffic envelope passed.\n";
    return EXIT_SUCCESS;
}
