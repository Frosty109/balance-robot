cmake_minimum_required(VERSION 3.20)
project(balance_car_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ament_cmake REQUIRED)
find_package(balance_car_core REQUIRED)
find_package(balance_car_interfaces REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rclcpp REQUIRED)

add_library(range_mapping STATIC src/range_mapping.cpp)
target_include_directories(range_mapping PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
# A missed RangeStatus case must fail the build, not slip through as a warning.
target_compile_options(range_mapping PRIVATE -Wall -Wextra -Werror=switch)
target_link_libraries(range_mapping PUBLIC
  balance_car_core::balance_car_core
  ${balance_car_interfaces_TARGETS}
  ${builtin_interfaces_TARGETS})

add_library(serial_link STATIC src/serial_link.cpp)
target_include_directories(serial_link PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_options(serial_link PRIVATE -Wall -Wextra)

add_executable(range_bridge src/bridge_node.cpp)
target_compile_options(range_bridge PRIVATE -Wall -Wextra)
target_link_libraries(range_bridge serial_link range_mapping rclcpp::rclcpp)
install(TARGETS range_bridge DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_range_mapping test/test_range_mapping.cpp)
  target_link_libraries(test_range_mapping range_mapping)

  ament_add_gtest(test_serial_link test/test_serial_link.cpp)
  target_link_libraries(test_serial_link serial_link)
endif()

ament_package()
