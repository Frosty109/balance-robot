# Balance car ROS 2 workspace

The first target is a read-only `/car/range` bridge. The parsers exist; ROS nodes and live
serial publication do not yet exist. Full state tracking and replay are deferred until command
support. Source is user-authored. See `../STATUS.md` and the integration plan in `../plans/`.

## Q6A environment verified 2026-11-07

- Ubuntu 24.04.4 LTS, aarch64, UTF-8 locale.
- ROS Jazzy Base under `/opt/ros/jazzy`; `rclcpp` is discoverable after sourcing its setup.
- colcon at `/usr/bin/colcon`, GCC 13.3.0, CMake 3.28.3, Ninja, GoogleTest 1.14.0.
- The operator installed ROS and the development tools interactively with sudo.

The core package is copied to the Q6A under `~/balance-car/ros2/src/balance_car_core/`.
Its C++23 build passed and its **52/52 core tests
passed**, zero failures, in **0.29 s**. This is not the full Windows host suite (148 tests).
Evidence: `../sessions/artefacts/2026-11-07/q6a-core-ctest-lasttest.log`.

To repeat that standalone core build on the Q6A:

```bash
cmake -S ~/balance-car/ros2/src/balance_car_core -B ~/balance-car/ros2/build-core -G Ninja
cmake --build ~/balance-car/ros2/build-core --parallel 2
ctest --test-dir ~/balance-car/ros2/build-core --output-on-failure
```

Install/export rules in `src/balance_car_core/CMakeLists.txt` are saved and verified on the Q6A:
colcon build/install passes, all 52 tests pass, and CMake finds the installed core package.
The bridge can use `find_package(balance_car_core REQUIRED)` and link
`balance_car_core::balance_car_core`. CTest currently emits a non-fatal missing
`DartConfiguration.tcl` warning during colcon test; it does not prevent the tests running.
Evidence: `../sessions/artefacts/2026-11-07/q6a-core-colcon-test.log`.

On the Q6A, from `~/balance-car/ros2`:

```bash
source /opt/ros/jazzy/setup.bash
colcon build --base-paths src --packages-select balance_car_core
colcon test --base-paths src --packages-select balance_car_core --return-code-on-test-failure
colcon test-result --verbose
source install/setup.bash
```

Next: the range message interface and read-only publisher. No ROS dependency is added to the core.

No serial port was opened during setup or these tests. Live serial observation is a separate
disarmed step, with exclusive port ownership and a no-reset-on-open check.
