cmake_minimum_required(VERSION 3.20)
project(balance_car_core VERSION 0.1.0 LANGUAGES CXX)

add_library(balance_car_core
    src/telemetry.cpp
    src/state_events.cpp
)
add_library(balance_car_core::balance_car_core ALIAS balance_car_core)

target_compile_features(balance_car_core PUBLIC cxx_std_23)
target_include_directories(balance_car_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_options(balance_car_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

option(BALANCE_CAR_CORE_BUILD_TESTS "Build the core unit tests" ON)
if(BALANCE_CAR_CORE_BUILD_TESTS)
    enable_testing()
    # The host build supplies GoogleTest via FetchContent; colcon on the Q6A finds the system package.
    if(NOT TARGET GTest::gtest_main)
        find_package(GTest REQUIRED)
    endif()

    add_executable(balance_car_core_tests
        test/test_telemetry.cpp
        test/test_line_buffer.cpp
        test/test_state_events.cpp
    )
    target_link_libraries(balance_car_core_tests PRIVATE
        balance_car_core::balance_car_core
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(balance_car_core_tests DISCOVERY_MODE PRE_TEST)
endif()
