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

## Control safety (noted 2026-09-12 — Stage 7 integral session)

Full analysis: `devlog/artefacts/2026-09-12-stage7-integral-windup-and-fault-recovery.txt`.
Raw evidence: runs A-D are preserved as
`devlog/artefacts/2026-09-12-stage7-run-a-ki35-limit200-ground-handled.txt` through
`devlog/artefacts/2026-09-12-stage7-run-d-ki35-limit500-fault-recovery.txt`; Run E was not
recoverable and remains operator-transcribed and unverified.
Plans updated: `plans/2026-08-16-balance-bring-up.md` (Stage 7 integral limit, launch gate),
`plans/2026-09-05-stage5-encoder-signs-velocity.md` (follow-ups 1 and 2, deferred table).

- [x] **⭐ No intentional launch gate** — **CLOSED 2026-09-22.** — `src/app/app_control.cpp`, `stm32/USER/main.cpp`.
  Motors run whenever the sample is fresh and `|angle| < 40`, so lifting the car disarms
  nothing. Demonstrated: body hand-held at ~±3°, both motors at full PWM for the entire
  8.5 s capture with the wheels free, `L` ~4600 against the 2600 clamp. The angle fault
  never fires because peak angles stayed at −35.79°/+34.74°. Fixed by the disabled/armed
  state specified in the bring-up plan, built in Phase B of
  `plans/2026-09-14-pre-free-balance-safety-pair.md`.

  Verified on restrained hardware 2026-09-22,
  `devlog/artefacts/2026-09-22-launch-gate-verification.txt`: 11 arm transitions, every one
  preceded by an operator `a` byte, none spurious, across 12 faults and 14 boots. B7 rows 1
  (startup purge) and 8 (stale) were skipped by decision and remain unobserved on hardware.

- [x] **⭐ Encoder backlog defeats the angle-fault integral reset** — **CLOSED 2026-09-18** in
  `6cad87d`; verified n = 3, `devlog/artefacts/2026-09-18-backlog-drain-verification.txt`. — `src/app/app_control.cpp:77`
  returns before the encoder reads at `:105-106`, so `TIM->CNT` accumulates unread for the
  whole fault. On the first non-faulted sample one read returns the entire backlog and
  `encoder_integral_` is driven straight back to its clamp — to the same rail it held before
  the fault. `velocity_.reset()` runs and makes no difference. Measured across three
  recoveries: backlog windows of 5,034 / −5,077 / 5,542 counts against ~1,100 normal. First
  post-recovery command reached **L = 43402**, 16.7x the clamp and the largest commanded PWM
  recorded in the project. That figure is from the Ki=0 run and is pure velocity P, so **Ki=0
  does not mitigate it — only draining the backlog does.** Same early-return problem exists on
  the stale path at `:51`.

- [ ] **`SafetyCutoffOnLowBattery` is a vacuous test** — `tests/test_app_control.cpp:142`. The
  battery cutoff it names is commented out at `src/app/app_control.cpp:60`, and the test drives
  angle 0, which produces zero balance output anyway — so it would pass whether or not a cutoff
  existed. It is not evidence of anything. Logged 2026-09-22; fix it when the battery cutoff is
  restored, not before, so the test and the feature land together.

- [ ] **Velocity integral has no leak term** — `src/pid/pid_control.cpp:28-38`. It is the
  integral term of a velocity PI controller, and integrating velocity error produces a
  displacement-like state. There is no decay: with the wheels stopped it holds indefinitely
  (observed parked at exactly the clamp for 4.2 s), at whatever value the wheels stopped on.
  Under the hand-held restraint used on 2026-09-12 it parked at or near the clamp for ~28–29%
  of windows at both limit 200 and limit 500, so raising the limit did not reduce clamp
  occupancy under those conditions. Needs a policy decision (leak, conditional integration, or
  reset on blocked/held) rather than a limit value. **Not** a candidate cause of the
  flat-ground hopping: that predates the Ki=35 experiment, and at Ki=0 the integral contributes
  nothing to the output.

- [ ] **Flat-ground and mat-edge hopping, cause unknown** — operator-reported, predates any
  nonzero Ki, resolved by a small push. Two candidates that fit a Ki=0 history: the deadzone
  discontinuity (`Motor::deadzone()` steps applied PWM through 2602 counts as the command
  crosses zero, at up to 200 Hz — observed `L` −15 → +1 between consecutive windows), and
  velocity P amplifying a stall-release against an obstruction. Both untested. A hop capture
  with the new `i=` field would separate them.

- [ ] **Left/right encoder asymmetry is direction-specific** — reproduced in two captures at
  identical saturated commands. Forward `enc_l/enc_r` = 1.20 and 1.26; reverse = 0.97 and 1.00.
  `enc_r` feeds the velocity loop, so any directional bias in the counts becomes a directional
  controller bias. Cause open: motors, gearboxes, H-bridges, brush geometry, friction and
  backlash can all behave asymmetrically by direction, so this does not point at the counting.
  (It is not `42ce5c6` — negating the right count cannot produce a 20% magnitude error.) Test:
  mark both wheels, drive at fixed PWM for fixed time each way, compare revolutions against
  counts — wheels differing points at the drivetrain, counts differing at the encoder path.

- [ ] **`Velocity_Kp = 7000` leaves little span margin under hard disturbance** — velocity P
  alone saturates the usable 0–1300 request span at ~19 counts/sample (~3,700 counts/s).
  Ground captures peaked at 12.8–15.5 counts/sample, i.e. 69–83% of the span consumed by
  velocity P, leaving the balance term almost nothing when they push the same way. Kp=7000 is
  the reference value and the Stage 6 gate passed with it, so this is recorded, not actioned.
  Weigh before free balancing.

- [ ] **⭐ AGENTS.md contradicts the plans** — it declares Stage 2B the active milestone,
  lists free-balancing gates that closed weeks ago, and is wrong about `reviews/` being
  gitignored. As of 2026-09-12 the plans declare a different active milestone (launch gate
  + encoder backlog drain), so the declared operational source of truth now disagrees with
  the planning documents. **Reconcile before the next agent-led session**, and cover all
  remaining pre-free-balance prerequisites, not only the two promoted on 2026-09-12.
