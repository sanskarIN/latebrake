#include "Racing/DriftComboMeter.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool yes, const char* msg)
{
    if (!yes)
    {
        std::cerr << "DriftComboMeter FAIL: " << msg << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-8;
}
latebrake::DrivingSample Sample(double dt, double speed, double sideways)
{
    return {dt, speed, sideways, false};
}
} // namespace

int main()
{
    using namespace latebrake;
    DriftComboMeter m;
    Check(!m.HasActiveCombo() && m.GetComboCount() == 0 &&
          Near(m.GetTotalPoints(), 0.0), "new race has no invented drift");
    for (int i = 0; i < 2; ++i)
    {
        Check(m.Observe(Sample(0.2, 1600.0, 1000.0)),
              "clean sideways speed sample accepted");
    }
    Check(!m.HasActiveCombo() && Near(m.GetTotalPoints(), 0.0),
          "two isolated 0.2s samples do not activate a combo");
    Check(m.Observe(Sample(0.2, 1600.0, 1000.0)),
          "third qualifying sample admitted");
    Check(m.HasActiveCombo() && m.GetComboCount() == 1 &&
          m.GetCurrentComboPoints() > 0.0 &&
          Near(m.GetCurrentComboSeconds(), 0.6),
          "real sustained 0.6s drift qualifies once");
    Check(Near(m.GetCurrentComboMultiplier(), 1.0),
          "newly qualified arcade combo has x1 multiplier");
    const double beginning = m.GetCurrentComboPoints();
    Check(m.Observe(Sample(0.2, 1600.0, 1000.0)) &&
          m.GetCurrentComboPoints() > beginning,
          "continued actual slip accrues finite extra arcade points");
    Check(m.Observe(Sample(0.2, 1600.0, 0.0)) &&
          m.HasActiveCombo(), "brief recovery gap preserves same combo");
    Check(m.Observe(Sample(0.2, 1600.0, 1000.0)) &&
          m.GetComboCount() == 1, "reacquiring within gap does not double count");
    Check(m.Observe(Sample(0.2, 1600.0, 0.0)) &&
          m.Observe(Sample(0.2, 1600.0, 0.0)) &&
          !m.HasActiveCombo(), "sustained return to grip banks combo");
    const double earned = m.GetTotalPoints();
    Check(earned > 0.0 && Near(m.GetBestComboPoints(), earned) &&
          m.GetTotalDriftSeconds() >= 0.8,
          "banked score and genuine drift duration are preserved");
    m.Suspend();
    Check(Near(m.GetTotalPoints(), earned),
          "repeated suspension never double-banks a finished combo");

    Check(m.Observe(Sample(0.2, 1600.0, 1000.0)) &&
          !m.HasActiveCombo(), "new combo must requalify after pause");
    m.Suspend();
    Check(!m.HasActiveCombo() && m.GetComboCount() == 1 &&
          Near(m.GetTotalPoints(), earned),
          "prequalification samples never earn points across teleport");
    for (int i = 0; i < 4; ++i)
        Check(m.Observe(Sample(0.2, 1700.0, 950.0)), "new measured drift");
    Check(m.HasActiveCombo() && m.GetComboCount() == 2 &&
          m.GetTotalPoints() > earned,
          "second separate combo adds only real score");
    const double latest = m.GetTotalPoints();
    Check(!m.Observe(Sample(std::numeric_limits<double>::quiet_NaN(),
                            1000.0, 500.0)) && !m.HasActiveCombo() &&
          Near(m.GetTotalPoints(), latest),
          "invalid frames suspend and bank only existing score");
    Check(!m.Observe(Sample(0.2, 1000.0, 1500.0)),
          "unphysical lateral velocity is rejected");
    Check(!m.Observe(Sample(0.5, 1600.0, 1000.0)),
          "hitch sample cannot fabricate time or points");
    Check(!m.Observe(Sample(0.2, std::numeric_limits<double>::infinity(),
                            1000.0)), "infinite chassis speed rejected");
    Check(!m.Observe(Sample(0.2, 10001.0, 1000.0)),
          "excessive chassis speed rejected");
    Check(m.Observe(Sample(0.2, 800.0, 500.0)) &&
          !m.HasActiveCombo(), "low-speed lateral movement is not a drift");
    Check(m.Observe(Sample(0.2, 1600.0, 350.0)) &&
          !m.HasActiveCombo(), "small lateral ratio does not create points");
    Check(m.GetTotalPoints() > 0.0,
          "inactive sample does not erase properly banked previous score");
    DriftComboMeter lengthy;
    for (int n = 0; n < 17; ++n)
    {
        Check(lengthy.Observe(Sample(0.2, 1700.0, 950.0)),
              "sustained measured drift sample accepted");
    }
    Check(lengthy.HasActiveCombo() &&
          Near(lengthy.GetCurrentComboMultiplier(), 1.25),
          "measured three-second combo earns first arcade multiplier");
    lengthy.Suspend();
    Check(!lengthy.HasActiveCombo() &&
          Near(lengthy.GetCurrentComboMultiplier(), 1.0) &&
          lengthy.GetTotalPoints() > 0.0,
          "banked combo cannot retain old multiplier");

    DriftComboMeter fragmented;
    Check(fragmented.Observe(Sample(0.2, 1600.0, 1000.0)),
          "first isolated slip accepted");
    Check(fragmented.Observe(Sample(0.1, 1600.0, 0.0)),
          "ordinary grip interrupts before qualifying");
    Check(fragmented.Observe(Sample(0.2, 1600.0, 1000.0)) &&
          fragmented.Observe(Sample(0.2, 1600.0, 1000.0)),
          "new qualifying sequence begins after early grip correction");
    Check(!fragmented.HasActiveCombo() &&
          Near(fragmented.GetTotalPoints(), 0.0) &&
          fragmented.GetComboCount() == 0,
          "separated prequalification slips never fabricate a combo");
    Check(fragmented.Observe(Sample(0.2, 1600.0, 1000.0)) &&
          fragmented.HasActiveCombo(),
          "third uninterrupted qualifying sample starts combo");

    m.Reset();
    Check(!m.HasActiveCombo() && m.GetComboCount() == 0 &&
          Near(m.GetTotalPoints(), 0.0) &&
          Near(m.GetTotalDriftSeconds(), 0.0) &&
          Near(m.GetBestComboPoints(), 0.0),
          "restart clears entire run-local arcade score");
    std::cout << "Drift combo meter passed.\n";
    return EXIT_SUCCESS;
}
