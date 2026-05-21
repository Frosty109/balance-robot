# Known Issues / Follow-ups

Running list of bugs, drift, and cleanups noticed during review. Tick as fixed.
Date noted: 2026-05-07.

---

## Bugs

- [ ] **Physics integration is wrong** — `simulation/physics.cpp:24-26`
  - `velocity_ = force * dt` overwrites velocity each tick instead of integrating
    it (`velocity_ += accel * dt`).
  - `position_left_ += velocity_ * ENCODER_SCALE` is missing `* dt`.
  - Effect: the sim isn't physically meaningful, so PID gains tuned in sim
    won't transfer cleanly to the real robot.

- [ ] **Build is broken** — `CMakeLists.txt:27-28`
  - `balance_tests` target lists `tests/test_velocity_pi.cpp` and
    `tests/test_turn_pd.cpp`, but only `test_balance_pd.cpp` exists.
  - Either write the two missing test files or remove them from the target.

- [ ] **Stray file at project root** — `imu.cpp`
  - 1-line orphan. The real implementation is `bsp/mpu6050/imu.cpp`.
  - Delete.

## Smaller cleanups

- [ ] **Implicit narrowing in `BalancePD::compute`** — `src/pid/pid_control.cpp:13`
  - Returns `int` from a float expression with no explicit cast.
  - Add `static_cast<int>(...)` so intent is visible and `-Wconversion` stays
    quiet if enabled.

- [ ] **Sim runs in real time** — `simulation/sim_main.cpp:35`
  - `std::this_thread::sleep_for(5ms)` per step means 50s sim = 50s wall clock.
  - Fine for live plotting, painful for batch PID tuning. Consider a
    `--realtime` flag (default off).

- [ ] **Magic numbers in STM32 main** — `stm32/USER/main.cpp:37`
  - `Stm32MotorHal motor_hal(2880, 0)` — name these (PWM period / prescaler)
    so the call site explains itself.

## Drift

- [ ] **PLAN.md status checkboxes stale** — `PLAN.md:246`
  - Says "next: tests" but `tests/test_balance_pd.cpp` is already written.
  - Tick the box and update the "Next step" line.

## Possible future cleanups (not urgent)

- [ ] **Sim CSV output is sparse** — `simulation/sim_main.cpp:29-33`
  - Only emits pitch, pitch_rate, velocity. For real PID tuning you'll want
    PWM, encoder_left, encoder_right, setpoint, error too. Add when tuning
    starts.

- [ ] **Transitive include in `sim_main.cpp`**
  - Doesn't directly include `pid_control.hpp` — works only because
    `app_control.hpp` pulls it in. Add the explicit include for robustness.