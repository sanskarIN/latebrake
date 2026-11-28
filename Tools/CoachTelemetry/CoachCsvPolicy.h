#pragma once

#include "Racing/DrivingCoachIntegrity.h"
#include "Racing/DrivingCoachFocusAreas.h"
#include "Racing/DrivingCoachPracticeGoals.h"

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>

namespace latebrake::coachcsv
{
// Offline, unauthenticated, read-only QA samples. These are not Unreal
// frame receipts, SaveGame records, or official course/vehicle evidence.
// This policy deliberately does not parse generalized quoted CSV: a fixed,
// strict five-column grammar reduces ambiguity and hostile input surfaces.
inline constexpr std::string_view Header=
    "kind,delta_seconds,speed_cmps,lateral_cmps,braking";
inline constexpr std::size_t MaximumLineBytes=128;
inline constexpr std::uint64_t MaximumTotalBytes=16u*1024u*1024u;
inline constexpr std::uint64_t MaximumDataRows=200000u;
inline constexpr std::uint64_t FingerprintPrime=1099511628211ull;
inline constexpr std::uint64_t FingerprintSeed=14695981039346656037ull;

enum class CsvError : std::uint8_t
{
    None,
    Empty,
    InvalidHeader,
    OversizedLine,
    OversizedInput,
    TooManyRows,
    MissingFields,
    NoncanonicalFields,
    UnsupportedRow,
    InvalidNumber,
    InvalidContext,
    DuplicateContext,
    LateContext,
    InvalidSample,
    DuplicateEnd,
    RowsAfterEnd,
    NoAcceptedSamples,
    StreamFailure,
    InternalSourceMismatch
};

inline const char* CsvErrorLabel(CsvError error) noexcept
{
    switch(error)
    {
    case CsvError::None:return "OK";
    case CsvError::Empty:return "EMPTY INPUT";
    case CsvError::InvalidHeader:return "EXPECTED FIXED FIVE-COLUMN HEADER";
    case CsvError::OversizedLine:return "SAMPLE ROW EXCEEDS BYTE LIMIT";
    case CsvError::OversizedInput:return "INPUT EXCEEDS BYTE LIMIT";
    case CsvError::TooManyRows:return "TOO MANY SAMPLE ROWS";
    case CsvError::MissingFields:return "ROW REQUIRES FIVE COMMA FIELDS";
    case CsvError::NoncanonicalFields:return "NONCANONICAL FIELD ENCODING";
    case CsvError::UnsupportedRow:return "UNKNOWN ROW KIND";
    case CsvError::InvalidNumber:return "INVALID FINITE NUMBER";
    case CsvError::InvalidContext:return "INVALID OPTIONAL LOCAL SESSION CONTEXT";
    case CsvError::DuplicateContext:return "SESSION CONTEXT WAS ALREADY SUPPLIED";
    case CsvError::LateContext:return "SESSION CONTEXT MUST PRECEDE OBSERVATIONS";
    case CsvError::InvalidSample:return "CHASSIS SAMPLE REJECTED";
    case CsvError::DuplicateEnd:return "DUPLICATE END RECEIPT";
    case CsvError::RowsAfterEnd:return "ROW AFTER END RECEIPT";
    case CsvError::NoAcceptedSamples:return "NO ACCEPTED CHASSIS SAMPLES";
    case CsvError::StreamFailure:return "READ FAILURE";
    case CsvError::InternalSourceMismatch:return "COACH/SOURCE INTEGRITY REFUSED";
    default:return "INVALID QA TRACE";
    }
}

// Caller-claimed routing only; never source-authenticated and not persisted.
struct CsvSessionContext final
{
    bool Valid=false;
    int CircuitId=-1;
    int EventId=-1;
    int VehicleId=-1;
    std::uint8_t Mode=0; // 1=practice, 2=race, 3=time
    bool SourceAuthenticated=false;
};

inline bool ValidSessionContext(CsvSessionContext c) noexcept
{
    return c.Valid && c.CircuitId>=0 && c.CircuitId<4 &&
        c.EventId>=0 && c.EventId<4 &&
        c.VehicleId>=0 && c.VehicleId<6 &&
        c.Mode>=1 && c.Mode<=3 && !c.SourceAuthenticated;
}

inline bool ParseSmallId(std::string_view value,int maximum,int& out) noexcept
{
    if(value.empty() || value.size()>2 ||
        (value.size()>1 && value[0]=='0'))return false;
    int parsed=-1;
    const auto [end,err]=std::from_chars(value.data(),
        value.data()+value.size(),parsed);
    if(err!=std::errc{} || end!=value.data()+value.size() ||
       parsed<0 || parsed>=maximum)return false;
    out=parsed;
    return true;
}

inline bool ParseSessionContext(const std::array<std::string_view,5>& fields,
    CsvSessionContext& result) noexcept
{
    if(fields[0]!="context")return false;
    CsvSessionContext candidate;
    if(!ParseSmallId(fields[1],4,candidate.CircuitId) ||
       !ParseSmallId(fields[2],4,candidate.EventId) ||
       !ParseSmallId(fields[3],6,candidate.VehicleId))return false;
    candidate.Mode=fields[4]=="practice" ? 1u :
        fields[4]=="race" ? 2u : fields[4]=="time" ? 3u : 0u;
    candidate.Valid=candidate.Mode!=0;
    if(!ValidSessionContext(candidate))return false;
    result=candidate;
    return true;
}

struct CsvAudit final
{
    bool Valid=false;
    CsvError Error=CsvError::Empty;
    std::uint64_t ErrorLine=0;
    std::uint64_t TotalBytes=0;
    std::uint64_t PhysicalLines=0;
    std::uint64_t AcceptedRows=0;
    std::uint64_t SuspensionRows=0;
    std::uint64_t Fnv64=FingerprintSeed;
    bool ExplicitEnd=false;
    CsvSessionContext Context{};
    bool SourceIsAuthenticated=false;
    bool NativeUnrealVerified=false;
    bool PhysicsAuthorityChanged=false;
    bool SaveDataRead=false;
    bool SaveDataWritten=false;
    DrivingCoachIntegrityResult Integrity{};
    DrivingCoachReview Review{};
    DrivingCoachFocusReview Focus{};
    DrivingCoachGoalPlan Goals{};
};

inline bool ParseAsciiDecimal(std::string_view token,double& out) noexcept
{
    if(token.empty() || token.size()>32)return false;
    // Ordinary decimal/scientific notation only. Whitespace, leading plus,
    // commas, locale decimal separators and NaN/Infinity are not accepted.
    for(char ch:token)
    {
        if((ch>='0' && ch<='9') || ch=='-' || ch=='.' ||
           ch=='e' || ch=='E' || ch=='+')continue;
        return false;
    }
    if(token.front()=='+')return false;
    double parsed=0.0;
    const auto [end,err]=std::from_chars(
        token.data(),token.data()+token.size(),parsed,
        std::chars_format::general);
    if(err!=std::errc{} || end!=token.data()+token.size() ||
       !std::isfinite(parsed))return false;
    out=parsed;
    return true;
}

inline bool SplitFive(std::string_view line,
    std::array<std::string_view,5>& fields) noexcept
{
    std::size_t cursor=0;
    for(std::size_t i=0;i<4;++i)
    {
        const auto index=line.find(',',cursor);
        if(index==std::string_view::npos)return false;
        fields[i]=line.substr(cursor,index-cursor);
        cursor=index+1;
    }
    if(line.find(',',cursor)!=std::string_view::npos)return false;
    fields[4]=line.substr(cursor);
    return true;
}

enum class BoundedReadStatus : std::uint8_t
{
    Line,
    End,
    LineTooLong,
    InputTooLarge,
    IOFailure
};

// Never let std::getline allocate an unbounded hostile record. Consume at
// most MaximumLineBytes; stop immediately on overflow instead of scanning
// arbitrary attacker-controlled megabytes inside one line.
inline BoundedReadStatus ReadBoundedLine(std::istream& stream,
    std::string& out,std::uint64_t& bytes,std::uint64_t& fingerprint) noexcept
{
    out.clear();
    bool sawAny=false;
    for(;;)
    {
        const auto value=stream.get();
        if(value==std::char_traits<char>::eof())
        {
            if(stream.bad())return BoundedReadStatus::IOFailure;
            return sawAny?BoundedReadStatus::Line:BoundedReadStatus::End;
        }
        sawAny=true;
        if(bytes>=MaximumTotalBytes)return BoundedReadStatus::InputTooLarge;
        ++bytes;
        const auto ch=static_cast<unsigned char>(value);
        fingerprint=(fingerprint^ch)*FingerprintPrime;
        if(ch=='\n')break;
        if(out.size()>=MaximumLineBytes)
            return BoundedReadStatus::LineTooLong;
        if(ch==0u)return BoundedReadStatus::LineTooLong;
        out.push_back(static_cast<char>(ch));
    }
    // Accept CRLF from Windows files without including CR in data fields.
    if(!out.empty() && out.back()=='\r')out.pop_back();
    if(out.find('\r')!=std::string::npos)
        return BoundedReadStatus::LineTooLong;
    return BoundedReadStatus::Line;
}

inline CsvAudit AnalyzeCsv(std::istream& stream,
    DrivingSessionStats& destination) noexcept
{
    CsvAudit report;
    DrivingSessionStats staged;
    std::string line;
    std::uint64_t bytes=0;
    std::uint64_t hash=FingerprintSeed;
    auto refuse=[&](CsvError reason) noexcept
    {
        report.Error=reason;
        report.ErrorLine=report.PhysicalLines;
        report.TotalBytes=bytes;
        report.Fnv64=hash;
        return report;
    };
    bool header=false;
    bool ended=false;
    for(;;)
    {
        const auto status=ReadBoundedLine(stream,line,bytes,hash);
        if(status==BoundedReadStatus::End)break;
        // Even a malformed oversized row is the next physical line.
        ++report.PhysicalLines;
        if(status==BoundedReadStatus::LineTooLong)
            return refuse(CsvError::OversizedLine);
        if(status==BoundedReadStatus::InputTooLarge)
            return refuse(CsvError::OversizedInput);
        if(status==BoundedReadStatus::IOFailure)
            return refuse(CsvError::StreamFailure);
        if(!header)
        {
            header=true;
            if(line!=Header)return refuse(CsvError::InvalidHeader);
            continue;
        }
        if(ended)return refuse(line=="end,,,," ?
            CsvError::DuplicateEnd:CsvError::RowsAfterEnd);
        if(report.AcceptedRows+report.SuspensionRows>=MaximumDataRows)
            return refuse(CsvError::TooManyRows);
        std::array<std::string_view,5> fields{};
        if(!SplitFive(line,fields))return refuse(CsvError::MissingFields);
        if(fields[0]=="context")
        {
            if(report.Context.Valid)return refuse(CsvError::DuplicateContext);
            if(report.AcceptedRows>0 || report.SuspensionRows>0)
                return refuse(CsvError::LateContext);
            if(!ParseSessionContext(fields,report.Context))
                return refuse(CsvError::InvalidContext);
            continue;
        }
        if(fields[0]=="end")
        {
            for(std::size_t i=1;i<fields.size();++i)
                if(!fields[i].empty())return refuse(CsvError::NoncanonicalFields);
            ended=true;
            report.ExplicitEnd=true;
            continue;
        }
        if(fields[0]=="suspend")
        {
            for(std::size_t i=1;i<fields.size();++i)
                if(!fields[i].empty())return refuse(CsvError::NoncanonicalFields);
            staged.Suspend();
            ++report.SuspensionRows;
            continue;
        }
        if(fields[0]!="sample")return refuse(CsvError::UnsupportedRow);
        double dt=0.0,velocity=0.0,lateral=0.0;
        if(!ParseAsciiDecimal(fields[1],dt) ||
           !ParseAsciiDecimal(fields[2],velocity) ||
           !ParseAsciiDecimal(fields[3],lateral) ||
           (fields[4]!="0" && fields[4]!="1"))
            return refuse(CsvError::InvalidNumber);
        if(!staged.Observe({dt,velocity,lateral,fields[4]=="1"}))
            return refuse(CsvError::InvalidSample);
        ++report.AcceptedRows;
    }
    if(!header)return refuse(CsvError::Empty);
    if(report.AcceptedRows==0)return refuse(CsvError::NoAcceptedSamples);
    report.Integrity=VerifyDrivingCoachIntegrity(staged);
    if(!report.Integrity.Valid)
        return refuse(CsvError::InternalSourceMismatch);
    const auto snapshot=staged.GetCoachReview();
    report.Review=BuildDrivingCoachReview(snapshot);
    report.Focus=BuildDrivingCoachFocusAreas(snapshot);
    report.Goals=BuildDrivingCoachGoals(snapshot);
    if(!report.Focus.Valid || !report.Goals.Valid ||
       (report.Integrity.AdviceAllowed &&
        (!report.Review.Valid || !ValidDrivingCoachGoalPlan(report.Goals))))
        return refuse(CsvError::InternalSourceMismatch);
    report.Valid=true;
    report.Error=CsvError::None;
    report.TotalBytes=bytes;
    report.Fnv64=hash;
    destination=staged;
    return report;
}
} // namespace latebrake::coachcsv
