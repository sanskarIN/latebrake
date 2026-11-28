#include "../../Tools/CoachTelemetry/CoachCsvPolicy.h"

#include <array>
#include <cmath>
#include <cstdint>
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
void Require(bool v,const char* reason)
{
    if(!v)
    {
        std::cerr << "CoachCsvPolicy: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
std::string Sample(int brake=0,double speed=1800.0,
    double lateral=100.0,double dt=0.1)
{
    std::ostringstream s;
    s << "sample," << dt << ',' << speed << ',' << lateral << ',' << brake << '\n';
    return s.str();
}
std::string MakeTrace(int frames=80,bool includeEnd=true)
{
    std::string out=std::string(Header)+'\n';
    for(int i=0;i<frames;++i)
        out+=Sample(i%7==0,1500.0+i%31,100.0+i%7);
    if(includeEnd)out+="end,,,,\n";
    return out;
}
CsvAudit Run(std::string text,DrivingSessionStats& destination)
{
    std::istringstream input{text};
    return AnalyzeCsv(input,destination);
}
void SourceAcceptanceAndFingerprints()
{
    DrivingSessionStats collected;
    const auto first=Run(MakeTrace(80),collected);
    Require(first.Valid && first.Error==CsvError::None &&
        first.AcceptedRows==80 && first.SuspensionRows==0 &&
        first.PhysicalLines==82 && first.ExplicitEnd,
        "fixed header and original source samples accepted");
    Require(first.Integrity.AdviceAllowed && first.Review.Valid &&
        first.Focus.Valid && first.Goals.Valid &&
        collected.GetAcceptedSamples()==80,
        "independent source counters, review, focus and practice goals connected");
    const auto second=Run(MakeTrace(80),collected);
    Require(first.Fnv64==second.Fnv64 &&
        first.TotalBytes==second.TotalBytes,
        "byte-stable deterministic diagnostic fingerprint");
    const auto distinct=Run(MakeTrace(81),collected);
    Require(distinct.Fnv64!=first.Fnv64 &&
        distinct.AcceptedRows==81,
        "even one additional sample changes diagnostic fingerprint");
    Require(!first.SourceIsAuthenticated && !first.NativeUnrealVerified &&
        !first.PhysicsAuthorityChanged && !first.SaveDataRead &&
        !first.SaveDataWritten,
        "editable diagnostic never asserts release/source authenticity");
}
void ParseGrammarAndNumberBoundaries()
{
    for(const auto& good: {"0", "0.000001", "12", "1800.00", "1e2", "1E-3"})
    {
        double out=-1;
        Require(ParseAsciiDecimal(good,out) && out>=0.0,
            "known canonical float accepted");
    }
    for(const auto& bad: {"", "--1", "1,5", " +1", "+1", "NaN",
        "inf", "Infinity", "1.2e9999", "0x1p1", "1. 2", "1 2",
        "3;4", "3.0junk"})
    {
        double out=42;
        Require(!ParseAsciiDecimal(bad,out) && out==42,
            "malformed float refused without mutating output");
    }
    std::array<std::string_view,5> columns{};
    Require(SplitFive("sample,0.1,2000,500,1",columns) &&
        columns[1]=="0.1" && columns[4]=="1",
        "fixed five-column split");
    Require(!SplitFive("sample,0.1,2000,500",columns) &&
        !SplitFive("sample,0.1,2000,500,1,extra",columns),
        "missing/extra delimiter cannot be silently dropped");
}
void FailClosedAndTransactional()
{
    DrivingSessionStats destination;
    Require(destination.Observe({0.1,1200.0,0.0,false}),
        "preset caller state for atomic replacement");
    auto baseline=destination.GetCoachReview();
    const auto invalid=[&](std::string text,CsvError expected)
    {
        const auto report=Run(text,destination);
        if(report.Valid || report.Error!=expected)
            std::cerr << "invalid sample expected " << static_cast<int>(expected)
                << " got " << static_cast<int>(report.Error)
                << " line " << report.ErrorLine << " input " << text.substr(0,100) << '\n';
        Require(!report.Valid && report.Error==expected,
            "specific malformed trace error returned");
        const auto current=destination.GetCoachReview();
        Require(current.AcceptedSamples==baseline.AcceptedSamples &&
            current.ObservedSeconds==baseline.ObservedSeconds,
            "failed parse never commits partial telemetry to caller");
    };
    invalid("",CsvError::Empty);
    invalid("wrong,columns\n",CsvError::InvalidHeader);
    invalid(std::string(Header)+'\n',CsvError::NoAcceptedSamples);
    invalid(std::string(Header)+"\nwrong,0.1,1200,0,0\n",CsvError::UnsupportedRow);
    invalid(std::string(Header)+"\nsample,0.1,1200,0\n",CsvError::MissingFields);
    invalid(std::string(Header)+"\nsample,0.1,1200,0,1,extra\n",CsvError::MissingFields);
    invalid(std::string(Header)+"\nsample,0.1,NaN,0,0\n",CsvError::InvalidNumber);
    invalid(std::string(Header)+"\nsample,0.3,1200,0,0\n",CsvError::InvalidSample);
    invalid(std::string(Header)+"\nsuspend,0,,,\n",CsvError::NoncanonicalFields);
    invalid(std::string(Header)+"\nend,,,,\nsample,0.1,1200,0,1\n",CsvError::RowsAfterEnd);
    invalid(std::string(Header)+"\nsample,0.1,1200,0,1\nend,,,,\nend,,,,\n",CsvError::DuplicateEnd);
    invalid(std::string(Header)+"\n"+std::string(130,'a')+"\n",CsvError::OversizedLine);
    auto nul=std::string(Header)+"\nsample,0.1,1200,0,1";
    nul.push_back('\0');
    nul+="XYZ\n";
    invalid(nul,CsvError::OversizedLine);
    invalid(std::string(Header)+"\nsample,0.1,1200,0,2\n",CsvError::InvalidNumber);
}
void RecoveryAndCRLF()
{
    std::string data=std::string(Header)+"\r\n";
    for(int i=0;i<300;++i)
    {
        if(i%41==0)data+="suspend,,,,\r\n";
        data+="sample,0.100,2000,800,1\r\n";
    }
    data+="end,,,,\r\n";
    DrivingSessionStats stats;
    const auto report=Run(data,stats);
    Require(report.Valid && report.AcceptedRows==300 &&
        report.SuspensionRows==8 && report.ExplicitEnd,
        "Windows CRLF and explicit gap markers supported");
    Require(report.Integrity.AdviceAllowed &&
        stats.GetCoachReview().Suspensions==8 &&
        stats.GetCoachReview().AcceptedBrakeEpisodes==8,
        "suspension prevents artificial cross-gap brake merging");
}
void SeededRegression()
{
    std::mt19937 rng{0xDA5B00};
    std::string data=std::string(Header)+'\n';
    std::uint64_t frames=0,gaps=0;
    for(int i=0;i<12000;++i)
    {
        if(i%521==0)
        {
            data+="suspend,,,,\n";
            ++gaps;
        }
        const double v=static_cast<double>((rng()%9000)+10);
        const double lat=static_cast<double>(static_cast<int>(rng()%18000)-9000);
        const bool brake=(rng()%4)==0;
        data+=Sample(brake,v,lat,0.05);
        ++frames;
    }
    DrivingSessionStats stats;
    const auto result=Run(data,stats);
    Require(result.Valid && result.AcceptedRows==frames &&
        result.SuspensionRows==gaps &&
        VerifyDrivingCoachIntegrity(stats).AdviceAllowed,
        "12,000 real-format source samples remain analyzable after bounded gaps");
    Require(result.Focus.Valid && result.Goals.Valid,
        "high-volume trace yields bounded review and practice guidance");
}
}
int main()
{
    SourceAcceptanceAndFingerprints();
    ParseGrammarAndNumberBoundaries();
    FailClosedAndTransactional();
    RecoveryAndCRLF();
    SeededRegression();
    std::cout << "Coach CSV strict offline parser tests passed.\n";
    return EXIT_SUCCESS;
}
