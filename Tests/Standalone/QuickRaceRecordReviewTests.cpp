#include "Racing/QuickRaceRecordReview.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

static void Need(bool ok, const char* label)
{
    if (!ok) { std::cerr << "QuickRaceRecordReview FAIL: " << label << '\n'; std::exit(1); }
}
int main()
{
    using namespace latebrake;
    using S = QuickRaceRecordState;
    auto review = [](EventKind e, bool career, bool eligible, bool failed,
                     double elapsed, double before, double after)
    {
        return ReviewQuickRaceRecord(e,career,eligible,failed,elapsed,before,after);
    };
    for (auto e : {EventKind::Circuit,EventKind::TimeAttack})
    {
        Need(review(e,false,true,false,70,0,70).State==S::FirstRecord,"first stored PB");
        auto best=review(e,false,true,false,68,70,68);
        Need(best.State==S::Improved&&std::abs(best.DeltaSeconds-2)<1e-6,
             "stored PB improvement from pre-run");
        Need(review(e,false,true,false,70,70,70).State==S::Matched,"tie");
        auto slower=review(e,false,true,false,74.5,70,70);
        Need(slower.State==S::Slower&&std::abs(slower.DeltaSeconds-4.5)<1e-6,
             "slower race includes time penalties");
        Need(review(e,false,true,true,65,70,65).State==S::SaveFailed,
             "failed save never claims a PB");
        Need(review(e,true,true,false,65,70,65).State==S::Unavailable,
             "career is independent");
        Need(review(e,false,false,false,65,70,65).State==S::Unavailable,
             "ineligible finish hidden");
        Need(review(e,false,true,false,65,70,90).State==S::Unavailable,
             "saved PB never worsens");
        Need(review(e,false,true,false,65,70,0).State==S::Unavailable,
             "no persisted PB");
        Need(review(e,false,true,false,65,70,35).State==S::Unavailable,
             "unrelated saved record");
        Need(review(e,false,true,false,65,0,35).State==S::Unavailable,
             "unrelated first save");
        Need(review(e,false,true,false,69.999,70,70).State==S::Matched,
             "float ties are not new PBs");
    }
    constexpr double nan=std::numeric_limits<double>::quiet_NaN();
    for (double time : {0., -5., nan, std::numeric_limits<double>::infinity(),
                        604800.1})
        Need(review(EventKind::TimeAttack,false,true,false,time,70,70).State==
             S::Unavailable,"invalid time");
    Need(review(EventKind::TimeAttack,false,true,false,604800,0,604800).State==
         S::FirstRecord,"seven day bound");
    Need(review(EventKind::TimeAttack,false,true,false,65,nan,65).State==
         S::FirstRecord,"invalid prior sanitized to no record");
    Need(review(EventKind::TimeAttack,false,true,false,65,70,
                std::numeric_limits<double>::infinity()).State==
         S::Unavailable,"infinite persisted record");
    Need(review(static_cast<EventKind>(99),false,true,false,65,0,65).State==
         S::Unavailable,"unknown event");
    std::cout << "Quick Race record result review tests passed.\n";
}
