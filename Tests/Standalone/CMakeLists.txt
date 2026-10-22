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
foreach(name IN ITEMS CareerCatalog CareerProgress CareerChallenges MenuNavigation ProfileWriteCheckpoint TouchDriveState TouchPressTracker TouchViewportGuard CareerLaunchSelection DecorationBatchPlan)
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
