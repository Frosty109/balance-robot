# Review Findings: Whole-Project Review

Date: 2026-11-01
Reviewed: project-owned source at HEAD `9c2554f` plus working-tree edits (`src/`, `hal/`, `bsp/`,
`stm32/USER/`, both CMake files and the toolchain file, `tests/`, `simulation/`, `hal/sim/`), vendor
I2C/IMU code where project code depends on it, and repo hygiene (`.gitignore`, `ISSUES.md`,
`tools/`, Docker files). The ROS integration plan was reviewed separately the same day and
updated in place: `plans/2026-09-09-sbc-integration-and-avoidance.md`.

## Summary

The code is in good shape. The safety-critical paths fail safe by design and have targeted tests:
launch gate, stale-data shutdown, frame parser (any unexpected byte disarms), motion governor
(sequence check, slew limits, 250 ms timeout), and the SPSC receive ring with its error bit. The
ultrasonic ISR handles missed and delayed edges carefully. The HAL boundary is clean: `src/` holds
no hardware knowledge.

The findings are mostly around the code rather than in it: which image is running, unversioned
host tools, unchosen build settings, and a few latent traps.

No source changes were made as part of this review; no build or test was run.

## Findings

### 1. High: the running firmware cannot be traced to a commit

The flashed 52656-byte image was built from an uncommitted tree (dtmax instrumentation, RANGE
print guard removal, yaw tests). STATUS repeatedly qualifies evidence with "full runtime
configuration remains to be confirmed" and "every gain is not printed at runtime".

Recommendations:

- Commit a checkpoint before the Step 2 armed-RANGE flash.
- Print the git hash, a dirty flag and every gain in the boot banner. CMake can pass the hash as a
  compile definition. This removes a whole class of evidence qualifications.

### 2. High: `tools/` is not version-controlled, and this has already cost a file

`tools/flash.sh` was overwritten with 52584 bytes of firmware data and there is no history to
restore it from. `tools/drive.py` is safety-relevant (disarm on quit, SIGHUP handling) and is the
parity reference for the planned ROS bridge. `.gitignore` excludes `/tools/`, and AGENTS.md lists it
as local-only.

Recommendation: version `tools/*.py`, `*.sh` and `*.ps1` (not `__pycache__/`). These are source,
not notes. Owner decision, since it changes a documented policy.

### 3. Medium: the IMU self-test bias branch is dead code

`bsp/mpu6050/imu.cpp:147` pushes self-test biases to the DMP only when
`mpu_run_self_test() == 0x7`. With `MPU6050` defined and no `AK89xx_SECONDARY` compass,
`stm32/BSP_C/DMP/inv_mpu.c` builds the result from gyro (`0x01`) and accel (`0x02`) only, so the
maximum is `0x3` and the branch never runs. Yahboom's copy has the same defect.

Consequence: accel bias is never loaded; gyro bias relies only on DMP auto-calibration
(`DMP_FEATURE_GYRO_CAL`), which needs several seconds stationary. This *may* contribute to the
0-4 degree spread in mid-angle measurements (Stage 4b). That link is inference, not tested.

Do not simply change the check to `0x3`. The self-test bias assumes the chip is level and still at
boot, so enabling it is a behaviour change. Either delete the dead branch, or fix it and compare
the upright resting angle before and after in a controlled test.

### 4. Medium: the planned battery cutoff threshold is wrong for this pack

- `src/app/app_control.cpp:118`: `// || battery < 9.6f # Add this back in later`
- `bsp/motor/motor.hpp:22`: `MIN_VOLTAGE = 9.6f`, used only by `Motor::isFaulted`, which nothing calls
- `tests/test_app_control.cpp:142` `SafetyCutoffOnLowBattery` uses 9.0 V as "low"

9.6 V is Yahboom's 3S threshold. This car runs a 2S pack (about 8.4 V full), so re-enabling the
cutoff as written would stop the car ever arming. The test is also vacuous: it never arms, so the
motors are zero regardless (already tracked in ISSUES.md).

Recommendation: delete `Motor::isFaulted` and its constants; replace the comment with a
threshold-free TODO pointing at the battery open-risk row in STATUS.md.

### 5. Medium: build type, debug info and warnings were never chosen

`CMAKE_BUILD_TYPE` is empty in both `build/` and `build-fw-stage1-clean/`. The firmware compiles
with `-mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -std=gnu++23 -fno-exceptions
-fno-rtti`: that is **-O0**, no `-g`, no warnings.

- All timing evidence was taken at -O0, so this is not wrong, but it was not chosen.
- `poll=4` spends 4 ms of the 5 ms sample period, mostly bit-banged I2C overhead; optimisation
  would likely reduce it. UART transmit time is baud-bound and would not change.
- I2C and trigger delays use SysTick (`stm32/BSP_C/delay.c:58`), so optimisation does not break
  their timing. It does change loop timing: treat it as a controlled change with a restrained test.
- `-g` costs no flash (symbols stay in the ELF) and makes OpenOCD debugging usable.
- `-Wall -Wextra` on project sources only (not FWLib) has real learning value for a C++ project.

### 6. Medium: the simulator cannot currently validate anything

- `hal/sim/sim_sensor_hal.cpp:12-13` returns cumulative encoder position; firmware returns
  per-sample deltas, so `VelocityPI` sees ever-growing speed.
- Physics pitch and pitch-rate units do not match what `BalancePD` expects (degrees and raw gyro
  counts).
- `simulation/sim_main.cpp` gains (200/0.8, 1.2/0.05/200) date from May, not the firmware values.

Fine while parked. Fix before the simulation roadmap
(`plans/2026-09-28-simulation-self-righting-open-source-roadmap.md`) starts, and do not cite
simulator results until then.

### 7. Low: the telemetry wire format is spread across `printf` calls

Health rows are formatted inside `AppControl::update()`; event lines in both `AppControl` and
`main.cpp`. Host tests could capture stdout but none do: `dtmax` has no test and nothing pins
field names. The ROS bridge will now depend on this exact text (plan §4.1).

Recommendation, medium term: a small telemetry struct formatted in one place, giving the grammar a
single home in code and making it testable.

### 8. Low: range conversion logic has no in-repo tests

Pulse-to-mm conversion and status mapping live in `hal/stm32/stm32_range_sensor.cpp`, coupled to the
BSP driver. The earlier 30-assertion host harness was temporary. Moving the mapping into a pure
function with gtest coverage is cheap, and Step 4 proximity decisions depend on it.

## Minor

- `stm32/BSP_C/IOI2C.c:175-180`: `i2cWrite` ignores the register-address ack and returns 0 on a data
  NACK. DMP firmware load is read-back verified; plain config writes are not. Already open in ISSUES.md.
- ISSUES.md is stale. Open items resolved in current source: "DMP FIFO always empty",
  "`IIC_MPU6050_Init()` is never called" (now `imu.cpp:24`), "Remove temporary diagnostics" (no
  `<cstdio>` in `imu.cpp`), and "AGENTS.md contradicts the plans" (no Stage 2B claim remains).
  `myget_ms` is still a stub but harmless; the FIFO timestamp is unused.
- Comments exceeding the one-to-two-line rule: `src/app/app_control.cpp:217-224`,
  `stm32/CMakeLists.txt:39-42`.
- Motor timer and PC6-9 are configured only after the multi-second IMU/DMP initialisation, so driver
  inputs float until then. Relevant only if a power-on twitch has ever been seen; if so, initialise
  motors with zero duty first. Inference, not observed.

## Plan correction made during this review

With nothing in range the HC-SR04 holds ECHO high past the 30 ms deadline, so
`bsp/ultrasonic/ultrasonic.cpp` reports `timeout` (or `capture_error` if ECHO is still high at the
next trigger), not `no_echo`. The Step 4 nuisance-case bullet in the ROS plan was corrected.

## Suggested order

1. Commit a checkpoint; add hash and gains to the boot banner (finding 1).
2. Decide on versioning `tools/` and recover `flash.sh` from the Q6A copy (finding 2).
3. Remove the 9.6 V traps (finding 4) and reconcile ISSUES.md.
4. Choose build type, add `-g` and project-only warnings; defer any optimisation change to its own
   timing test (finding 5).
5. Self-test bias, telemetry struct, range tests and simulator fixes as their areas come up.
