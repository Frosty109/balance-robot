# balance-car-cpp

C++23 firmware for an STM32 self-balancing robot, with hardware-independent control logic,
host-side tests and ROS 2 integration in progress.

This project rebuilds the firmware for a Yahboom STM32 self-balancing car as a way to learn
modern C++, embedded control and robot integration on real hardware. The work covers both the
software architecture and the practical job of getting a two-wheeled robot to balance, drive
and recover from disturbances.

The STM32 handles balancing, motor control, sensors and shutdown conditions. An onboard
Radxa Dragon Q6A provides Linux computing and the existing USB serial drive connection.
The next goal is to expose the robot's range readings and state through ROS 2, then add a
supervised proximity-stop behaviour.

## Current progress

As of **6 November 2026**:

- The physical car balances freely, drives forwards and backwards, and turns on command
  during supervised development runs.
- Firmware includes balance PD, velocity PI and yaw-rate control with feedforward, encoder
  feedback, explicit operator arming, angle-fault shutdown and stale-IMU shutdown.
- The serial motion link checks CRC and sequence numbers, limits requested speeds and slews
  targets. Command timeout brings motion targets towards zero while balancing continues.
- Timer-based HC-SR04 acquisition produces range readings with explicit result status and
  timestamps. Proximity stopping has not been implemented yet.
- The ROS-independent C++ core parses RANGE messages and explicit state events, and frames
  serial input with a bounded line buffer that recovers after overlong lines.

Residual creep, fore/aft cycling, hesitation and weak turn-rate tracking remain. Battery
measurement and cutoff, intermittent serial startup and other hardware questions are still
open. The simulator also needs work before it can represent the real car's behaviour.

The ROS bridge, periodic health-row parser, current-state tracker and capture replay are
still to be built. ROS installation on the Q6A has not yet been verified.

See [STATUS.md](STATUS.md) for the current source/image distinction, physical-test evidence
and open risks. Some supporting lab notes and captures referenced there are kept locally
and are not included in this repository.

## Why C++?

The aim is to make the control logic understandable and testable while keeping hardware
details at the edges. Controllers and application logic use hardware-independent interfaces;
STM32 adapters implement those interfaces using board-support classes. Retained C drivers
provide the vendor-library and sensor boundaries.

This lets the same control classes run in host tests and on the microcontroller. It also lets
the new serial parsing code be tested on a PC before it is connected to ROS or the car.

The project uses C++23. The bare-metal firmware builds with exceptions and RTTI disabled.
The bridge core uses value types, `std::string_view` and `std::expected` for explicit parsing
results, with no ROS or firmware HAL dependency.

## Hardware

The development car uses:

- Yahboom YB-EST01-V2.0 board with an STM32F103RCT6 microcontroller.
- MPU6050 IMU with retained DMP support.
- Two encoder-equipped motors and a 2S battery pack.
- HC-SR04 ultrasonic range sensor.
- USB serial connection for telemetry and commands.
- Radxa Dragon Q6A mounted onboard, with its own power bank.

The Q6A's existing role is serial driving and flashing through local host tools. ROS nodes
will run on the Q6A; the STM32 will continue to own the balance loop and motor control.

## Repository layout

| Path | Responsibility |
|---|---|
| [`src/pid/`](src/pid/) | Platform-independent balance, velocity and yaw controllers |
| [`src/app/`](src/app/) | Control orchestration, arming/fault gates and the motion-command link |
| [`hal/interface/`](hal/interface/) | Hardware-independent sensor, motor and clock interfaces |
| [`hal/stm32/`](hal/stm32/) | STM32 implementations of those interfaces |
| [`hal/sim/`](hal/sim/) | Simulator implementations |
| [`bsp/`](bsp/) | C++ board support for motors, encoders, sensors, serial and timers |
| [`stm32/`](stm32/) | Firmware entry point, startup/linker configuration and retained C libraries/drivers |
| [`tests/`](tests/) | Host tests for controllers and application behaviour |
| [`simulation/`](simulation/) | Early simulator; currently unrepresentative of the physical car |
| [`ros2/src/balance_car_core/`](ros2/src/balance_car_core/) | ROS-independent serial framing and parsing, with host tests |
| [`reviews/`](reviews/) | Project review findings |

The firmware dependency direction is:

```text
Application and controllers
          |
Hardware-independent interfaces
          |
     STM32 adapters
          |
    C++ board support
          |
Retained C drivers, ST FWLib and CMSIS
```

ROS types stay outside the portable core. The planned bridge will translate between the
serial protocol and ROS messages.

## Build and test on a PC

Requirements:

- CMake 3.20 or newer.
- A C++23 compiler and standard library with `std::expected` support.
- A supported build tool, such as Ninja or Make.
- Internet access for the initial GoogleTest download through CMake `FetchContent`.

Windows development has been checked with MinGW GCC 16.1.0.

```sh
git clone https://github.com/Frosty109/balance-robot.git
cd balance-robot
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run CTest after a successful build. The host build includes the simulator, application tests
and ROS-independent core tests; ROS is not required.

On the development Windows setup, MinGW needs an explicit CA bundle for the GoogleTest
download. From the project root in PowerShell:

```powershell
$env:SSL_CERT_FILE = 'C:\mingw64\bin\curl-ca-bundle.crt'
cmake -B build -DCMAKE_TLS_CAINFO=C:/mingw64/bin/curl-ca-bundle.crt
cmake --build build
ctest --test-dir build --output-on-failure
```

Adjust the certificate path if MinGW is installed elsewhere.

An Ubuntu 24.04 development container is also provided. This command uses a separate build
directory so it does not reuse a Windows CMake cache:

```sh
docker compose run --rm build bash -lc "cmake -S . -B build-docker && cmake --build build-docker && ctest --test-dir build-docker --output-on-failure"
```

The latest recorded 141-test result is from Windows; a fresh container run is a separate check.

## Build the STM32 firmware

Install the Arm GNU toolchain with C++23 support and put `arm-none-eabi-gcc`,
`arm-none-eabi-g++` and the associated binutils on your PATH. Then run:

```sh
cmake -S . -B build-fw-stage1-clean -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
cmake --build build-fw-stage1-clean
```

The cross build includes the STM32 target and skips the host simulator, GoogleTest and ROS
core. Outputs are written under `build-fw-stage1-clean/stm32/`:

- `firmware.elf`
- `firmware.hex`
- `firmware.bin`

With a correctly connected ST-Link, OpenOCD installed and the car disarmed or powered safely,
the existing flash target rebuilds, programs, verifies and resets:

```sh
cmake --build build-fw-stage1-clean --target flash
```

The optional Q6A serial drive/flash helpers are currently local development tools and are not
included in this checkout. See [STATUS.md](STATUS.md) for their current workflow limitations.

## Operating the physical car

**Disarm and see `DISARMED` before lifting, carrying, repositioning or touching the wheels.**
Send a space and read `DISARMED reason=operator` back before handling. If the serial link
stops responding while armed, cut power before approaching the car.

- The firmware starts disarmed. Only an explicit `a` can arm the motors when the gates permit it.
- A fault disarms the car. Recovery does not restart the motors; it needs another explicit arm.
- Motion-command timeout reduces targets to zero while balance remains active. It is not an
  operator-presence check or a motor-off acknowledgement.
- Battery cutoff is disabled. Use supervised runs only.
- Build/test success alone does not establish physical behaviour or stopping clearance.

## Next milestones

1. Parse periodic armed/disarmed health telemetry and track confirmed robot state.
2. Replay saved captures to exercise framing, parsing, faults, freshness and restart handling.
3. Verify the Q6A environment and publish read-only range/state topics through a ROS 2 bridge.
4. Add bounded motion commands, explicit arm/disarm handling and an operator run lease.
5. Demonstrate a supervised proximity stop with measured physical behaviour.

Later work includes camera integration, odometry and bounded moves, a reporting dashboard,
and experiments with voice and model-assisted behaviour. These are future directions.

## Project licence

The repository does not currently include a project-wide licence file. The ROS core package
still has a `TODO` licence entry. Retained third-party sources carry their existing notices;
a project licence has not been selected as part of this work.
