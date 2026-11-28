#include "CoachCsvComparison.h"
#include "CoachCsvQuality.h"
#include "CoachCsvBatchPairs.h"
#include "CoachCsvWindowAudit.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <system_error>

namespace
{
namespace coachcsv=latebrake::coachcsv;

void PrintNumber(bool valid,double value)
{
    if(valid && std::isfinite(value))std::cout << value;
    else std::cout << "null";
}

void PrintFingerprint(std::uint64_t value)
{
    std::cout << std::hex << std::setw(16) << std::setfill('0') << value
        << std::dec << std::setfill(' ');
}

void PrintQuality(const coachcsv::CsvAudit& audit,
    const latebrake::DrivingSessionStats& stats)
{
    const auto report=coachcsv::BuildCoachCsvQualityReview(audit,stats);
    if(!coachcsv::ValidCoachCsvQualityReview(report,audit,stats))
    {
        std::cout << "null";
        return;
    }
    std::cout << "{\"category\": " << static_cast<unsigned>(report.Category)
        << ", \"sample_count\": " << report.SampleCount
        << ", \"observed_seconds\": " << report.ObservedSeconds
        << ", \"occupied_speed_bands\": " << report.OccupiedSpeedBands
        << ", \"dominant_speed_band\": " << report.DominantSpeedBand
        << ", \"dominant_band_concentration\": "
        << report.SpeedBandConcentration
        << ", \"speed_band_diversity\": "
        << report.SpeedBandSimpsonDiversity
        << ", \"source_authenticated\": false"
        << ", \"official_race_evidence\": false"
        << ", \"notices\": [";
    for(unsigned index=0;index<report.NoticeCount;++index)
    {
        if(index)std::cout << ',';
        // Every label is a fixed compile-time string, never user CSV text.
        std::cout << "\"" << coachcsv::CoachCsvQualityNoticeLabel(
            report.Notices[index]) << "\"";
    }
    std::cout << "]}";
}

void PrintWindowAudit(const coachcsv::CsvAudit& audit,
    const latebrake::DrivingSessionStats& stats)
{
    const auto windows=coachcsv::BuildCoachCsvWindowAudit(audit,stats);
    if(!coachcsv::ValidCoachCsvWindowAudit(windows))
    {
        std::cout << "null";
        return;
    }
    std::cout << "{\"valid\": true, \"source_authenticity_verified\": false"
        << ", \"official_lap_timing_verified\": false"
        << ", \"writes_save_data\": false"
        << ", \"source_samples\": " << windows.SourceSamples
        << ", \"retained_samples\": " << windows.RetainedSamples
        << ", \"retained_window_count\": " << windows.RetainedCount
        << ", \"complete_windows\": " << windows.CompleteCount
        << ", \"partial_windows\": " << windows.PartialCount
        << ", \"observed_epoch_changes\": " << windows.ObservedEpochChanges
        << ", \"older_windows_evicted\": " << windows.SomeOldWindowsNotRetained
        << ", \"retained_coverage_fraction\": " << windows.RetainedCoverageFraction
        << ", \"retained_seconds\": " << windows.RetainedSeconds
        << ", \"source_observed_seconds\": " << windows.SourceObservedSeconds
        << ", \"newest_first\": true, \"windows\": [";
    for(unsigned index=0;index<windows.RetainedCount;++index)
    {
        const auto& row=windows.Rows[index];
        if(index)std::cout << ',';
        std::cout << "{\"sequence\": " << row.Sequence
            << ", \"epoch\": " << row.ContinuityEpoch
            << ", \"complete\": " << row.Complete
            << ", \"started_seconds\": " << row.StartedSeconds
            << ", \"ended_seconds\": " << row.EndedSeconds
            << ", \"observed_seconds\": " << row.ObservedSeconds
            << ", \"accepted_samples\": " << row.AcceptedSamples
            << ", \"mean_speed_kmh\": " << row.MeanSpeedKmh
            << ", \"estimated_distance_km\": " << row.EstimatedDistanceKm
            << ", \"braking_share\": " << row.BrakingFraction
            << ", \"high_slip_share\": " << row.HighSlipFraction << '}';
    }
    std::cout << "], \"warning\": "
        << "\"Window timestamps count CSV sample time only; they are not "
           "verified engine time, lap times or race evidence.\"}";
}

void PrintJson(const coachcsv::CsvAudit& audit,
    const latebrake::DrivingSessionStats& stats)
{
    std::cout << std::boolalpha << std::setprecision(12);
    const auto read=stats.GetCoachReview();
    std::cout << "{\n"
        << "  \"schema\": 1,\n"
        << "  \"kind\": \"latebrake-local-coach-csv-audit\",\n"
        << "  \"valid\": " << audit.Valid << ",\n"
        << "  \"source_authenticated\": false,\n"
        << "  \"native_unreal_verified\": false,\n"
        << "  \"race_timing_verified\": false,\n"
        << "  \"saved_data_read\": false,\n"
        << "  \"saved_data_written\": false,\n"
        << "  \"source_sampled_integrity\": " << audit.Integrity.Valid << ",\n"
        << "  \"advice_allowed\": " << audit.Integrity.AdviceAllowed << ",\n"
        << "  \"bytes\": " << audit.TotalBytes << ",\n"
        << "  \"lines\": " << audit.PhysicalLines << ",\n"
        << "  \"accepted_rows\": " << audit.AcceptedRows << ",\n"
        << "  \"suspension_rows\": " << audit.SuspensionRows << ",\n"
        << "  \"end_receipt\": " << audit.ExplicitEnd << ",\n"
        << "  \"claimed_context\": ";
    if(coachcsv::ValidSessionContext(audit.Context))
    {
        std::cout << "{\"circuit_id\": " << audit.Context.CircuitId
            << ", \"event_id\": " << audit.Context.EventId
            << ", \"vehicle_id\": " << audit.Context.VehicleId
            << ", \"mode_id\": " << static_cast<unsigned>(audit.Context.Mode)
            << ", \"authenticated\": false}";
    }
    else std::cout << "null";
    std::cout << ",\n  \"fingerprint_fnv64\": \"";
    PrintFingerprint(audit.Fnv64);
    std::cout << "\",\n  \"sampled_seconds\": ";
    PrintNumber(read.Valid,read.ObservedSeconds);
    std::cout << ",\n  \"sampled_distance_km\": ";
    PrintNumber(read.Valid,read.DistanceKm);
    std::cout << ",\n  \"mean_speed_kmh\": ";
    PrintNumber(read.Valid,read.MeanSpeedKmh);
    std::cout << ",\n  \"peak_speed_kmh\": ";
    PrintNumber(read.Valid,read.PeakSpeedKmh);
    std::cout << ",\n  \"braking_share\": ";
    PrintNumber(read.Valid,read.BrakeFraction);
    std::cout << ",\n  \"high_slip_share\": ";
    PrintNumber(read.Valid,read.HighSlipFraction);
    std::cout << ",\n  \"sampled_brake_episodes\": " << read.AcceptedBrakeEpisodes;
    std::cout << ",\n  \"sampled_slip_episodes\": " << read.AcceptedSlipEpisodes;
    std::cout << ",\n  \"focus_areas\": [";
    for(std::size_t i=0;i<audit.Focus.Count;++i)
    {
        const auto& focus=audit.Focus.Areas[i];
        if(i)std::cout << ',';
        std::cout << "\n    {\"category\": " << static_cast<unsigned>(focus.Kind)
            << ", \"sequence\": " << focus.Sequence
            << ", \"start_seconds\": " << focus.StartedSeconds
            << ", \"end_seconds\": " << focus.EndedSeconds
            << ", \"severity\": " << focus.Severity << '}';
    }
    std::cout << "\n  ],\n  \"practice_goals\": [";
    for(std::size_t i=0;i<audit.Goals.Count;++i)
    {
        const auto& goal=audit.Goals.Goals[i];
        if(i)std::cout << ',';
        std::cout << "\n    {\"kind\": " << static_cast<unsigned>(goal.Kind)
            << ", \"baseline\": " << goal.Baseline
            << ", \"target\": " << goal.Target << '}';
    }
    std::cout << "\n  ],\n  \"quality\": ";
    PrintQuality(audit,stats);
    std::cout << ",\n  \"retained_window_audit\": ";
    PrintWindowAudit(audit,stats);
    std::cout << ",\n  \"warning\": "
        << "\"Editable user-supplied CSV is structural sample evidence only; "
           "not engine authenticity, physics, release, or career record proof.\"\n}"
        << '\n';
}

void PrintMetric(const char* name,const coachcsv::ComparisonMetric& metric,
    bool trailing)
{
    std::cout << "    \"" << name << "\": {\"baseline\": "
        << metric.BaselineValue << ", \"candidate\": "
        << metric.CandidateValue << ", \"delta\": "
        << metric.AbsoluteDifference << ", \"relative_percent\": "
        << metric.RelativeDifferencePercent << ", \"direction\": "
        << static_cast<unsigned>(metric.Direction) << '}';
    if(trailing)std::cout << ',';
    std::cout << '\n';
}

void PrintComparison(const coachcsv::CoachCsvComparison& comparison,
    const coachcsv::CsvAudit& baseline,const coachcsv::CsvAudit& candidate)
{
    std::cout << std::boolalpha << std::setprecision(12);
    std::cout << "{\n"
        << "  \"schema\": 1,\n"
        << "  \"kind\": \"latebrake-local-coach-csv-comparison\",\n"
        << "  \"valid\": " << comparison.Valid << ",\n"
        << "  \"source_authenticated\": false,\n"
        << "  \"native_unreal_verified\": false,\n"
        << "  \"race_timing_verified\": false,\n"
        << "  \"same_authenticated_track\": false,\n"
        << "  \"same_authenticated_car\": false,\n"
        << "  \"competitive_improvement_verified\": false,\n"
        << "  \"saved_data_read\": false,\n"
        << "  \"saved_data_written\": false,\n"
        << "  \"official_lap_evidence\": false,\n"
        << "  \"claimed_context\": {\"circuit_id\": "
        << comparison.ClaimedContext.CircuitId << ", \"event_id\": "
        << comparison.ClaimedContext.EventId << ", \"vehicle_id\": "
        << comparison.ClaimedContext.VehicleId << ", \"mode_id\": "
        << static_cast<unsigned>(comparison.ClaimedContext.Mode) << "},\n"
        << "  \"baseline\": {\"accepted_rows\": " << baseline.AcceptedRows
        << ", \"observed_seconds\": " << comparison.BaselineObservedSeconds
        << ", \"fingerprint_fnv64\": \"";
    PrintFingerprint(baseline.Fnv64);
    std::cout << "\"},\n  \"candidate\": {\"accepted_rows\": "
        << candidate.AcceptedRows << ", \"observed_seconds\": "
        << comparison.CandidateObservedSeconds
        << ", \"fingerprint_fnv64\": \"";
    PrintFingerprint(candidate.Fnv64);
    std::cout << "\"},\n  \"metrics\": {\n";
    PrintMetric("mean_speed_kmh",comparison.MeanSpeedKmh,true);
    PrintMetric("braking_time_percent",comparison.BrakingTimePercent,true);
    PrintMetric("high_slip_time_percent",comparison.HighSlipTimePercent,true);
    PrintMetric("moving_time_percent",comparison.MovingTimePercent,true);
    PrintMetric("brake_applications_per_minute",comparison.BrakeApplicationsPerMinute,true);
    PrintMetric("strong_speed_changes_per_minute",
        comparison.StrongSpeedChangesPerMinute,false);
    std::cout << "  },\n  \"warning\": "
        << "\"Matching metadata and sampled metrics are unauthenticated, "
           "not independently verified lap, driving skill or race performance claims.\"\n}"
        << '\n';
}

void PrintBatch(const coachcsv::CoachBatchReview& report,
    const coachcsv::CoachBatchPairReview& pairs)
{
    std::cout << std::boolalpha << std::setprecision(12);
    std::cout << "{\n"
        << "  \"schema\": 1,\n"
        << "  \"kind\": \"latebrake-local-coach-csv-batch\",\n"
        << "  \"valid\": true,\n"
        << "  \"source_authenticated\": false,\n"
        << "  \"official_race_ranking\": false,\n"
        << "  \"source_sessions_independent\": false,\n"
        << "  \"identical_track_conditions_verified\": false,\n"
        << "  \"modifies_game_profile\": false,\n"
        << "  \"file_count\": " << report.Count << ",\n"
        << "  \"status\": \""
        << coachcsv::CoachBatchReviewStatusLabel(report.Status)
        << "\",\n  \"all_share_claimed_context\": "
        << report.AllClaimSameContext
        << ",\n  \"repeated_input_fingerprint_pairs\": "
        << report.RepeatedFingerprintPairs
        << ",\n  \"sparse_files\": " << report.SparseFiles
        << ",\n  \"interrupted_files\": " << report.InterruptedFiles
        << ",\n  \"missing_end_files\": " << report.MissingEndFiles
        << ",\n  \"minimum_mean_speed_kmh\": "
        << report.MinimumMeanSpeedKmh
        << ",\n  \"maximum_mean_speed_kmh\": "
        << report.MaximumMeanSpeedKmh
        << ",\n  \"pair_review\": {\"pairs\": " << pairs.PairCount
        << ", \"reused_input_pairs\": " << pairs.RepeatedInputPairs
        << ", \"missing_claim_pairs\": " << pairs.MissingClaimPairs
        << ", \"different_claim_pairs\": " << pairs.DifferentClaimPairs
        << ", \"insufficient_pairs\": " << pairs.InsufficientPairs
        << ", \"compatible_claim_pairs\": " << pairs.CompatibleClaimPairs
        << ", \"authenticity_established\": false, "
        << "\"official_performance_ranked\": false},\n  \"entries\": [";
    for(std::size_t index=0;index<report.Count;++index)
    {
        const auto& entry=report.Entries[index];
        if(index)std::cout << ',';
        std::cout << "\n    {\"index\": " << index
            << ", \"accepted_samples\": " << entry.AcceptedSamples
            << ", \"sampled_seconds\": " << entry.ObservedSeconds
            << ", \"mean_speed_kmh\": " << entry.MeanSpeedKmh
            << ", \"braking_share\": " << entry.BrakingShare
            << ", \"high_slip_share\": " << entry.HighSlipShare
            << ", \"moving_share\": " << entry.MovingShare
            << ", \"speed_diversity\": " << entry.SpeedDiversity
            << ", \"end_marker\": " << entry.EndMarkerPresent
            << ", \"claimed_context\": " << entry.HasClaimedContext
            << ", \"quality_category\": "
            << static_cast<unsigned>(entry.Quality)
            << ", \"fingerprint_fnv64\": \"";
        PrintFingerprint(entry.Fingerprint);
        std::cout << "\"}";
    }
    std::cout << "\n  ],\n  \"warning\": "
        << "\"Every file is an editable local observation; neither matching "
           "metadata nor hashes prove actual source, race timing or driving skill.\"\n}"
        << '\n';
}

int LoadCsvFile(const std::filesystem::path& path,
    coachcsv::CsvAudit& audit,latebrake::DrivingSessionStats& stats)
{
    namespace fs=std::filesystem;
    std::error_code error;
    if(fs::is_symlink(path,error) || error ||
       !fs::is_regular_file(path,error) || error)
    {
        std::cerr << "Source file must be a regular, non-symlink local CSV.\n";
        return 66;
    }
    const auto size=fs::file_size(path,error);
    if(error || size==0 || size>coachcsv::MaximumTotalBytes)
    {
        std::cerr << "Source file is empty, inaccessible or above 16 MiB.\n";
        return 66;
    }
    std::ifstream stream(path,std::ios::binary);
    if(!stream)
    {
        std::cerr << "Cannot open source CSV for read only.\n";
        return 66;
    }
    audit=coachcsv::AnalyzeCsv(stream,stats);
    if(!audit.Valid)
    {
        std::cerr << "LateBrake coach CSV rejected at line " << audit.ErrorLine
            << ": " << coachcsv::CsvErrorLabel(audit.Error) << '\n';
        return 2;
    }
    return 0;
}
}

int main(int argc,char** argv)
{
    const bool one=argc==2 && argv[1] &&
        std::string_view(argv[1]).size()>0 &&
        std::string_view(argv[1])!="--batch" &&
        std::string_view(argv[1])!="--compare";
    const bool compare=argc==4 && argv[1] && argv[2] && argv[3] &&
        std::string_view(argv[1])=="--compare" &&
        std::string_view(argv[2]).size()>0 &&
        std::string_view(argv[3]).size()>0;
    const bool batch=argc>=4 &&
        argc<=static_cast<int>(coachcsv::MaximumCoachBatchFiles)+2 &&
        argv[1] && std::string_view(argv[1])=="--batch";
    if(!one && !compare && !batch)
    {
        std::cerr << "Usage: latebrake_coach_csv_inspector <local-coach.csv>\n"
            << "   or: latebrake_coach_csv_inspector --compare <before.csv> <after.csv>\n"
            << "   or: latebrake_coach_csv_inspector --batch <a.csv> <b.csv> [up to 12]\n"
            << "Header: " << coachcsv::Header << '\n';
        return 64;
    }
    if(one)
    {
        coachcsv::CsvAudit audit;
        latebrake::DrivingSessionStats stats;
        const auto status=LoadCsvFile(argv[1],audit,stats);
        if(status)return status;
        PrintJson(audit,stats);
        return 0;
    }
    if(batch)
    {
        constexpr auto Limit=coachcsv::MaximumCoachBatchFiles;
        std::array<coachcsv::CsvAudit,Limit> audits{};
        std::array<latebrake::DrivingSessionStats,Limit> stats{};
        const auto count=static_cast<std::size_t>(argc-2);
        for(std::size_t index=0;index<count;++index)
        {
            if(!argv[index+2] || std::string_view(argv[index+2]).empty())
                return 64;
            const auto status=LoadCsvFile(argv[index+2],audits[index],stats[index]);
            if(status)return status;
        }
        const auto report=coachcsv::BuildCoachCsvBatch(
            audits.data(),stats.data(),count);
        if(!coachcsv::ValidCoachCsvBatchReview(report))
        {
            std::cerr << "LateBrake coach batch refused: invalid source evidence.\n";
            return 3;
        }
        const auto pairs=coachcsv::BuildCoachBatchPairReview(report);
        if(!coachcsv::ValidCoachBatchPairReview(pairs,report))
        {
            std::cerr << "LateBrake coach pair review refused: invalid source evidence.\n";
            return 3;
        }
        PrintBatch(report,pairs);
        return 0;
    }
    coachcsv::CsvAudit baseline;
    coachcsv::CsvAudit candidate;
    latebrake::DrivingSessionStats beforeStats;
    latebrake::DrivingSessionStats afterStats;
    const auto before=LoadCsvFile(argv[2],baseline,beforeStats);
    if(before)return before;
    const auto after=LoadCsvFile(argv[3],candidate,afterStats);
    if(after)return after;
    const auto review=coachcsv::CompareCoachCsvSessions(
        baseline,beforeStats,candidate,afterStats);
    if(!coachcsv::ValidCoachCsvComparison(review))
    {
        std::cerr << "LateBrake coach comparison refused: "
            << coachcsv::ComparisonStatusLabel(review.Status) << '\n';
        return 3;
    }
    PrintComparison(review,baseline,candidate);
    return 0;
}
