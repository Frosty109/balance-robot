cmake_minimum_required(VERSION 3.20)
project(balance_car_cpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable (sim
    src/pid/pid_control.cpp
    src/app/app_control.cpp
    hal/sim/sim_sensor_hal.cpp
    hal/sim/sim_motor_hal.cpp
    simulation/physics.cpp
    simulation/sim_main.cpp
)
target_include_directories(sim PRIVATE .)

enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(balance_tests
    src/pid/pid_control.cpp
    tests/test_balance_pd.cpp
    tests/test_velocity_pi.cpp
    tests/test_turn_pd.cpp
)
target_link_libraries(balance_tests PRIVATE GTest::gtest_main)
target_include_directories(balance_tests PRIVATE .)

include(GoogleTest)
gtest_discover_tests(balance_tests DISCOVERY_MODE PRE_TEST)
