# Known Issues / Follow-ups

Running list of bugs, drift, and cleanups noticed during review. Tick as fixed.
Date noted: 2026-05-07.

---

## Bugs

- [x] **Physics integration is wrong** — `simulation/physics.cpp:24-26`
  - `velocity_ = force * dt` overwrites velocity each tick instead of integrating
    it (`velocity_ += accel * dt`).
  - `position_left_ += velocity_ * ENCODER_SCALE` is missing `* dt`.
  - Effect: the sim isn't physically meaningful, so PID gains tuned in sim
    won't transfer cleanly to the real robot.

- [x] **Build is broken** — `CMakeLists.txt:27-28`
  - `balance_tests` target lists `tests/test_velocity_pi.cpp` and
    `tests/test_turn_pd.cpp`, but only `test_balance_pd.cpp` exists.
  - Either write the two missing test files or remove them from the target.

- [x] **Stray file at project root** — `imu.cpp`
  - 1-line orphan. The real implementation is `bsp/mpu6050/imu.cpp`.
  - Delete.

## Smaller cleanups

- [x] **Implicit narrowing in `BalancePD::compute`** — `src/pid/pid_control.cpp:13`
  - Returns `int` from a float expression with no explicit cast.
  - Add `static_cast<int>(...)` so intent is visible and `-Wconversion` stays
    quiet if enabled.

- [ ] **Sim runs in real time** — `simulation/sim_main.cpp:35`
  - `std::this_thread::sleep_for(5ms)` per step means 50s sim = 50s wall clock.
  - Fine for live plotting, painful for batch PID tuning. Consider a
    `--realtime` flag (default off).

- [x] **Magic numbers in STM32 main** — `stm32/USER/main.cpp:37`
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

## Firmware / flashing (noted 2026-07-04 — first hardware flash)

- [x] **OpenOCD transport was `hla_swd`, rejected by modern driver** — `stm32/openocd.cfg:2`
  - First real flash (Windows, xPack OpenOCD 0.12) failed: `Error: Debug
    adapter doesn't support 'hla_swd' transport`. Modern `interface/stlink.cfg`
    selects the `st-link` (dapdirect) driver, which speaks `swd` — `hla_swd`
    belonged to the deprecated `hla` (High-Level Adapter) driver.
  - Fixed: `transport select hla_swd` → `transport select swd`. Config had never
    been run against hardware before (flashing was hardware-gated), so it sat
    latent ~2 weeks. Not OS-specific — Homebrew OpenOCD on the Mac is the same
    0.12 and would hit the identical error. Candidate CPP06 journal note.

- [ ] **`build-fw/` CMake cache is not portable across machines** — `build-fw/CMakeCache.txt`
  - `cmake --build build-fw --target flash` failed on Windows: the cache was
    generated on the Mac and hardcodes the Mac source path + Unix Makefiles /
    `/usr/bin/make`. Build dirs are per-machine, disposable artifacts.
  - Flashed instead by handing the existing ELF straight to OpenOCD (ELF is
    OS-independent, same arm-none-eabi-gcc toolchain):
    `openocd -f stm32/openocd.cfg -c "program build-fw/stm32/firmware.elf verify reset exit"`
  - Follow-up: add `build-fw/` to `.gitignore` / `.megaignore` so it stops
    syncing between machines; regenerate it fresh per machine when a local
    `--target flash` build is wanted (needs a Windows build tool, e.g. Ninja).

## MPU6050 / DMP angle path (noted 2026-07-04 — first hardware bring-up)

Full analysis: `issues/2026-07-04-dmp-fifo-review.md`. Fix plan: `plans/2026-07-04-imu-read-nan-fix.md`.

- [x] **`Imu::read()` was never called** — nothing drained the DMP FIFO, so `pitch_`
  never updated. Fixed with `poll()` on `ISensorHal`, called first in `AppControl::update()`.
- [x] **IMU angle members uninitialized** — `bsp/mpu6050/imu.cpp` constructor left
  `pitch_/roll_/yaw_` as garbage → NaN → `(int)NaN = -2147483648`. Fixed with member-init
  (identity quaternion, zeroed angles).
- [x] **`Stm32SensorHal::init()` swallowed the IMU bool** — was `void`, dropped
  `imu_.init()`'s result. Now `bool init()`; `main.cpp` prints `IMU INIT FAILED`. Also
  removed a duplicate `sensor_hal.init()` and moved `setvbuf` before the check.
- [ ] **DMP FIFO always empty** — `dmp_read_fifo` returns `R=-1 S=0 M=0` every call
  (`fifo_count < packet_length`). DMP configured but not producing packets. Driver + init
  are faithful to the original. **Not battery-related.** Decisive diagnostic (register dump)
  staged in `imu.cpp`, not yet flashed.
- [ ] **⭐ `IIC_MPU6050_Init()` is never called** — `stm32/BSP_C/IOI2C.c`. Soft-I2C pin
  setup (SDA/SCL = PB10/PB11) has zero callers; SCL (PB11) is configured by nothing.
  Concrete regression from the original. Fix: call it at the top of `Imu::init()`. Prime
  suspect for the empty FIFO (though I2C paradoxically works during init).
- [ ] **`i2cWrite()` returns success on NACK** — `stm32/BSP_C/IOI2C.c`. No write-error
  detection; ignores the reg-address ACK and returns `0` on a data-byte NACK. Latent.
- [ ] **`myget_ms()` is an empty stub** — `inv_mpu.c:2848`. DMP FIFO timestamp always
  garbage. Harmless (unused), matches original. Note only.
- [ ] **Remove temporary diagnostics** — `bsp/mpu6050/imu.cpp` (`<cstdio>` + register-dump
  `printf` in `read()`) and the `main.cpp` debug `printf` loop, once the angle tracks tilt.