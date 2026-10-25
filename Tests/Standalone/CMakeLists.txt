cmake_minimum_required(VERSION 3.16)
project(LatebrakePortableTests LANGUAGES CXX)

enable_testing()

add_executable(latebrake_race_tests
    RaceProgressTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)

target_include_directories(latebrake_race_tests PRIVATE
    ../../Source/Latebrake/Public
)
target_compile_features(latebrake_race_tests PRIVATE cxx_std_20)

if (MSVC)
    target_compile_options(latebrake_race_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_race_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME RaceProgress COMMAND latebrake_race_tests)

add_executable(latebrake_pilot_tests RivalPilotTests.cpp)
target_include_directories(latebrake_pilot_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_pilot_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_pilot_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_pilot_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalPilot COMMAND latebrake_pilot_tests)

add_executable(latebrake_rewards_tests RaceRewardsTests.cpp)
target_include_directories(latebrake_rewards_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rewards_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rewards_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rewards_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RaceRewards COMMAND latebrake_rewards_tests)

add_executable(latebrake_route_tests RivalRouteTests.cpp)
target_include_directories(latebrake_route_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_route_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_route_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_route_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalRoute COMMAND latebrake_route_tests)

add_executable(latebrake_steering_tests SteeringFilterTests.cpp)
target_include_directories(latebrake_steering_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_steering_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_steering_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_steering_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME SteeringFilter COMMAND latebrake_steering_tests)

# v0.5.48: shared player/touch/controller/rival high-speed steering policy.
add_executable(latebrake_speed_sensitive_steering_tests
    SpeedSensitiveSteeringTests.cpp
)
target_include_directories(latebrake_speed_sensitive_steering_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_speed_sensitive_steering_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_speed_sensitive_steering_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_speed_sensitive_steering_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME SpeedSensitiveSteering COMMAND latebrake_speed_sensitive_steering_tests)

add_executable(latebrake_garage_tests GarageStateTests.cpp)
target_include_directories(latebrake_garage_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_garage_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_garage_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_garage_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME GarageState COMMAND latebrake_garage_tests)

add_executable(latebrake_hud_tests HudLayoutTests.cpp)
target_include_directories(latebrake_hud_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_hud_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_hud_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_hud_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME HudLayout COMMAND latebrake_hud_tests)

add_executable(latebrake_record_tests RecordValidationTests.cpp)
target_include_directories(latebrake_record_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_record_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_record_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_record_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RecordValidation COMMAND latebrake_record_tests)

add_executable(latebrake_input_safety_tests DriverInputSafetyTests.cpp)
target_include_directories(latebrake_input_safety_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_input_safety_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_input_safety_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_input_safety_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME DriverInputSafety COMMAND latebrake_input_safety_tests)

add_executable(latebrake_garage_tuning_tests GarageTuningKeyTests.cpp)
target_include_directories(latebrake_garage_tuning_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_garage_tuning_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_garage_tuning_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_garage_tuning_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME GarageTuningKey COMMAND latebrake_garage_tuning_tests)

add_executable(latebrake_circuit_layout_tests CircuitLayoutTests.cpp)
target_include_directories(latebrake_circuit_layout_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_circuit_layout_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_circuit_layout_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_circuit_layout_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CircuitLayout COMMAND latebrake_circuit_layout_tests)

add_executable(latebrake_track_decorations_tests TrackDecorationsTests.cpp)
target_include_directories(latebrake_track_decorations_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_track_decorations_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_track_decorations_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_track_decorations_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME TrackDecorations COMMAND latebrake_track_decorations_tests)

add_executable(latebrake_circuit_map_tests CircuitMapTests.cpp)
target_include_directories(latebrake_circuit_map_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_circuit_map_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_circuit_map_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_circuit_map_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CircuitMap COMMAND latebrake_circuit_map_tests)

add_executable(latebrake_touch_layout_tests TouchControlLayoutTests.cpp)
target_include_directories(latebrake_touch_layout_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_touch_layout_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_touch_layout_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_touch_layout_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME TouchControlLayout COMMAND latebrake_touch_layout_tests)

# Independent career rules: no Unreal SDK or licensed assets required.
foreach(name IN ITEMS CareerCatalog CareerProgress CareerChallenges MenuNavigation ProfileWriteCheckpoint TouchDriveState TouchPressTracker TouchFingerRange TouchViewportGuard TouchViewportSafety CareerLaunchSelection CareerReplayPreparation CircuitGateCrossing DecorationBatchPlan GateDiagnostic GateDiagnosticCounters GateDiagnosticWindow CircuitStartupAudit CircuitCollisionPlan CollisionBlockSetup CircuitSpawnSafety CollisionResponsePolicy)
    add_executable(latebrake_${name}_tests ${name}Tests.cpp)
    target_include_directories(latebrake_${name}_tests PRIVATE ../../Source/Latebrake/Public)
    target_compile_features(latebrake_${name}_tests PRIVATE cxx_std_20)
    if (MSVC)
        target_compile_options(latebrake_${name}_tests PRIVATE /W4 /WX)
    else()
        target_compile_options(latebrake_${name}_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME ${name} COMMAND latebrake_${name}_tests)
endforeach()


# A finger release, invalid movement or drag off GAS must drop the
# already-smoothed motor/steering command, not only its touch ownership.
add_executable(latebrake_touch_residual_safety_tests TouchResidualSafetyTests.cpp)
target_include_directories(latebrake_touch_residual_safety_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_touch_residual_safety_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_touch_residual_safety_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_touch_residual_safety_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME TouchResidualSafety COMMAND latebrake_touch_residual_safety_tests)

# A release arriving while Unreal is paused must release the physical
# finger latch before a new pause/steering/gas press can be admitted.
add_executable(latebrake_paused_touch_release_tests PausedTouchReleaseTests.cpp)
target_include_directories(latebrake_paused_touch_release_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_paused_touch_release_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_paused_touch_release_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_paused_touch_release_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME PausedTouchRelease COMMAND latebrake_paused_touch_release_tests)

# Race sector timings need the same portable RaceProgress implementation as
# the core gate/lap tests; no Unreal headers or platform SDK are required.
add_executable(latebrake_sector_timing_tests
    SectorTimingTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_sector_timing_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_sector_timing_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_sector_timing_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_sector_timing_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME SectorTiming COMMAND latebrake_sector_timing_tests)

# RecoveryPenalty uses portable RaceProgress::AddTimePenalty, not Unreal actor
# teleportation. The engine-facing movement must be tested on real devices.
add_executable(latebrake_recovery_charge_tests
    RecoveryChargeTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_recovery_charge_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_recovery_charge_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_recovery_charge_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_recovery_charge_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RecoveryCharge COMMAND latebrake_recovery_charge_tests)

# A possessed car is not playable unless its Chaos chassis mesh,
# physical collision and all motion axes are usable. Use original
# RaceProgress.cpp to reject phantom clock accumulation across transitions.
add_executable(latebrake_race_vehicle_physics_readiness_tests
    RaceVehiclePhysicsReadinessTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_race_vehicle_physics_readiness_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_race_vehicle_physics_readiness_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_race_vehicle_physics_readiness_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_race_vehicle_physics_readiness_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RaceVehiclePhysicsReadiness COMMAND latebrake_race_vehicle_physics_readiness_tests)

# Player recovery uses the same readiness as real player scoring/clock.
# Link original RaceProgress.cpp to verify clock/counter transactions rather
# than a mocked race, without claiming an Unreal game or Android QA.
add_executable(latebrake_player_recovery_readiness_tests
    PlayerRecoveryReadinessTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_player_recovery_readiness_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_player_recovery_readiness_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_player_recovery_readiness_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_player_recovery_readiness_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME PlayerRecoveryReadiness COMMAND latebrake_player_recovery_readiness_tests)

# The career garage policy test exercises *real* race completion and restart,
# so it must link the portable RaceProgress implementation, not just headers.
add_executable(latebrake_career_garage_policy_tests
    CareerGaragePolicyTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_career_garage_policy_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_career_garage_policy_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_career_garage_policy_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_career_garage_policy_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CareerGaragePolicy COMMAND latebrake_career_garage_policy_tests)

# Rejected nonfinite actor velocity is a trace discontinuity, NOT an
# acceptable baseline for the next physics frame. Exercise all four real
# ordered gates with the original RaceProgress implementation.
add_executable(latebrake_checkpoint_velocity_continuity_tests
    CheckpointVelocityContinuityTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_checkpoint_velocity_continuity_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_checkpoint_velocity_continuity_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_checkpoint_velocity_continuity_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_checkpoint_velocity_continuity_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CheckpointVelocityContinuity COMMAND latebrake_checkpoint_velocity_continuity_tests)

# Motion-epoch gate tests use actual ordered RaceProgress, not mock Unreal
# pawns. Build with the same strict flags on all three desktop CI platforms.
add_executable(latebrake_checkpoint_motion_trace_tests
    CheckpointMotionTraceTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_checkpoint_motion_trace_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_checkpoint_motion_trace_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_checkpoint_motion_trace_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_checkpoint_motion_trace_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CheckpointMotionTrace COMMAND latebrake_checkpoint_motion_trace_tests)

# v0.5.5 player-facing finish analytics read actual RaceProgress state and
# record validation without any Unreal Engine linking or test mock.
add_executable(latebrake_finished_race_insights_tests
    FinishedRaceInsightsTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_finished_race_insights_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_finished_race_insights_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_finished_race_insights_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_finished_race_insights_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME FinishedRaceInsights COMMAND latebrake_finished_race_insights_tests)

# Bounded recent-lap ring is standalone C++20 and runs unchanged in
# the actual Unreal gameplay module. No test mocks or engine headers.
add_executable(latebrake_recent_timed_laps_tests RecentTimedLapsTests.cpp)
target_include_directories(latebrake_recent_timed_laps_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_recent_timed_laps_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_recent_timed_laps_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_recent_timed_laps_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RecentTimedLaps COMMAND latebrake_recent_timed_laps_tests)



# v0.5.9: engine-facing Tick uses this actual RaceProgress policy, not
# a clock mock. Invalid startup, pause, malformed steps, finish and
# restart are exercised with the real portable racing implementation.
add_executable(latebrake_race_clock_readiness_tests
    RaceClockReadinessTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_race_clock_readiness_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_race_clock_readiness_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_race_clock_readiness_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_race_clock_readiness_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RaceClockReadiness COMMAND latebrake_race_clock_readiness_tests)

# v0.5.10: a valid local player pawn is required before a ready track can
# accumulate real race time; linked against original game RaceProgress.
add_executable(latebrake_player_race_clock_readiness_tests
    PlayerRaceClockReadinessTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_player_race_clock_readiness_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_player_race_clock_readiness_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_player_race_clock_readiness_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_player_race_clock_readiness_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME PlayerRaceClockReadiness COMMAND latebrake_player_race_clock_readiness_tests)

# v0.5.11: checkpoint samples cannot span a pause/menu session boundary.
# Exercise all four real directional gates with actual portable race progress.
add_executable(latebrake_checkpoint_session_trace_tests
    CheckpointSessionTraceTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_checkpoint_session_trace_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_checkpoint_session_trace_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_checkpoint_session_trace_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_checkpoint_session_trace_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CheckpointSessionTrace COMMAND latebrake_checkpoint_session_trace_tests)

# v0.5.15: actual gate/race tests verify that restarting or closing a
# menu can never inherit the checkpoint's pre-transition position.
add_executable(latebrake_race_session_epoch_tests
    RaceSessionEpochTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_race_session_epoch_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_race_session_epoch_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_race_session_epoch_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_race_session_epoch_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RaceSessionEpoch COMMAND latebrake_race_session_epoch_tests)

# v0.5.17: present conservative finish estimate from real timed race laps.
add_executable(latebrake_live_race_pace_tests
    LiveRacePaceProjectionTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_live_race_pace_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_live_race_pace_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_live_race_pace_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_live_race_pace_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME LiveRacePaceProjection COMMAND latebrake_live_race_pace_tests)

# v0.5.18: rival AI must respect current player/circuit racing readiness.
add_executable(latebrake_rival_drive_readiness_tests
    RivalDriveReadinessTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_rival_drive_readiness_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_drive_readiness_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_drive_readiness_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_drive_readiness_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalDriveReadiness COMMAND latebrake_rival_drive_readiness_tests)

# Live current-lap potential is an informational best-sector floor, not a
# result, award or guaranteed ghost. Test the actual RaceProgress.cpp.
add_executable(latebrake_current_lap_potential_tests
    CurrentLapPotentialTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_current_lap_potential_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_current_lap_potential_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_current_lap_potential_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_current_lap_potential_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CurrentLapPotential COMMAND latebrake_current_lap_potential_tests)

# Live sector best feedback uses the real game race implementation and never
# claims a completed result for an unfinished split.
add_executable(latebrake_live_sector_target_tests
    LiveSectorTargetTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_live_sector_target_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_live_sector_target_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_live_sector_target_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_live_sector_target_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME LiveSectorTarget COMMAND latebrake_live_sector_target_tests)

# Real player clock, checkpoint trace and rival/recovery readiness respond
# to a latched OS deactivation even if Unreal's world pause request fails.
add_executable(latebrake_app_focus_guard_tests
    AppFocusRaceGuardTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_app_focus_guard_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_app_focus_guard_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_app_focus_guard_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_app_focus_guard_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME AppFocusRaceGuard COMMAND latebrake_app_focus_guard_tests)

# v0.5.36: committed penalties must be identical in real race clock and
# bounded audit telemetry; failed charges cannot authorize recoveries.
add_executable(latebrake_time_penalty_accounting_tests
    TimePenaltyAccountingTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_time_penalty_accounting_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_time_penalty_accounting_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_time_penalty_accounting_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_time_penalty_accounting_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME TimePenaltyAccounting COMMAND latebrake_time_penalty_accounting_tests)

# v0.5.37: distinguish the current lap's charged fees from preceding laps.
add_executable(latebrake_lap_penalty_attribution_tests
    LapPenaltyAttributionTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_lap_penalty_attribution_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_lap_penalty_attribution_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_lap_penalty_attribution_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_lap_penalty_attribution_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME LapPenaltyAttribution COMMAND latebrake_lap_penalty_attribution_tests)

# v0.5.39: a failed Unreal world pause cannot authorize new driving,
# recovery, menu navigation or duplicate-finger resume via mobile touch.
add_executable(latebrake_focus_suspended_touch_policy_tests
    FocusSuspendedTouchPolicyTests.cpp
)
target_include_directories(latebrake_focus_suspended_touch_policy_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_focus_suspended_touch_policy_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_focus_suspended_touch_policy_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_focus_suspended_touch_policy_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME FocusSuspendedTouchPolicy COMMAND latebrake_focus_suspended_touch_policy_tests)

# v0.5.40: every awarded finish needs a real clock advance for EACH lap;
# recovery charges cannot substitute for active timing evidence.
add_executable(latebrake_finish_timing_eligibility_tests
    FinishTimingEligibilityTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_finish_timing_eligibility_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_finish_timing_eligibility_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_finish_timing_eligibility_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_finish_timing_eligibility_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME FinishTimingEligibility COMMAND latebrake_finish_timing_eligibility_tests)

# v0.5.42: no motor/torque while menu, finish or OS/world pause has
# frozen the race clock, even if Unreal keeps ticking after pause failure.
add_executable(latebrake_player_drive_readiness_tests
    PlayerDriveReadinessTests.cpp
)
target_include_directories(latebrake_player_drive_readiness_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_player_drive_readiness_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_player_drive_readiness_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_player_drive_readiness_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME PlayerDriveReadiness COMMAND latebrake_player_drive_readiness_tests)

# v0.5.43: standalone manual PAUSE stays authoritative when Unreal world
# pause is rejected; prevents time, ordered gates, rivals and paid recovery.
add_executable(latebrake_manual_race_pause_guard_tests
    ManualRacePauseGuardTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_manual_race_pause_guard_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_manual_race_pause_guard_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_manual_race_pause_guard_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_manual_race_pause_guard_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ManualRacePauseGuard COMMAND latebrake_manual_race_pause_guard_tests)

# v0.5.45: actual compiled production policy .cpp plus real RaceProgress.
# Fail closed if Chaos reports nonfinite angular velocity or bad body mass.
add_executable(latebrake_vehicle_dynamics_readiness_tests
    VehicleDynamicsReadinessTests.cpp
    ../../Source/Latebrake/Private/Vehicles/VehicleDynamicsReadiness.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_vehicle_dynamics_readiness_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_vehicle_dynamics_readiness_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_vehicle_dynamics_readiness_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_vehicle_dynamics_readiness_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME VehicleDynamicsReadiness COMMAND latebrake_vehicle_dynamics_readiness_tests)

# v0.5.46+v0.5.47: paid RECOVER commits a fee only after the actual
# production verifier confirms complete pose and post-move chassis motion.
add_executable(latebrake_recovery_teleport_transaction_tests
    RecoveryTeleportTransactionTests.cpp
    ../../Source/Latebrake/Private/Racing/RecoveryTeleportVerification.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_recovery_teleport_transaction_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_recovery_teleport_transaction_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_recovery_teleport_transaction_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_recovery_teleport_transaction_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RecoveryTeleportTransaction COMMAND latebrake_recovery_teleport_transaction_tests)

# v0.5.49: link the actual production teleport verifier to rival
# request/result accounting; Unreal must still be device-qualified.
add_executable(latebrake_rival_recovery_outcome_tests
    RivalRecoveryOutcomeTests.cpp
    ../../Source/Latebrake/Private/Racing/RecoveryTeleportVerification.cpp
)
target_include_directories(latebrake_rival_recovery_outcome_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_recovery_outcome_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_recovery_outcome_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_recovery_outcome_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalRecoveryOutcome COMMAND latebrake_rival_recovery_outcome_tests)

# v0.5.50: race-clock, checkpoint, recovery and motor authorization must
# fail closed until an actual Unreal RESTART placement is verified.
add_executable(latebrake_restart_placement_guard_tests
    RestartPlacementGuardTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_restart_placement_guard_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_restart_placement_guard_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_restart_placement_guard_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_restart_placement_guard_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RestartPlacementGuard COMMAND latebrake_restart_placement_guard_tests)

# v0.5.53: production handling and advisory wrong-way UI policies.
# These strict portable tests do not execute Unreal Engine, Chaos or HUD.
add_executable(latebrake_corner_traction_assist_tests CornerTractionAssistTests.cpp)
target_include_directories(latebrake_corner_traction_assist_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_corner_traction_assist_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_corner_traction_assist_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_corner_traction_assist_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CornerTractionAssist COMMAND latebrake_corner_traction_assist_tests)

add_executable(latebrake_wrong_way_indicator_tests WrongWayIndicatorTests.cpp)
target_include_directories(latebrake_wrong_way_indicator_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_wrong_way_indicator_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_wrong_way_indicator_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_wrong_way_indicator_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME WrongWayIndicator COMMAND latebrake_wrong_way_indicator_tests)

# v0.5.60: measured driving telemetry with production implementation,
# route guidance (advisory only), and earlier opponent corner braking.
add_executable(latebrake_driving_session_stats_tests
    DrivingSessionStatsTests.cpp
    ../../Source/Latebrake/Private/Racing/DrivingSessionStats.cpp
)
target_include_directories(latebrake_driving_session_stats_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_driving_session_stats_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_driving_session_stats_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_driving_session_stats_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME DrivingSessionStats COMMAND latebrake_driving_session_stats_tests)

add_executable(latebrake_circuit_route_advisor_tests
    CircuitRouteAdvisorTests.cpp
    ../../Source/Latebrake/Private/Track/CircuitRouteAdvisor.cpp
)
target_include_directories(latebrake_circuit_route_advisor_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_circuit_route_advisor_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_circuit_route_advisor_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_circuit_route_advisor_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CircuitRouteAdvisor COMMAND latebrake_circuit_route_advisor_tests)

add_executable(latebrake_rival_corner_anticipation_tests
    RivalCornerAnticipationTests.cpp
    ../../Source/Latebrake/Private/AI/RivalCornerAnticipation.cpp
)
target_include_directories(latebrake_rival_corner_anticipation_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_corner_anticipation_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_corner_anticipation_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_corner_anticipation_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalCornerAnticipation COMMAND latebrake_rival_corner_anticipation_tests)

# v0.5.61: nearest original-circuit pavement guidance and alert debounce.
# No Unreal/Chaos runtime is exercised by this portable test.
add_executable(latebrake_off_track_advisor_tests OffTrackAdvisorTests.cpp)
target_include_directories(latebrake_off_track_advisor_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_off_track_advisor_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_off_track_advisor_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_off_track_advisor_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME OffTrackAdvisor COMMAND latebrake_off_track_advisor_tests)

# v0.5.62: production radar minimap projection, true rival marker,
# heading tick and grounded direct separation (no position ranking).
add_executable(latebrake_race_minimap_tests
    RaceMiniMapTests.cpp
    ../../Source/Latebrake/Private/Track/RaceMiniMap.cpp
)
target_include_directories(latebrake_race_minimap_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_race_minimap_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_race_minimap_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_race_minimap_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RaceMiniMap COMMAND latebrake_race_minimap_tests)

# v0.5.63: real race radar proximity alert, finite sample admission,
# 15/20-meter hysteresis, 0.6-second debounce and state invalidation.
add_executable(latebrake_rival_proximity_indicator_tests
    RivalProximityIndicatorTests.cpp
    ../../Source/Latebrake/Private/Track/RaceMiniMap.cpp
)
target_include_directories(latebrake_rival_proximity_indicator_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_proximity_indicator_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_proximity_indicator_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_proximity_indicator_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalProximityIndicator COMMAND latebrake_rival_proximity_indicator_tests)

# v0.5.65: signed direct opponent separation-rate policy on real radar
# geometry. Strict, deterministic, engine-neutral, not gameplay QA.
add_executable(latebrake_rival_relative_motion_tests
    RivalRelativeMotionTests.cpp
    ../../Source/Latebrake/Private/Track/RaceMiniMap.cpp
)
target_include_directories(latebrake_rival_relative_motion_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_relative_motion_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_relative_motion_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_relative_motion_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalRelativeMotion COMMAND latebrake_rival_relative_motion_tests)

# v0.5.66: direct production HUD geometry and hit-region safety.
add_executable(latebrake_hud_vertical_plan_tests HudVerticalPlanTests.cpp)
target_include_directories(latebrake_hud_vertical_plan_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_hud_vertical_plan_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_hud_vertical_plan_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_hud_vertical_plan_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME HudVerticalPlan COMMAND latebrake_hud_vertical_plan_tests)

# v0.5.67: phone-safe text lanes and font-driven ellipsis decisions.
add_executable(latebrake_hud_text_bounds_tests HudTextBoundsTests.cpp)
target_include_directories(latebrake_hud_text_bounds_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_hud_text_bounds_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_hud_text_bounds_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_hud_text_bounds_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME HudTextBounds COMMAND latebrake_hud_text_bounds_tests)

# v0.5.68: menu-specific vertical overflow/notice geometry.
add_executable(latebrake_hud_menu_vertical_tests HudMenuVerticalPlanTests.cpp)
target_include_directories(latebrake_hud_menu_vertical_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_hud_menu_vertical_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_hud_menu_vertical_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_hud_menu_vertical_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME HudMenuVerticalPlan COMMAND latebrake_hud_menu_vertical_tests)
