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

# Actual shared player/rival arcade brake thermal response, independent
# of Unreal Engine runtime and vehicle physics packaging.
add_executable(latebrake_arcade_brake_heat_tests ArcadeBrakeHeatTests.cpp)
target_include_directories(latebrake_arcade_brake_heat_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_arcade_brake_heat_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_arcade_brake_heat_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_arcade_brake_heat_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ArcadeBrakeHeat COMMAND latebrake_arcade_brake_heat_tests)

# Race-local hard wall/vehicle contacts; conservative bounded player/rival
# damage arithmetic runs in C++20 without licensed Unreal components.
add_executable(latebrake_arcade_collision_damage_tests ArcadeCollisionDamageTests.cpp)
target_include_directories(latebrake_arcade_collision_damage_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_arcade_collision_damage_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_arcade_collision_damage_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_arcade_collision_damage_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ArcadeCollisionDamage COMMAND latebrake_arcade_collision_damage_tests)

# Autonomous rival pace authority grounded in previously sampled tire
# grip and actual lateral slip, independent of Unreal build/runtime.
add_executable(latebrake_rival_grip_pace_tests RivalGripPaceTests.cpp)
target_include_directories(latebrake_rival_grip_pace_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_grip_pace_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_grip_pace_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_grip_pace_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalGripPace COMMAND latebrake_rival_grip_pace_tests)

# Test the actual composition order of route/grip/traffic/brake-hold
# policies with safe numerical fixtures; no Unreal runtime required.
add_executable(latebrake_rival_racecraft_composition_tests RivalRacecraftCompositionTests.cpp)
target_include_directories(latebrake_rival_racecraft_composition_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_racecraft_composition_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_racecraft_composition_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_racecraft_composition_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalRacecraftComposition COMMAND latebrake_rival_racecraft_composition_tests)

# Rival physical spin response: only bounded propulsion reduction based on
# real chassis yaw/lateral slip; no player or Unreal runtime dependencies.
add_executable(latebrake_rival_spin_throttle_tests RivalSpinThrottleGuardTests.cpp)
target_include_directories(latebrake_rival_spin_throttle_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_spin_throttle_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_spin_throttle_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_spin_throttle_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalSpinThrottleGuard COMMAND latebrake_rival_spin_throttle_tests)

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


# Bounded, engine-independent race-session tire wear and heat policy.
add_executable(latebrake_arcade_tire_tests ArcadeTireConditionTests.cpp)
target_include_directories(latebrake_arcade_tire_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_arcade_tire_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_arcade_tire_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_arcade_tire_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ArcadeTireCondition COMMAND latebrake_arcade_tire_tests)

# Observed actual XY circuit pavement; affects existing rigid-body
# motor and lateral damping only. Never a replacement for UE physics.
add_executable(latebrake_arcade_surface_traction_tests ArcadeSurfaceTractionTests.cpp)
target_include_directories(latebrake_arcade_surface_traction_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_arcade_surface_traction_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_arcade_surface_traction_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_arcade_surface_traction_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ArcadeSurfaceTraction COMMAND latebrake_arcade_surface_traction_tests)

# Strict, engine-independent rechargeable nitro/heat/physics-safety policy.
add_executable(latebrake_arcade_nitro_tests ArcadeNitroBoostTests.cpp)
target_include_directories(latebrake_arcade_nitro_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_arcade_nitro_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_arcade_nitro_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_arcade_nitro_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ArcadeNitroBoost COMMAND latebrake_arcade_nitro_tests)

# Geometrically qualified single-rival drafting boost: no Unreal SDK.
add_executable(latebrake_slipstream_tests SlipstreamDraftBoostTests.cpp)
target_include_directories(latebrake_slipstream_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_slipstream_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_slipstream_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_slipstream_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME SlipstreamDraftBoost COMMAND latebrake_slipstream_tests)

# Shared nitro/slipstream additive force budget with invalid-input gates.
add_executable(latebrake_boost_arbitration_tests ArcadeBoostArbitrationTests.cpp)
target_include_directories(latebrake_boost_arbitration_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_boost_arbitration_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_boost_arbitration_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_boost_arbitration_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ArcadeBoostArbitration COMMAND latebrake_boost_arbitration_tests)

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

# v0.5.69: bounded Results/Career/Garage menu detail pagination.
add_executable(latebrake_menu_detail_pager_tests MenuDetailPagerTests.cpp)
target_include_directories(latebrake_menu_detail_pager_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_menu_detail_pager_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_menu_detail_pager_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_menu_detail_pager_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME MenuDetailPager COMMAND latebrake_menu_detail_pager_tests)

# v0.5.70: genuine finished-run lap review using production
# RaceProgress and eight-record ring. Results presentation only.
add_executable(latebrake_recent_lap_review_tests
    RecentLapReviewTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_recent_lap_review_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_recent_lap_review_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_recent_lap_review_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_recent_lap_review_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RecentLapReview COMMAND latebrake_recent_lap_review_tests)

# v0.5.71: production-linked, engine-neutral Results insight policy.
add_executable(latebrake_recent_lap_pace_statistics_tests
    RecentLapPaceStatisticsTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_recent_lap_pace_statistics_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_recent_lap_pace_statistics_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_recent_lap_pace_statistics_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_recent_lap_pace_statistics_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RecentLapPaceStatistics COMMAND latebrake_recent_lap_pace_statistics_tests)

# v0.5.71: production-linked, engine-neutral Results insight policy.
add_executable(latebrake_best_sector_review_tests
    BestSectorReviewTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_best_sector_review_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_best_sector_review_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_best_sector_review_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_best_sector_review_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME BestSectorReview COMMAND latebrake_best_sector_review_tests)

# v0.5.71: production-linked, engine-neutral Results insight policy.
add_executable(latebrake_sampled_driving_review_tests
    SampledDrivingReviewTests.cpp
    ../../Source/Latebrake/Private/Racing/DrivingSessionStats.cpp
)
target_include_directories(latebrake_sampled_driving_review_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_sampled_driving_review_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_sampled_driving_review_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_sampled_driving_review_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME SampledDrivingReview COMMAND latebrake_sampled_driving_review_tests)

# v0.5.72: production camera framing policy remains Unreal-free and strict.
add_executable(latebrake_camera_presets_tests CameraPresetsTests.cpp)
target_include_directories(latebrake_camera_presets_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_camera_presets_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_camera_presets_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_camera_presets_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CameraPresets COMMAND latebrake_camera_presets_tests)

# v0.5.72: real disjoint touch geometry and one-shot CAM admission.
add_executable(latebrake_touch_camera_cycle_policy_tests TouchCameraCyclePolicyTests.cpp)
target_include_directories(latebrake_touch_camera_cycle_policy_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_touch_camera_cycle_policy_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_touch_camera_cycle_policy_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_touch_camera_cycle_policy_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME TouchCameraCyclePolicy COMMAND latebrake_touch_camera_cycle_policy_tests)

# v0.5.73: source-observed arcade drift meter, never an award/record.
add_executable(latebrake_drift_combo_meter_tests DriftComboMeterTests.cpp)
target_include_directories(latebrake_drift_combo_meter_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_drift_combo_meter_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_drift_combo_meter_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_drift_combo_meter_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME DriftComboMeter COMMAND latebrake_drift_combo_meter_tests)

# v0.5.74: player-axis noise gates are independent of Unreal and rivals.
add_executable(latebrake_drive_axis_deadzone_tests DriveAxisDeadzoneTests.cpp)
target_include_directories(latebrake_drive_axis_deadzone_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_drive_axis_deadzone_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_drive_axis_deadzone_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_drive_axis_deadzone_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME DriveAxisDeadzone COMMAND latebrake_drive_axis_deadzone_tests)

# v0.5.75: exact frame-boundary race clock/start-gate source contract.
add_executable(latebrake_race_start_lights_tests
    RaceStartLightsTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_race_start_lights_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_race_start_lights_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_race_start_lights_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_race_start_lights_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RaceStartLights COMMAND latebrake_race_start_lights_tests)

# Next source candidate: honest per-run forward launch speed estimate.
add_executable(latebrake_launch_speed_meter_tests
    LaunchSpeedMeterTests.cpp
    ../../Source/Latebrake/Private/Racing/RaceProgress.cpp
)
target_include_directories(latebrake_launch_speed_meter_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_launch_speed_meter_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_launch_speed_meter_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_launch_speed_meter_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME LaunchSpeedMeter COMMAND latebrake_launch_speed_meter_tests)

# Optional actual world-velocity G-force telemetry; no Unreal dependency.
add_executable(latebrake_vehicle_g_force_meter_tests
    VehicleGForceMeterTests.cpp
)
target_include_directories(latebrake_vehicle_g_force_meter_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_vehicle_g_force_meter_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_vehicle_g_force_meter_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_vehicle_g_force_meter_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME VehicleGForceMeter COMMAND latebrake_vehicle_g_force_meter_tests)

# Decorative, non-physical wheel rotation/steering from genuine car velocity.
add_executable(latebrake_wheel_visual_motion_tests WheelVisualMotionTests.cpp)
target_include_directories(latebrake_wheel_visual_motion_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_wheel_visual_motion_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_wheel_visual_motion_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_wheel_visual_motion_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME WheelVisualMotion COMMAND latebrake_wheel_visual_motion_tests)

# Physical player-aware AI caution: no Unreal SDK, no race placement changes.
add_executable(latebrake_rival_traffic_awareness_tests RivalTrafficAwarenessTests.cpp)
target_include_directories(latebrake_rival_traffic_awareness_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_traffic_awareness_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_traffic_awareness_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_traffic_awareness_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalTrafficAwareness COMMAND latebrake_rival_traffic_awareness_tests)

# A short post-emergency brake hold for the single offline prototype rival.
# No Unreal, network, physics force or results dependencies.
add_executable(latebrake_rival_following_brake_hold_tests
    RivalFollowingBrakeHoldTests.cpp)
target_include_directories(latebrake_rival_following_brake_hold_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_following_brake_hold_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_following_brake_hold_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_following_brake_hold_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalFollowingBrakeHold COMMAND latebrake_rival_following_brake_hold_tests)

# Optional brake/reverse visual signals, not vehicle braking physics.
add_executable(latebrake_vehicle_signal_lights_tests VehicleSignalLightsTests.cpp)
target_include_directories(latebrake_vehicle_signal_lights_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_vehicle_signal_lights_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_vehicle_signal_lights_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_vehicle_signal_lights_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME VehicleSignalLights COMMAND latebrake_vehicle_signal_lights_tests)

# Observed best-lap ghost poses: bounded source-only run-local visual trace.
add_executable(latebrake_best_lap_ghost_tests BestLapGhostTests.cpp)
target_include_directories(latebrake_best_lap_ghost_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_best_lap_ghost_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_best_lap_ghost_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_best_lap_ghost_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME BestLapGhost COMMAND latebrake_best_lap_ghost_tests)

# Player-only display preference. Ghost recording, official race timing and
# saved progression remain unchanged when a visual replay is hidden.
add_executable(latebrake_ghost_display_preference_tests GhostDisplayPreferenceTests.cpp)
target_include_directories(latebrake_ghost_display_preference_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_ghost_display_preference_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_ghost_display_preference_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_ghost_display_preference_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME GhostDisplayPreference COMMAND latebrake_ghost_display_preference_tests)

# Android-only ghost visibility action: exact shared touch rectangle and
# authoritative eligibility. No engine headers or game packages required.
add_executable(latebrake_ghost_touch_policy_tests GhostTouchPolicyTests.cpp)
target_include_directories(latebrake_ghost_touch_policy_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_ghost_touch_policy_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_ghost_touch_policy_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_ghost_touch_policy_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME GhostTouchPolicy COMMAND latebrake_ghost_touch_policy_tests)

# Read-only separation between real car and same-lap-time ghost position.
# No signed race position, physics forces, timing or awards involved.
add_executable(latebrake_ghost_separation_tests GhostSeparationTests.cpp)
target_include_directories(latebrake_ghost_separation_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_ghost_separation_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_ghost_separation_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_ghost_separation_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME GhostSeparation COMMAND latebrake_ghost_separation_tests)

# Read-only rigid-body travel direction relative to the car heading.
add_executable(latebrake_chassis_slip_angle_meter_tests
    ChassisSlipAngleMeterTests.cpp)
target_include_directories(latebrake_chassis_slip_angle_meter_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_chassis_slip_angle_meter_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_chassis_slip_angle_meter_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_chassis_slip_angle_meter_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ChassisSlipAngleMeter COMMAND latebrake_chassis_slip_angle_meter_tests)

# Source-only chassis lag-spike resistance shared by player and rival pawn.
add_executable(latebrake_frame_stable_damping_tests FrameStableDampingTests.cpp)
target_include_directories(latebrake_frame_stable_damping_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_frame_stable_damping_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_frame_stable_damping_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_frame_stable_damping_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME FrameStableDamping COMMAND latebrake_frame_stable_damping_tests)

# Held, temporary human rear-view; the base chase preset remains unchanged.
add_executable(latebrake_held_look_back_camera_tests HeldLookBackCameraTests.cpp)
target_include_directories(latebrake_held_look_back_camera_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_held_look_back_camera_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_held_look_back_camera_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_held_look_back_camera_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME HeldLookBackCamera COMMAND latebrake_held_look_back_camera_tests)

# Real forward chassis speed affects human camera optics only, never handling.
add_executable(latebrake_speed_reactive_fov_tests SpeedReactiveCameraFovTests.cpp)
target_include_directories(latebrake_speed_reactive_fov_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_speed_reactive_fov_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_speed_reactive_fov_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_speed_reactive_fov_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME SpeedReactiveCameraFov COMMAND latebrake_speed_reactive_fov_tests)

# Source-only camera look-ahead: human chase yaw is a bounded visual
# response to measured signed forward speed and filtered steering.
# This does not qualify UE camera animation or cooked gameplay.
add_executable(latebrake_corner_look_ahead_camera_tests
    CornerLookAheadCameraTests.cpp)
target_include_directories(latebrake_corner_look_ahead_camera_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_corner_look_ahead_camera_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_corner_look_ahead_camera_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_corner_look_ahead_camera_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME CornerLookAheadCamera COMMAND
    latebrake_corner_look_ahead_camera_tests)

# Source-only human handbrake policy; no claims about tire/suspension simulation.
add_executable(latebrake_handbrake_drive_tests HandbrakeDrivePolicyTests.cpp)
target_include_directories(latebrake_handbrake_drive_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_handbrake_drive_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_handbrake_drive_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_handbrake_drive_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME HandbrakeDrivePolicy COMMAND latebrake_handbrake_drive_tests)

# Current-rate projected stop distance is observational only (no UE runtime).
add_executable(latebrake_observed_brake_distance_tests ObservedBrakeDistanceTests.cpp)
target_include_directories(latebrake_observed_brake_distance_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_observed_brake_distance_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_observed_brake_distance_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_observed_brake_distance_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ObservedBrakeDistance COMMAND latebrake_observed_brake_distance_tests)

# Combined production arcade force policies: on-road/off-road,
# impact, tires, thermal braking, corner assist, nitro and drafting.
# Real UHT/Chaos and installed game QA are separate mandatory gates.
add_executable(latebrake_arcade_drive_composition_tests
    ArcadeDriveCompositionTests.cpp)
target_include_directories(latebrake_arcade_drive_composition_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_arcade_drive_composition_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_arcade_drive_composition_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_arcade_drive_composition_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ArcadeDriveComposition COMMAND latebrake_arcade_drive_composition_tests)

# Prototype reverse gear: constrain only signed backwards chassis motion,
# preserve lateral slides and forward/nitro speed, no Unreal build required.
add_executable(latebrake_arcade_reverse_speed_tests ArcadeReverseSpeedLimitTests.cpp)
target_include_directories(latebrake_arcade_reverse_speed_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_arcade_reverse_speed_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_arcade_reverse_speed_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_arcade_reverse_speed_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ArcadeReverseSpeedLimit COMMAND latebrake_arcade_reverse_speed_tests)

# Actual rigid-body motor power tapers near gear-specific measured speed caps.
# Shared by normal propulsion, nitro and drafting; no Unreal game runtime.
add_executable(latebrake_gear_aware_motor_power_tests GearAwareMotorPowerTests.cpp)
target_include_directories(latebrake_gear_aware_motor_power_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_gear_aware_motor_power_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_gear_aware_motor_power_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_gear_aware_motor_power_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME GearAwareMotorPower COMMAND latebrake_gear_aware_motor_power_tests)

# Rival reacts to measured, confirmed dirt and previous accepted chassis
# damage without steering changes, free acceleration or race authority.
add_executable(latebrake_rival_surface_damage_pace_tests
    RivalSurfaceDamagePaceTests.cpp)
target_include_directories(latebrake_rival_surface_damage_pace_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_surface_damage_pace_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_surface_damage_pace_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_surface_damage_pace_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalSurfaceDamagePace COMMAND
    latebrake_rival_surface_damage_pace_tests)

# Recover a fast-sliding or circling physical rival that makes no meaningful
# progress toward its actual next waypoint, without changing lap authority.
add_executable(latebrake_rival_no_approach_recovery_tests
    RivalNoApproachRecoveryTests.cpp)
target_include_directories(latebrake_rival_no_approach_recovery_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_no_approach_recovery_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_no_approach_recovery_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_no_approach_recovery_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalNoApproachRecovery COMMAND
    latebrake_rival_no_approach_recovery_tests)

# Measured Chaos yaw rate damps same-direction waypoint steering without
# touching rival throttle, braking, route progress or player handling.
add_executable(latebrake_rival_yaw_rate_steering_tests
    RivalYawRateSteeringTests.cpp)
target_include_directories(latebrake_rival_yaw_rate_steering_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_yaw_rate_steering_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_yaw_rate_steering_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_yaw_rate_steering_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalYawRateSteering COMMAND
    latebrake_rival_yaw_rate_steering_tests)

# Real rival waypoint advancement shares the player's 3D directional
# checkpoint geometry rather than awarding a proximity-only sphere visit.
add_executable(latebrake_rival_directional_gate_tests
    RivalDirectionalGateTests.cpp)
target_include_directories(latebrake_rival_directional_gate_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_directional_gate_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_directional_gate_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_directional_gate_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalDirectionalGate COMMAND
    latebrake_rival_directional_gate_tests)

# The rival may only finish a checkpoint after a forward plane crossing,
# never when its generic waypoint planner reaches a radial stop distance.
add_executable(latebrake_rival_gate_run_through_tests
    RivalGateRunThroughTests.cpp)
target_include_directories(latebrake_rival_gate_run_through_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_gate_run_through_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_gate_run_through_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_gate_run_through_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalGateRunThrough COMMAND
    latebrake_rival_gate_run_through_tests)

# Arcade-only local human traction assist when real Chaos yaw and lateral
# velocity jointly identify a high-speed spin; no Unreal/device build here.
add_executable(latebrake_human_spin_traction_tests
    HumanSpinTractionControlTests.cpp)
target_include_directories(latebrake_human_spin_traction_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_human_spin_traction_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_human_spin_traction_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_human_spin_traction_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME HumanSpinTractionControl COMMAND
    latebrake_human_spin_traction_tests)

# Read-only physical spin-assist display model: never changes engine force,
# result timing or player economy; tested with 200k deterministic frames.
add_executable(latebrake_human_spin_assist_meter_tests
    HumanSpinAssistMeterTests.cpp)
target_include_directories(latebrake_human_spin_assist_meter_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_human_spin_assist_meter_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_human_spin_assist_meter_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_human_spin_assist_meter_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME HumanSpinAssistMeter COMMAND latebrake_human_spin_assist_meter_tests)

# Advisory only: compare player and rival accepted directional checkpoint
# totals, never direct distance/ranking/finish timing or career rewards.
add_executable(latebrake_rival_gate_gap_tests RivalGateGapTests.cpp)
target_include_directories(latebrake_rival_gate_gap_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_gate_gap_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_gate_gap_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_gate_gap_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalGateGap COMMAND latebrake_rival_gate_gap_tests)

# Shared player/rival engine thermal state: strict portable C++20 QA.
add_executable(latebrake_arcade_engine_heat_tests ArcadeEngineHeatTests.cpp)
target_include_directories(latebrake_arcade_engine_heat_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_arcade_engine_heat_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_arcade_engine_heat_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_arcade_engine_heat_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME ArcadeEngineHeat COMMAND latebrake_arcade_engine_heat_tests)

# Real rival AI reads the prior qualified engine heat meter and may only
# ease its throttle, not override route/traffic brakes or steering.
add_executable(latebrake_rival_engine_heat_pace_tests
    RivalEngineHeatPaceTests.cpp)
target_include_directories(latebrake_rival_engine_heat_pace_tests PRIVATE
    ../../Source/Latebrake/Public)
target_compile_features(latebrake_rival_engine_heat_pace_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_rival_engine_heat_pace_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_rival_engine_heat_pace_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME RivalEngineHeatPace COMMAND
    latebrake_rival_engine_heat_pace_tests)

# Desktop visual race instruments: pure viewport and meter policy.
# This test does not claim rendered Unreal/Android QA.
add_executable(latebrake_hud_instrument_tests HudInstrumentClusterTests.cpp)
target_include_directories(latebrake_hud_instrument_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_hud_instrument_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_hud_instrument_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_hud_instrument_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME HudInstrumentCluster COMMAND latebrake_hud_instrument_tests)

# Decorative garage body variants are pure source policy (not Unreal art QA).
add_executable(latebrake_vehicle_visual_recipe_tests VehicleVisualRecipeTests.cpp)
target_include_directories(latebrake_vehicle_visual_recipe_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_vehicle_visual_recipe_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_vehicle_visual_recipe_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_vehicle_visual_recipe_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME VehicleVisualRecipe COMMAND latebrake_vehicle_visual_recipe_tests)

# Desktop-only Canvas menu buttons share one pure geometry/hit-test policy.
add_executable(latebrake_menu_mouse_tests HudMenuMouseControlsTests.cpp)
target_include_directories(latebrake_menu_mouse_tests PRIVATE ../../Source/Latebrake/Public)
target_compile_features(latebrake_menu_mouse_tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(latebrake_menu_mouse_tests PRIVATE /W4 /WX)
else()
    target_compile_options(latebrake_menu_mouse_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME HudMenuMouseControls COMMAND latebrake_menu_mouse_tests)
