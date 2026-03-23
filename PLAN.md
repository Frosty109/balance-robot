# balance-car-cpp — Project Plan

## What This Is

A C++ rewrite of the Yahboom Self-Balancing Car STM32 project, built alongside the
PID course (lessons 08–13 and beyond). Two goals run in parallel:

1. **Learn C++** — by translating known C code into proper C++ classes
2. **Build a professional embedded software base** — clean architecture that can
   grow with future lessons, support PC simulation, and eventually run on the real robot

The source of truth for the C implementation is:
`Q:\mega_documents\Documents\Robotics\Yahboom Self Balancing Car\My Code\MY_3.PID_Course\3.PID_Course\08-13.Balanced_Car_PID`

---

## Repository Structure

```
balance-car-cpp/
├── PLAN.md                         ← this file
├── CMakeLists.txt                  ← top-level CMake (builds sim + tests)
├── Dockerfile                      ← g++, CMake, Google Test
├── docker-compose.yml              ← `docker compose run build`
├── .github/
│   └── workflows/
│       └── ci.yml                  ← build + test on every push
│
├── src/
│   ├── pid/
│   │   ├── pid_control.hpp         ← BalancePD, VelocityPI, TurnPD classes
│   │   └── pid_control.cpp
│   └── app/
│       ├── app_control.hpp         ← application logic (angle select, motor combine)
│       └── app_control.cpp
│
├── hal/
│   ├── interface/
│   │   ├── i_sensor_hal.hpp        ← pure virtual: getAngle(), getEncoders(), etc.
│   │   └── i_motor_hal.hpp         ← pure virtual: setMotorPWM()
│   ├── stm32/
│   │   ├── stm32_sensor_hal.hpp    ← real MPU6050 + encoder reads via BSP
│   │   ├── stm32_sensor_hal.cpp
│   │   ├── stm32_motor_hal.hpp     ← real PWM via BSP
│   │   └── stm32_motor_hal.cpp
│   └── sim/
│       ├── sim_sensor_hal.hpp      ← returns values from physics model
│       ├── sim_sensor_hal.cpp
│       ├── sim_motor_hal.hpp       ← feeds PWM into physics model
│       └── sim_motor_hal.cpp
│
├── simulation/
│   ├── physics.hpp                 ← simple inverted pendulum model
│   ├── physics.cpp
│   └── sim_main.cpp                ← PC entry point, CSV output → Serial Studio
│
├── tests/
│   ├── CMakeLists.txt
│   ├── test_balance_pd.cpp         ← unit tests for BalancePD
│   ├── test_velocity_pi.cpp        ← unit tests for VelocityPI
│   └── test_turn_pd.cpp            ← unit tests for TurnPD
│
├── stm32/                          ← Keil MDK5 project (STM32 target)
│   ├── BSP/                        ← copied/symlinked from Yahboom project
│   ├── CMSIS/
│   ├── FWLib/
│   └── USER/
│       ├── main.cpp                ← C++ entry point for STM32
│       └── stm32f10x_it.cpp
│
└── Learning-Journal/
    ├── CPP01-classes-and-pid.md    ← C++ classes, replacing static with members
    ├── CPP02-hal-pattern.md        ← hardware abstraction, pure virtual, polymorphism
    ├── CPP03-cmake-and-docker.md   ← build systems, containers
    └── CPP04-simulation.md         ← physics model, CSV output, Serial Studio
```

---

## Two Build Targets

### Target 1 — PC Simulation (Docker / CMake)
- Compiles `src/pid/`, `src/app/`, `hal/sim/`, `simulation/`
- Runs a fake physics loop, outputs CSV to stdout
- Pipe into Serial Studio for live plotting (same format as STM32 printf)
- Build command: `docker compose run build`

### Target 2 — STM32 Firmware (Keil MDK5, Windows)
- Compiles `src/pid/`, `src/app/`, `hal/stm32/`, `stm32/`
- Links against BSP, CMSIS, FWLib (unchanged C drivers)
- Flash with ST-Link SWD as usual

The PID classes in `src/pid/` are shared — identical code, different HAL underneath.

---

## Key C++ Concepts Introduced Per Phase

| Phase | Files | Concepts |
|---|---|---|
| 1 | `pid_control.hpp/cpp` | Classes, constructors, private members replacing `static` |
| 2 | `hal/interface/` | Pure virtual functions, abstract base classes, polymorphism |
| 3 | `hal/stm32/`, `hal/sim/` | Inheritance, object composition, dependency injection |
| 4 | `simulation/` | Separate compilation, numerical integration, `std::chrono` |
| 5 | `tests/` | Google Test, test fixtures, assertions |
| 6 | `CMakeLists.txt` | CMake targets, linking, include paths |
| 7 | `Dockerfile` | Containers, reproducible builds |
| 8 | `.github/workflows/` | CI/CD, automated testing |

---

## Implementation Order

1. `src/pid/pid_control.hpp` — class definitions
2. `src/pid/pid_control.cpp` — implementations (direct translation from C)
3. `hal/interface/` — abstract sensor and motor interfaces
4. `hal/sim/` — simulation implementations
5. `simulation/physics.cpp` + `sim_main.cpp` — PC runnable
6. `CMakeLists.txt` — build system for sim + tests
7. `Dockerfile` + `docker-compose.yml`
8. `hal/stm32/` — real hardware wrappers
9. `stm32/USER/main.cpp` — STM32 entry point
10. `tests/` — unit tests
11. `.github/workflows/ci.yml` — CI
12. Learning journals for each phase

---

## Relationship to the Course

Each Yahboom lesson adds a new feature to this project:

| Lesson | Adds to this project |
|---|---|
| 08 — Upright PD | `BalancePD` class |
| 09 — Speed PI | `VelocityPI` class |
| 10 — Steering PD | `TurnPD` class |
| 11 — DMP angle | `hal/stm32/stm32_sensor_hal` DMP path |
| 12 — Kalman | `KalmanFilter` class in `src/` |
| 13 — Complementary | `ComplementaryFilter` class in `src/` |
| Future — Bluetooth | New HAL interface for remote control |
| Future — LQR | `LQRController` class replacing PID |

---

## Tools

| Tool | Purpose |
|---|---|
| Keil MDK5 | STM32 compilation and flashing (Windows only) |
| CMake 3.20+ | Cross-platform build system for PC targets |
| Docker / Docker Desktop | Reproducible Linux build environment on Windows |
| Google Test | C++ unit testing framework |
| GitHub Actions | CI — builds and tests on every push |
| Serial Studio | Live plot CSV output from sim or STM32 |
| ST-Link SWD | Flash and debug STM32 |

---

## Current Status

- [ ] Project folder created: `Q:\mega_documents\Documents\Robotics\balance-car-cpp\`
- [ ] Git repo initialised
- [ ] PLAN.md written
- [ ] Implementation not yet started

Next step: create `src/pid/pid_control.hpp`
