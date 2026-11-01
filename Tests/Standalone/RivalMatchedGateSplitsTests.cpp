#include "Racing/RivalMatchedGateSplits.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool good, const char* reason)
{
    if (!good)
    {
        std::cerr << "RivalMatchedGateSplits FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b) { return std::abs(a - b) <= 1e-9; }
}

int main()
{
    using latebrake::RivalMatchedGateSplits;
    RivalMatchedGateSplits split;
    Check(!split.RecordPlayer(1, 4.0) && !split.LatestMatched().Valid,
          "not configured");
    split.Configure(4, 3);
    Check(split.RecordPlayer(1, 10.0) && !split.LatestMatched().Valid,
          "single crossing cannot be a matched split");
    Check(!split.RecordPlayer(1, 11.0) &&
          !split.RecordRival(0, 3.0) &&
          !split.RecordRival(13, 15.0),
          "duplicate or out-of-bounds crossing rejected");
    Check(split.RecordRival(1, 11.25), "matching rival admitted");
    auto first = split.LatestMatched();
    Check(first.Valid && first.GateOrdinal == 1 &&
          first.LapNumber == 1 && first.GateNumber == 1 &&
          Near(first.PlayerSeconds, 10.0) &&
          Near(first.RivalSeconds, 11.25) &&
          Near(first.RivalMinusPlayerSeconds, 1.25),
          "positive split means rival arrived later");
    Check(split.RecordRival(2, 20.0) &&
          !split.RecordPlayer(2, 0.0) &&
          !split.RecordPlayer(2, std::numeric_limits<double>::quiet_NaN()) &&
          split.LatestMatched().GateOrdinal == 1,
          "no phantom match on invalid clock");
    Check(split.RecordPlayer(2, 18.0) &&
          Near(split.LatestMatched().RivalMinusPlayerSeconds, 2.0),
          "rival-first matched gate still compares");
    Check(split.RecordPlayer(3, 24.0) &&
          split.RecordRival(3, 23.0) &&
          Near(split.LatestMatched().RivalMinusPlayerSeconds, -1.0),
          "negative split means rival earlier");
    Check(split.RecordPlayer(4, 30.0) && split.RecordRival(4, 30.0) &&
          Near(split.LatestMatched().RivalMinusPlayerSeconds, 0.0),
          "matching timestamps are not a physical race tie");
    Check(split.RecordPlayer(5, 35.0) && split.RecordRival(5, 36.0) &&
          split.LatestMatched().LapNumber == 2 &&
          split.LatestMatched().GateNumber == 1,
          "lap rollover never matches prior lap gate");
    Check(!split.RecordPlayer(6, 34.0) && !split.RecordRival(5, 37.0) &&
          split.LatestMatched().GateOrdinal == 5,
          "reversed time and duplicates do not alter latest");
    split.Reset();
    Check(!split.LatestMatched().Valid &&
          split.RecordRival(1, 8.0) && split.RecordPlayer(1, 9.0) &&
          Near(split.LatestMatched().RivalMinusPlayerSeconds, -1.0),
          "restart clears stale attempt history");
    split.Configure(0, 3);
    Check(!split.RecordPlayer(1, 1.0), "invalid gate count disabled");
    split.Configure(4, 101);
    Check(!split.RecordRival(1, 1.0), "oversized lap count disabled");
    split.Configure(256, 100);
    Check(split.RecordPlayer(25600, 500.0) &&
          split.RecordRival(25600, 501.0) &&
          split.LatestMatched().LapNumber == 100 &&
          split.LatestMatched().GateNumber == 256,
          "largest supported ordinal remains safe");
    split.Configure(4, 3);
    Check(!split.RecordPlayer(1, std::numeric_limits<double>::infinity()) &&
          !split.RecordRival(1, std::numeric_limits<double>::quiet_NaN()) &&
          !split.RecordPlayer(1, RivalMatchedGateSplits::MaxElapsedSeconds + 1),
          "invalid or unreasonable timestamps rejected");
    Check(split.RecordPlayer(1, 1.0) &&
          split.RecordPlayer(2, 2.0) &&
          split.RecordRival(2, 2.5) &&
          split.LatestMatched().GateOrdinal == 2,
          "missing opposite gate never invents a match");
    split.Configure(40, 1);
    Check(split.RecordPlayer(1, 1.0), "first ring sample");
    for (int i = 2; i <= 34; ++i)
    {
        Check(split.RecordPlayer(i, static_cast<double>(i)),
              "valid later player gates retained");
    }
    Check(split.RecordRival(1, 1.5) && !split.LatestMatched().Valid,
          "evicted gate cannot match reused history slot");
    for (int i = 2; i <= 34; ++i)
    {
        Check(split.RecordRival(i, static_cast<double>(i) + 0.5),
              "rival can catch up across multiple gates");
    }
    Check(split.LatestMatched().Valid &&
          split.LatestMatched().GateOrdinal == 34,
          "ring index checked against full gate ordinal");
    for (int i = 0; i < 10000; ++i)
    {
        split.Configure(4, 3);
        Check(split.RecordRival(1, 10.0) &&
              split.RecordPlayer(1, 10.25) &&
              Near(split.LatestMatched().RivalMinusPlayerSeconds, -0.25),
              "many fresh sessions do not leak split history");
    }
    std::cout << "Rival matched-gate split portable policy passed.\n";
    return EXIT_SUCCESS;
}
