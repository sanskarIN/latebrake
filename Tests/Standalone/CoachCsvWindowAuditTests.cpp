#include "../../Tools/CoachTelemetry/CoachCsvWindowAudit.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>

namespace
{
using namespace latebrake;
using namespace latebrake::coachcsv;
void Require(bool condition,const char* reason)
{
    if(!condition)
    {
        std::cerr << "CoachCsvWindowAudit: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
std::string FixtureText(int count,int every=0,bool includeEnd=true,
    int speed=1800)
{
    std::ostringstream out;
    out << Header << '\n' << "context,0,1,2,race\n";
    for(int i=0;i<count;++i)
    {
        if(every>0 && i>0 && i%every==0)
            out << "suspend,,,,\n";
        out << "sample,0.100," << speed+(i%9)*20 << ","
            << (i%13<2 ? 700 : 90) << "," << (i%8<2?1:0) << '\n';
    }
    if(includeEnd)out << "end,,,,\n";
    return out.str();
}
struct Fixture
{
    CsvAudit Source{};
    DrivingSessionStats Stats{};
    CoachCsvWindowAudit Read{};
    explicit Fixture(const std::string& text)
    {
        std::istringstream in(text);
        Source=AnalyzeCsv(in,Stats);
        Require(Source.Valid,"fixture satisfies same strict CSV parser");
        Read=BuildCoachCsvWindowAudit(Source,Stats);
    }
};
void ExactCompleteAndUnfinishedWindowTiming()
{
    const Fixture input(FixtureText(123));
    const auto& report=input.Read;
    Require(ValidCoachCsvWindowAudit(report),
        "complete validated session produces trustworthy bounded retained windows");
    Require(report.SourceSamples==123 &&
        report.RetainedSamples==123 && report.RetainedCount==3 &&
        report.CompleteCount==2 && report.PartialCount==1,
        "two five-second intervals and a short tail are not merged");
    Require(report.IncludesUnfinishedWindows &&
        !report.SomeOldWindowsNotRetained &&
        !report.ContainsContinuityBreaks &&
        report.ObservedEpochChanges==0,
        "tail completeness and source coverage are independently disclosed");
    Require(std::abs(report.RetainedSeconds-12.3)<1.e-8 &&
        std::abs(report.RetainedCoverageFraction-1.0)<1.e-8 &&
        report.Rows[0].AcceptedSamples==23 &&
        report.Rows[1].AcceptedSamples==50 &&
        report.Rows[2].AcceptedSamples==50,
        "reverse ring indexing preserves exact observed durations and samples");
    Require(!report.SourceAuthenticityVerified &&
        !report.AuthenticLapTimingVerified && !report.DrivesGamePhysics &&
        !report.WritesSaveData,"user-editable source cannot prove actual game");
}
void SourceContinuityBreaksNeverCreateFakeWindows()
{
    const Fixture input(FixtureText(150,45));
    const auto& report=input.Read;
    Require(ValidCoachCsvWindowAudit(report) &&
        report.RetainedCount>=4 && report.PartialCount>=3 &&
        report.ContainsContinuityBreaks &&
        report.ObservedEpochChanges==3,
        "three explicit suspend receipts keep independent observed clock epochs");
    for(unsigned i=1;i<report.RetainedCount;++i)
    {
        const auto& newer=report.Rows[i-1];
        const auto& older=report.Rows[i];
        Require(newer.Sequence>older.Sequence &&
            newer.StartedSeconds>=older.EndedSeconds-1.e-7 &&
            newer.ContinuityEpoch>=older.ContinuityEpoch,
            "windows never interpolate across restart/pause/source discontinuities");
    }
    Require(report.SourceSamples==150 && report.RetainedSamples==150,
        "suspension markers are not counted as chassis motion samples");
}
void RingEvictionRefusesWholeSessionCoverageClaim()
{
    const Fixture input(FixtureText(2200));
    const auto& r=input.Read;
    Require(ValidCoachCsvWindowAudit(r),"long source retains valid partial ring");
    Require(r.SourceSamples==2200 &&
        r.RetainedCount==DrivingCoachWindowCapacity &&
        r.SomeOldWindowsNotRetained &&
        r.RetainedSamples<r.SourceSamples &&
        r.RetainedCoverageFraction<0.5 &&
        !r.AuthenticLapTimingVerified,
        "older windows evicted instead of inventing complete session analysis");
}
void VeryShortAndUnfinishedSourceIsStillHonestObservation()
{
    const Fixture input(FixtureText(4,0,false));
    const auto& r=input.Read;
    Require(ValidCoachCsvWindowAudit(r) &&
        r.RetainedCount==1 && r.CompleteCount==0 &&
        r.PartialCount==1 && r.SourceSamples==4 &&
        !r.SourceAuthenticityVerified,
        "short trace may be described without conferring official timing");
}
void CannotValidateFabricatedRowsOrImpossibleFlags()
{
    const Fixture f(FixtureText(150));
    auto bad=f.Read;
    Require(ValidCoachCsvWindowAudit(bad),"valid base report");
    bad.Rows[0].ObservedSeconds=-1;
    Require(!ValidCoachCsvWindowAudit(bad),"negative observed time rejected");
    bad=f.Read;
    bad.Rows[0].MeanSpeedKmh=std::numeric_limits<double>::quiet_NaN();
    Require(!ValidCoachCsvWindowAudit(bad),"NaN sampled speed rejected");
    bad=f.Read;
    bad.Rows[0].EstimatedDistanceKm=1.e8;
    Require(!ValidCoachCsvWindowAudit(bad),"made-up sample distance rejected");
    bad=f.Read;
    bad.Rows[0].BrakingFraction=1.001;
    Require(!ValidCoachCsvWindowAudit(bad),"impossible brake fraction rejected");
    bad=f.Read;
    bad.Rows[1].Sequence=bad.Rows[0].Sequence;
    Require(!ValidCoachCsvWindowAudit(bad),"reordered source window rejected");
    bad=f.Read;
    bad.Rows[1].EndedSeconds=bad.Rows[0].EndedSeconds+1.0;
    Require(!ValidCoachCsvWindowAudit(bad),"overlapping observed windows rejected");
    bad=f.Read;
    ++bad.RetainedSamples;
    Require(!ValidCoachCsvWindowAudit(bad),"misstated sample count rejected");
    bad=f.Read;
    bad.SomeOldWindowsNotRetained=true;
    Require(!ValidCoachCsvWindowAudit(bad),"fabricated source coverage loss rejected");
    bad=f.Read;
    bad.RetainedCoverageFraction=0.0;
    Require(!ValidCoachCsvWindowAudit(bad),"falsified window coverage rejected");
    bad=f.Read;
    bad.AuthenticLapTimingVerified=true;
    Require(!ValidCoachCsvWindowAudit(bad),"false official race proof rejected");
    bad=f.Read;
    bad.Rows[0].AcceptedSamples=std::numeric_limits<std::uint64_t>::max();
    Require(!ValidCoachCsvWindowAudit(bad),"overflowing sample count rejected");
}
void IndependentSampleSumAndWindowLineageFuzz()
{
    std::mt19937 gen{0xBADCAB1E};
    for(int trial=0;trial<200;++trial)
    {
        const int count=4+static_cast<int>(gen()%1996);
        const int pauses=(gen()%4==0)?35:0;
        const int speed=300+static_cast<int>(gen()%5500);
        const Fixture fixture(FixtureText(count,pauses,true,speed));
        const auto& r=fixture.Read;
        Require(ValidCoachCsvWindowAudit(r) &&
            r.SourceSamples==static_cast<std::uint64_t>(count),
            "random bounded samples retain authentic parser receipt count");
        std::uint64_t samples=0;
        double seconds=0.0;
        for(unsigned i=0;i<r.RetainedCount;++i)
        {
            const auto& row=r.Rows[i];
            samples+=row.AcceptedSamples;
            seconds+=row.ObservedSeconds;
            Require(ValidCoachCsvWindowRow(row),
                "no malformed window survives source quality validation");
        }
        Require(r.RetainedSamples==samples &&
            std::abs(r.RetainedSeconds-seconds)<1.e-8 &&
            r.RetainedSamples<=r.SourceSamples &&
            r.RetainedCoverageFraction>0.0 &&
            r.RetainedCoverageFraction<=1.0,
            "independent arithmetic oracle verifies sample and coverage totals");
        Require(r.SomeOldWindowsNotRetained ==
            (r.RetainedSamples<r.SourceSamples),
            "fixed ring eviction never becomes a false complete-capture claim");
    }
}
}
int main()
{
    ExactCompleteAndUnfinishedWindowTiming();
    SourceContinuityBreaksNeverCreateFakeWindows();
    RingEvictionRefusesWholeSessionCoverageClaim();
    VeryShortAndUnfinishedSourceIsStillHonestObservation();
    CannotValidateFabricatedRowsOrImpossibleFlags();
    IndependentSampleSumAndWindowLineageFuzz();
    std::cout << "Coach CSV retained-window audit tests passed.\n";
    return EXIT_SUCCESS;
}
