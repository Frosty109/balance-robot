# balance-car-cpp status

Where the project stands, with the evidence for each claim. Reconciled by the `status` skill;
`AGENTS.md` holds the standing rules and does not repeat what is here.

Last updated: 2026-11-07.

## Now

**The car balances freely, drives and turns on command.** Loaded-car gains and yaw ff=1 are
retained for supervised development after the 2026-10-24 holds, small-push recovery,
direction-reversal and turn captures. Residual creep, cycling, hesitation and weak turn-rate
tracking remain. Stage 10 is still open. Battery cutoff remains off.

**Next: a small read-only ROS 2 range bridge on the Q6A. Core parsers are complete.**
The operator agreed on 2026-11-01 to bring ROS forward and build proximity stopping as ROS
nodes, replacing the earlier non-ROS-demo-first order. The updated
`plans/2026-09-09-sbc-integration-and-avoidance.md` is **in-progress**: publish existing
serial telemetry while disarmed, verify the armed RANGE source change, add bounded command/arm/run
handling, then demonstrate a supervised proximity stop. Revised after review the same day, it now
holds the serial line grammar the bridge parses (§4.1), a replay adapter for drive.py logs, a
no-reset-on-open check, a predicted armed dtmax of about 17-19 ms from blocking prints (inference,
not measured), and a hold-to-run lease renewed by operator keystrokes so a Wi-Fi stall cannot keep
a run alive. ROS Jazzy is installed on the Q6A (SSH-verified 2026-11-07). The repository has a `ros2/`
workspace skeleton containing the plain-CMake core; no ROS nodes exist yet. Jazzy targets the Q6A's
Ubuntu 24.04 ARM64 platform. Language decided 2026-11-04: C++ (`rclcpp`) for the bridge,
operator lease and proximity policy, over a ROS-independent core testable on the PC; Python for
tooling only. The operator guides-and-types the code; Q6A was unreachable over SSH on 2026-11-04.
No further long static-distance capture is required to begin read-only integration.
On 2026-11-07 the operator agreed to start with `/car/range` and diagnostic health/state logs,
deferring full current-state tracking and capture replay until command support. They are not
prerequisites for first read-only output. An initial SSH check of `q6a` (192.168.32.165:22)
timed out on 2026-11-07; retry after the operator powered the Q6A succeeded. Verified Ubuntu
24.04.4/aarch64 and UTF-8 locale. The operator installed ROS Jazzy Base and development tools:
SSH confirms `ROS_DISTRO=jazzy`, `rclcpp` under `/opt/ros/jazzy`, `/usr/bin/colcon`, GCC 13.3.0
and CMake 3.28.3. After explicit operator approval, the 13-file core package was copied to
`~/balance-car/ros2/src/balance_car_core` on the Q6A. Its standalone Ninja build passed with
GCC 13.3.0 and system GoogleTest 1.14.0; **52/52 core tests passed**, zero failures, in **0.29 s**.
Evidence: `sessions/artefacts/2026-11-07/q6a-core-ctest-lasttest.log`. This is the core suite,
not the full Windows 148-test suite. Core install/export rules are now saved and synced:
colcon build/install passed, colcon reports 52 tests with zero failures, and CMake package
discovery reports `balance_car_core found`. The colcon CTest run emits a non-fatal missing
`DartConfiguration.tcl` warning. Evidence:
`sessions/artefacts/2026-11-07/q6a-core-colcon-test.log`. Next: range interface and read-only bridge.
No serial port was opened; live ROS publication remains pending.

**First core slice complete, committed as `e56ba10` (2026-11-06).**
`ros2/src/balance_car_core/` contains package metadata, a C++23 `parse_range()` implementation
and 12 parser tests. It is independent of ROS and firmware HAL and is wired into the root
host build. After correcting the older poll-duration test's obsolete end-of-line assumption
for the trailing `dtmax` field, the full host suite passed **108/108**, zero failures, in
**1.66 s** by the operator's terminal summary; the saved CTest log confirms all 108 passed.
Evidence: `sessions/artefacts/2026-11-06/host-ctest-summary-pasted.txt` and
`sessions/artefacts/2026-11-06/host-ctest-lasttest.log`.

**Bounded line buffer complete (2026-11-06), committed as `aa4c1a9`.** User-authored
`line_buffer.hpp`, `test_line_buffer.cpp` and package CMake wiring match the reviewed proposal
in `plans/2026-11-06-bounded-serial-line-buffer.md` (done). Its 16 tests cover fragmented and
coalesced lines, LF/CRLF capacity boundaries, overflow/discard resynchronisation, reset and
delivery to the RANGE parser. The integrated Windows host suite passed **124/124**, zero
failures, in **2.00 s** by the operator's summary, confirmed by the saved full CTest log:
`sessions/artefacts/2026-11-06/host-ctest-line-buffer-summary-pasted.txt` and
`sessions/artefacts/2026-11-06/host-ctest-line-buffer-lasttest.log`.

**Explicit state-event parsing complete (2026-11-06), committed as `aa4c1a9`.** User-authored
`state_events.hpp`, `state_events.cpp` and `test_state_events.cpp` match the reviewed proposal
in `plans/2026-11-06-state-event-parsing.md` (done); library/test CMake entries are present.
The parser decodes BOOT, ARMED, DISARMED, ARM REJECTED, FAULT, RECOVERED and STALE as owned
event values, without maintaining current robot state. All 17 new tests passed, and the full
Windows host suite passed **141/141**, zero failures, in **2.07 s** by the operator's summary,
confirmed by the saved CTest log: `sessions/artefacts/2026-11-06/host-ctest-state-events-summary-pasted.txt`
and `sessions/artefacts/2026-11-06/host-ctest-state-events-lasttest.log`.
**Periodic health-row parsing complete (2026-11-07).** The user authored
`health_telemetry.hpp`, `health_telemetry.cpp`, seven consolidated tests and package CMake
entries matching `plans/2026-11-07-health-row-parsing.md` (done). The agent added the missing
final test brace at the user's request. The parser preserves wire units and distinguishes
armed/disarmed layouts without granting arm permission. The full Windows host build and CTest
passed **148/148**, zero failures, in **1.93 s**; archived log:
`sessions/artefacts/2026-11-07/host-ctest-health-telemetry-lasttest.log`.
Next: add the range interface and publish range read-only while disarmed.
Serial transport, live ROS publication and proximity
stopping are not implemented. The package licence remains `TODO`; core install/export rules
are verified on the Q6A. No firmware build, flash or hardware run occurred today.

**Forward roadmap:** `plans/2026-11-01-feature-roadmap.md` (in-progress) sequences what follows:
camera, expression display, odometry and bounded moves, a read-only LLM, a model router across
Q6A/LAN/cloud models, voice, LiDAR, LLM-proposed actions, and a TypeScript web app starting as a
reporting dashboard. No display or LiDAR is owned yet. Stage 10 and historical motion follow-ups
remain open.

**Project review 2026-11-01:** `reviews/2026-11-01-project-review-findings.md`. Findings bearing on
status: source checkpointing has since been completed (`625a6af`, `b07fde6`, `e56ba10`, `aa4c1a9`),
but that does not establish the exact running image. The recorded image was built from an
uncommitted tree; both build trees have an empty `CMAKE_BUILD_TYPE`, so firmware is -O0
without `-g` (all timing evidence is at -O0);
the IMU self-test bias branch never runs on an MPU6050 (result mask at most `0x3`, code tests
`0x7`), so no boot bias is loaded; the 9.6 V cutoff remnants are a 3S value on this 2S car; the
simulator is unrepresentative.

**Passive HC-SR04 acquisition works; disarmed fresh-service timing now measured.** User-authored
BSP/HAL and CMake/main integration is committed as `9c2554f`. PA0 triggers, PA1/TIM5_CH2 captures
echo, internal TIM5_CH3 provides a 30 ms deadline, with attempts at least 100 ms apart. TIM2 clock,
TIM3/4 encoders and TIM8 PWM are unchanged. Main services range after `app.update()`; sensing
does not command motion. The saved 2026-11-01 diagnostic captures cover disarmed RANGE printing;
acquisition continues while armed. Source committed as `625a6af` uses
`if (range_sensor.poll())`, enabling printing in both states. On 2026-11-04 the operator reported
armed RANGE printing as fine; no capture or armed dtmax was saved, so the predicted armed load
remains unmeasured. Completion timestamps
retain conservative acquisition timing; capture errors use attempt start. The `dtmax` diagnostic,
also committed in `625a6af`, tracks maximum successive fresh IMU
poll-completion intervals, skips the first sample, persists across faults/arm/disarm and prints
in disarmed telemetry. It includes intervening range/serial work; `poll` remains IMU-poll-only.

2026-11-01 evidence:

- `sessions/artefacts/2026-11-01/range-dtmax-flash-pasted.txt`: operator's Q6A programmer excerpt
  verifies 52656 bytes and reset through the existing remote helper. Agent firmware build passed;
  ELF text/data/bss **52516/140/2548 bytes**, binary **52656 bytes**. Existing syscall-stub linker
  warnings remain. No full host suite was run that day; the saved result then was 96/96 on 2026-10-30.
- `sessions/artefacts/2026-11-01/range-dtmax-01-pasted.txt`: BOOT/IMU/MOTOR, CONFIG limit 2000,
  boot DISARMED; about 6 s of running telemetry, max poll 4 ms/dtmax 9 ms. One initial capture_error
  is followed by 60 valid results; ages 0-4 ms. This is a pasted terminal capture.
- Complete downloaded `sessions/artefacts/2026-11-01/2026-11-01-range-dtmax-02.txt`: 38.3 s,
  383 valid range results, no sequence gaps, ages 0-4 ms. Its 296 disarmed telemetry rows have
  poll 4 ms; dtmax changes 10->11 ms just after RECOVERED at host 5.491 s, then stays 11 through
  the last disarmed row at 35.045 s. Angle fault -40.15 degrees at 35.072 s stops ordinary
  telemetry; RANGE continues to 38.305 s. No STALE, arm or reboot appears; all transmitted
  motion frames are zero. No printed dtmax is available after the final angle fault.

These observations support proceeding with supervised integration. They do not establish
FIFO sample age, added range-only latency, timeout-path timing, armed printing load or stopping
clearance. The 11 ms maximum is an observed disarmed service gap below the 25 ms stale threshold,
not a general safety bound. Gain changes are not indicated. Bad-echo/raw-pulse characterisation
remains incomplete and can be targeted if integration exposes a problem.

Earlier range evidence, retained without requiring repeat captures:
`sessions/artefacts/2026-10-31/range-first-pasted.txt` has 71 results (70 valid, initial
capture_error), boot/config limit 2000 and RANGE resuming after disarm, consistent with armed
acquisition. `sessions/artefacts/2026-10-31/range-500mm-moved-pasted.txt` has 264 valid results;
the operator moved the target. Selected seq 1539-1559 spans 489-507 mm, mean 500.14 mm.
`sessions/artefacts/2026-10-31/range-1000mm-pasted.txt` has 330 valid/four timeouts with later
valid returns; selected seq 2827-2952 median 1019 mm, mean 1024.78 mm. Operator judged accuracy
sufficient; no calibration change planned. Operator-confirmed target removal/return around 500 mm
in `sessions/artefacts/2026-10-31/range-target-loss-500mm-pasted.txt` has 351 valid/two early
timeouts, with repeated near 500 mm sections and other valid background returns. Removal does
not guarantee invalid sensing indoors. None of these captures establishes calibrated accuracy
or armed-sensing acceptance. A temporary host harness previously passed 30 range HAL assertions;
ARM syntax/format checks passed. GPIO/TIM5/ISR timing was not exercised by that harness.

**Stock sensor hookup retained for first disarmed bring-up; electrical compatibility unmeasured.**
User reported plugging the HC-SR04 directly into the YB-EST01-V2.0 board and asked to follow
Yahboom. Their [car tutorial](https://www.yahboom.net/study/SBR-STM32/12245/#hardware-connection)
explicitly specifies 5V/TRIG PA0/ECHO PA1/GND and direct installation, with no added divider.
The local `Yahboom-Self-Balancing-Car/Yahboom Downloads/YB-EST01-V2.0.pdf` schematic likewise
shows direct ECHO routing. PA1 is not marked 5 V tolerant in the
[STM32 datasheet](https://www.st.com/resource/en/datasheet/stm32f103rc.pdf#page=32); actual ECHO
high voltage/protection remains unmeasured and the discrepancy unexplained. The agent's divider
proposal was an extra precaution, not a Yahboom assembly requirement. Retain the documented stock
kit connection for the first restrained disarmed test; do not label voltage compliance verified
or extend that decision to arbitrary replacement modules. First range output does not establish
voltage compliance.

**C++23 build/test validation completed 2026-10-30; hardware smoke passed by operator report.**
`CMakeLists.txt` sets 23; generated host and firmware Ninja files contain `-std=gnu++23`.
Windows host and Ubuntu 24.04 Docker each passed 96/96 tests. Firmware linked with
text/data/bss **49140/140/2548 bytes**, identical to the pre-rebuild ELF size recorded that day.
Bare-metal syscall-stub linker warnings were supplied; whether they are new was not established.
After the transfer/flash instructions, the operator reported successful operation and supplied
`sessions/artefacts/2026-10-30/cpp23-smoke-pasted.txt`: moving/turn-command telemetry,
`poll=4`, operator DISARMED at host 60.558 s, then an angle fault at 62.306 s while disarmed.
The excerpt starts mid-run; BOOT/initialisation and programmer output are not included, although
the tool summary counts one BOOT. **Current source velocity integral limit is 2000, intentionally
increased as confirmed by the operator on 2026-10-31**, and the
excerpt's `i` exceeds 1000. Do not describe this as an isolated standard-only hardware comparison
or promote limit 2000 to the earlier limit 1000 acceptance. Full runtime configuration and the
timing of the limit change relative to the flashed image remain to be confirmed. C++23 is now
committed as `23aeaaf`; limit 2000 and yaw feedforward source as `3aab687`. Commit existence
does not extend the earlier control acceptance. The subsequent range checkpoint is `9c2554f`;
range telemetry and the fresh-interval diagnostic are now committed as `625a6af`, and the
additional yaw tests as `b07fde6`. These source checkpoints do not identify a flashed image.

**Radxa Dragon Q6A stack mounted 2026-10-17:** about +360 g on a ~1 kg car (Q6A/heatsink
179 g, power bank/cable 164 g, standoffs). With corrected motor plugs it balanced on the earlier
gains (`devlog/artefacts/2026-10-17-q6a-balance.txt`: untouched ±3.3°, holds +48/−174/+185 mm).
Cable-free operation works: car Type-C to Q6A USB (`/dev/ttyUSB0`, ch341 built in), driving
through `tools/drive.py` over SSH, flashing through CH340 with `stm32flash` and the existing
Q6A `~/balance-car/flash.sh`.
Quit disarm is exercised; dropped-SSH-session disarm is written but not hardware-verified.
Workflow: `radxa/balance-car-README.md`. Full logs are on the Q6A; local evidence below is
labelled `*-pasted.txt` where supplied as terminal excerpts rather than complete remote files.

**Local flash-helper copy is damaged (2026-11-01).** `tools/flash.sh` is 52584 bytes of firmware
binary data, not a shell script; how it was overwritten is unknown. Do not upload it over the
working Q6A helper. The Q6A copy successfully flashed/verified 52656 bytes on 2026-11-01. Recover and
review the local script from that working copy or a verified backup before future helper
transfers; the documented direct `stm32flash` command remains an alternative. The firmware
binary and full diagnostic log transfers succeeded. No local helper repair was performed.

**Serial startup remains intermittent; communication restored for testing.** The completed
exit sequence `-rts,-dtr,dtr` is confirmed in the successful 2026-11-01 Q6A programmer output;
write/verify/reset succeeded. The local helper copy is damaged as noted above. The earlier
2026-10-21 recovery was not a complete fix; see
`devlog/artefacts/2026-10-21-limit-1000-flash-and-silent-start-excerpt.txt`.
On 2026-10-24 the first `tools/check-startup.py` hardware run received `RECOVERED` after 6.7 s,
but uptime was already 137.5 s: application alive, no demonstrated MCU reset. A later 30 s run
received zero characters; two reset-only bootloader attempts failed initialisation. Replacing
the cable/USB-C-to-A adapter setup preceded recovered telemetry. The operator suspects the
adapter, not isolated. Later ff=1 programming verified **49,280 bytes twice**, amid init
failures, followed by silence even in some upright captures and after restarting the Q6A.
Output returned following a reported full battery disconnect. Cause remains unknown;
NRST/BOOT0 levels are unmeasured. CH340 enumerated with the car switch off, so adapter presence
does not establish MCU power or a complete power cycle.

**Angle faults explain some silence, not all startup failures.** Regular telemetry stops after
the one-shot angle fault and resumes inside ±10°: +40.02° at host 15.297 s, `RECOVERED` +9.95°
at 16.526 s in the upright-link capture. Other silent attempts were upright by operator report.
Host `LINK UP`, local targets/key messages and programmer `Reset done.` do not establish
firmware acknowledgement. Evidence: `devlog/artefacts/2026-10-24-evidence-summary.md`,
`devlog/artefacts/2026-10-24-upright-link-check-pasted.txt`,
`devlog/artefacts/2026-10-24-yaw-ff1-flash-silent-pasted.txt` and
`devlog/artefacts/2026-10-24-yaw-ff1-startup-recovered-pasted.txt`.
On 2026-10-26 a physical reset with the receiver open captured genuine BOOT/IMU/MOTOR,
`CONFIG velocity_integral_limit=1000` and boot DISARMED. Explicit DTR=1/RTS=0 in
`tools/drive.py` restored output; closing/reopening also received continuing disarmed telemetry.
This is a practical improvement, not proof of reliable automatic reset or an isolated electrical
cause. NRST/BOOT0 remain unmeasured. Evidence: `sessions/2026-10-26.md` and its saved captures.

**Integral limit 1000 accepted for supervised development, with residual creep.** At balance
`(12240,94,0)` and velocity `(7000,35,1000)`, unchanged stack/firm rubber mats, zero-target
holds lasted 42.4 s and 26.4 s, with encoder-estimated mean backward drift 7.8 and 6.0 mm/s
(net 329 and 158 mm backward). User confirmed free balance throughout hold1. Repeat-hold
pitch −1.83°..+4.15°, integral +1000 in 41/263 rows (~16%), repeatedly unwinding. Residual
creep and a repeating fore/aft cycle remain; this is not stationary-balance acceptance.
Small pushes recovered freely in both directions, operator-confirmed without catching/support.
Separate ±50 mm/s reversal capture moved the requested way in 3/3 forward and 2/2 backward
holds, pitch −3.10°..+5.02°; initial start hesitated ~2.5 s, reversals ~0.1–1.2 s. All four
captures ended with acknowledged operator disarm; no armed reboot, `poll=4`. Evidence:
`devlog/artefacts/2026-10-24-limit1000-{hold1,hold2,push-recovery,reversal}-pasted.txt`.
Speeds use mean wheel counts, 6.27 counts/mm and nominal 100 ms telemetry windows, not
independent ground-distance measurements. Calculation windows and qualifications are in the
evidence summary above.

Historical comparison: 2026-10-21 limit 400 saturated windows averaged 91/97 mm/s backward;
limit 800 settled saturated windows averaged 27/25 (`i=800` throughout). Those selected windows
and the 2026-10-24 whole holds are different statistics, not a controlled percentage-improvement claim.
Evidence: `devlog/artefacts/2026-10-21-rollback-limit-{400,800}-pasted.txt`.
Earlier midpoint 1.0/2.0 trials worsened roll; balance ×1.4/midpoint −1 oscillated; single
balance-gain trials varied strongly (`devlog/artefacts/2026-10-17-q6a-bench2.txt`). Mixed floor/lift
capture is excluded: operator confirmed lifting while armed caused the sharp wheel-speed spikes.
Keep gains fixed during regressions; further integral increases need a new assessment.

**Stage 10 motion link: link accepted, floor closure open.** Plan:
`plans/2026-09-26-stage10-motion-command-link.md`, decisions accepted 2026-09-28.
Phases 0–6 done: parser/governor/ring `a52210b` (historical host 94/94), ISR ring `72af2ab`,
main-loop glue/telemetry `00735e0`, host tools. Bench covered every B7 row except hardware
receive error (`devlog/artefacts/2026-10-10-stage10-phase6-bench.txt`). Arrival order means
space then `a` in one pass arms. F1/F2 passed; F3/F4 stalls in
`devlog/artefacts/2026-10-10-stage10-phase7-hold.txt` were addressed by Stage 11 (closed
2026-10-17 at integral 400, 5/5 forward and 4/4 backward holds). At that image F5 passed:
`LINK LOST` at 250–251 ms, targets reached zero in ~0.3 s, balance continued; only ~25 mm/s
actual speed at pause. F6/F8 were weak, F7 not properly tested, F9 yaw partial/100 mm/s untried.
Historical evidence: `devlog/artefacts/2026-10-17-stage10-F5{,b,-F9}.txt`.

**2026-10-24 turn update: ff=1, cap 100 retained.** User added optional fourth yaw constructor
argument; feedforward uses the clamped target before the combined effort clamp. Default ff=0
preserves callers. Host build and **96/96 tests passed**, including two new feedforward tests.
`YawRateP(1,30,100,1)` floor capture demonstrated both turn directions up to ±30°/s and short
+50 mm/s/left-yaw motion, operator-confirmed. At forward target zero, aggregate mean yaw
was +2.4°/s at +10, +8.0°/s at +30 and −12.4°/s at −30. At +50/+30, ten rows averaged
+12.9°/s. Mixed windows include cycling/transients: direction/useful rotation demonstrated,
not accurate rate tracking or stationary in-place turns. Pitch −2.36°..+4.59°, peak |tr| 60
below cap 100, `poll=4`, acknowledged operator disarm, no armed fault/reboot. Evidence:
`devlog/artefacts/2026-10-24-yaw-ff1-left10-pasted.txt`. F6/F7 direction now demonstrated;
F8 left-curve evidence improved. Failure-path regression on this image remains open.

**Motion acceptance remains separate from ROS bring-up.** The 2026-11-01 52656-byte programming
verification and BOOT/config/dtmax capture establish the new range diagnostic image running
with integral limit 2000; every gain is not printed at runtime. The 2026-10-30 smoke excerpt and
the 2026-11-01 disarmed captures do not close the combined forward/right, return-to-zero or remaining
F9 rows. Repeat F2/F5 was deliberately deferred on 2026-10-26; optional physical cable pull
still needs a run/defer decision. Dropped-SSH-session disarm remains hardware-unverified.
Keep control settings fixed. Add command expiry, operator-loss and stop-path checks with the
ROS command/proximity feature trials; read-only ROS work need not wait for full Stage 10 closure.
The updated ROS integration plan above governs the new order. Passive range evidence is
sufficient to start that work; measured stopping clearance and useful range while balancing
remain to be established before accepting a proximity-stop demonstration. Onboard BLE remains
a fallback: UART5 PC12/PD2, reference 9600 baud, `YahBoom_BL`, FFE0/FFE1 verified with nRF Connect;
PC MediaTek GATT discovery hangs.

**C++23:** validation complete with the hardware-comparison qualification above;
`plans/2026-09-26-cpp23-migration.md`. Migration committed as `23aeaaf`.
**Queued:** simulation/self-righting/open-source rebuild
(`plans/2026-09-28-simulation-self-righting-open-source-roadmap.md`, after Stage 10).

**Last documented control acceptance (2026-10-24).** Configuration:
`BalancePD(12240,94,0)`, `VelocityPI(7000,35,1000)`, `YawRateP(1,30,100,1)`, yaw target from
link, `mid_angle=0`, `DEADZONE=1300`. User reported build/flash; supplied programmer output
verifies a 49,280-byte binary followed by running turn telemetry. `i` reaches 1000 and `tr`
is consistent with ff=1. On 2026-10-26 a boot banner directly confirmed limit 1000, but not every
gain. Current source instead sets limit 2000, as qualified above; the boot print shares that
constant with the constructor. Current source retains the control settings introduced in
`3aab687`: balance `(12240,94,0)`, velocity `(7000,35,2000)`, yaw `(1,30,100,1)`.
Range telemetry (also enabled while armed in source) and the fresh-interval diagnostic are
committed as `625a6af`; additional yaw tests as `b07fde6`. Historical Stage 11 remains closed
at the earlier `076b4a6` configuration, not the larger current limit. Battery cutoff off;
`FAULT_INJECT` absent. Governor unchanged:
±100 mm/s, ±30 deg/s, 200 mm/s², 60 deg/s², 250 ms. Latest full Windows host build/CTest
**148/148 passed (2026-11-07)**; latest saved Docker suite remains 96/96 (2026-10-30).
Latest documented firmware build passed on 2026-11-01, ELF text/data/bss
52516/140/2548 bytes. Operator Q6A output verifies a 52656-byte write and reset; the subsequent
BOOT/config/dtmax capture confirms the diagnostic runs with limit 2000. No readback hash or
full runtime gain banner was supplied. The historical 49280-byte and 52584-byte programming
images are superseded for current diagnostic work; binary lengths are not ELF text sizes.

## Closed milestones

| Milestone | Where | Evidence |
|---|---|---|
| Periodic health-row parser and host integration; 148/148 host tests | 2026-11-07; host tests only | `plans/2026-11-07-health-row-parsing.md` and its archived host CTest log |
| Explicit state-event parser and host integration; 141/141 host tests | 2026-11-06, `aa4c1a9`; PC tests only | `plans/2026-11-06-state-event-parsing.md` and its cited host CTest artefacts |
| Bounded line buffer and host integration; 124/124 host tests | 2026-11-06, `aa4c1a9`; PC tests only | `plans/2026-11-06-bounded-serial-line-buffer.md` and its cited host CTest artefacts |
| ROS-independent RANGE parser and host integration; 108/108 host tests | 2026-11-06, `e56ba10`; first core slice only | `sessions/2026-11-06.md` and its saved host CTest artefacts |
| C++23 build/test validation and reported hardware smoke | 2026-10-30, `23aeaaf`; hardware comparison qualified by limit 2000 source | `sessions/2026-10-30.md` |
| Fresh-packet-gated control | `fa6eb90` | |
| 1 kHz TIM2 monotonic clock behind `IMonotonicClock`; rate measured at 1.001 | `1bd5bb4` | |
| Stage 2B stale-data motor shutdown — all four gates | verified on a clean image 2026-09-05 | |
| Stage 5 encoder-sign normalisation | `42ce5c6` | |
| Stage 6 velocity P at `BalancePD(10200, 78, 0)` / `VelocityPI(7000, 0, 200)` | restrained, 2026-09-09 | |
| Encoder backlog drain on fault/recovery | `6cad87d`, 2026-09-18 | `devlog/artefacts/2026-09-18-backlog-drain-verification.txt` |
| Intentional launch gate | `09009ca` + `ab08218`, 2026-09-22 | `devlog/artefacts/2026-09-22-launch-gate-verification.txt` |
| Stage 7 velocity integral at `VelocityPI(7000, 35, 200)` | `7aa0053`, free balance 2026-09-24 | `devlog/artefacts/2026-09-24-stage7-ki35-limit200-verification.txt` |
| Stage 4b `mid_angle = 0.0f` | operator reconfirmed 2026-09-24 | |
| B7 hardware rows, 10/10 | 2026-09-24 | row 8: `devlog/artefacts/2026-09-24-b7-row8-stale-launch-gate.txt` |
| Stage 9 yaw-rate turn control at `YawRateP(1.0f, 30.0f, 100)` | `0d867d6`, `f94d16c`, `1240e22`, `2628a4a`, 2026-09-26 | see below |
| Stage 11 velocity integral limit 200 → 400 for commanded drive | `VelocityPI(7000, 35, 400)`, floor 2026-10-17 | see below |

Original evidence for the safety pair: `devlog/artefacts/2026-09-12-stage7-integral-windup-and-fault-recovery.txt`.
Plans: `plans/2026-09-14-pre-free-balance-safety-pair.md`, then
`plans/2026-09-22-phase-d-to-first-free-balance.md`.

### Notes on closed items

**Launch gate.** An explicit disarmed/armed state; the car only drives after an operator `a` byte.
Before it, the firmware drove whenever the sample was fresh and `|angle| < 40`, so a lift capture
showed both motors at full PWM for 8.5 s with the wheels free. Verification: 11 arm transitions,
each preceded by an operator byte, none spurious, over 12 faults, 8 recoveries and 14 boots.
B7 row 1 (an `a` during boot is purged) first passed by operator report; it was captured on
hardware on 2026-10-10 (`devlog/artefacts/2026-10-10-stage10-phase6-bench.txt`, `a` at 12,155 ms
between `BOOT` and `DISARMED reason=boot`, no arm). B7 row 8
produced stale shutdown at exactly 25 ms, no restart for 17.2 s, and a later arm rejection.

**Encoder backlog drain.** Fault and stale branches returned before the encoder reads, so the first
read on recovery delivered the whole coast-down: a post-recovery command of 43402 against a 2600
clamp. Encoders are now read on every fresh sample, ahead of every gate. Verified n = 3.
Details in follow-up 1 of `plans/2026-09-05-stage5-encoder-signs-velocity.md`.

**Stage 7.** Only `Ki` changed, 0 → 35 (Yahboom's value), keeping the conservative integral limit
of 200 rather than Yahboom's 8000. The integral left the clamp, wheel speed returned to zero after
disturbances, no runaway or growing oscillation. Capture: 21.74 s, 218 rows, angle −8.39° to
+8.42°, peak |motor| 1040, `poll` 3–4 ms, 39/218 rows at the integral clamp.

**Stage 4b.** A clean but unrepeated 3.3° hand measurement, later attempts spread over 0–4°, and
Yahboom uses integer zero. The car balances and recovers at zero without the direction-dependent
convergence failure a bad midpoint would cause. Reopen only after an IMU or mass-layout change, or
if that signature appears. A capture that parks at a non-zero angle is not, by itself, a reason.

**Stage 9.** `TurnPD` replaced by `YawRateP(kp_pwm_per_dps, max_target_dps, max_effort)` in deg/s;
positive yaw is left/CCW, as measured. kp=1.0 cancels the yaw the ~10% stronger left motor adds
during every pitch recovery; kp=0.5 was too weak. A +15 deg/s pulse gave a slight left turn;
neither −15 pulse moved the parked car because the wheels were stalled, and the symmetry check was
waived. 68/68 host tests. Plan: `plans/2026-09-24-stage9-yaw-rate-turn-control.md`. Evidence:
`devlog/artefacts/2026-09-25-stage9-yaw-sign.txt` and
`2026-09-26-stage9-{phase4-plumbing,phase5-pulse,phase6-kp05,phase6-kp10,phase7-pos15,phase7-neg15,final}.txt`.

**Stage 11.** Only the integral limit changed, 200 → 400 (authority ±70 → ±140 counts); 800 was
not needed. At 200 with the wheels driven, 1/4 `w` and 1/1 `s` holds moved; in stalled holds the
integral pinned within ~0.4 s and a small lean cancelled it, wheels at rest for 42/44 rows. At 400,
over two runs, `w` 5/5 (+101 to +172 mm) and `s` 4/4 (−55 to −172 mm), wheels at rest in 1–12 rows
per hold. Untouched: −7…+37 mm, −3.8…+1.6°, integral clamped 8% of rows (35–63% at 200). Two pushes
(+11.7°, −11.9°) recovered in ~2 s with no growing oscillation. Arming transient similar to 200.
Accepted with these shortfalls: untouched was 16 s, not 20; holds were split across two runs; the
pack was not metered (Phase 0 waived by the operator). Plan:
`plans/2026-10-10-stage11-velocity-authority.md`. Evidence in `devlog/artefacts/`:
`2026-10-17-stage11-baseline-200.txt` (zero-target only; drive keys ignored in the VS Code
terminal), `2026-10-17-stage11-baseline-200b.txt` (limit 200 holds; the first limit-400 run is
appended from line 1744), `2026-10-17-stage11-baseline-400b.txt`, `2026-10-17-stage11-limit-400-gate.txt`.

## Open, by accepted risk

These were once listed as gates to free balancing and demonstrably were not. They still matter.

| Item | State | Reopen when |
|---|---|---|
| Stage 3 FIFO/backlog acquisition | FIFO depth and worst-case sample age unmeasured; `Imu::read()` discards the DMP `more` count. Earlier balancing at 199.6 Hz with 3–4 ms worst-case polls. Disarmed range/serial-load diagnostic observed max fresh-service interval 11 ms on 2026-11-01; armed printing and timeout-path load remain unmeasured. FIFO sample age is still unknown. | balance/timing trouble, added runtime load, or FIFO overflow — expose the `more` high-water mark before redesigning acquisition |
| Battery measurement and cutoff (D4) | Cutoff off. Same ADC scale and 4.03 divider as Yahboom, but Yahboom averages 50 readings over ~1 s where ours averages four immediate conversions; a filtering problem is plausible but unproven. 2026-10-10: ~7.5 V at run start but ~5.6 V straight after every disarm in later runs (2S pack, ~8.4 V full); one run with zero wheel motion at ±4800 commanded. 2026-10-17: `battery=` jumps between ~5.6 V and ~7.6 V within one 100 ms row, with the motors off, during disarmed tipping and after boot. Still not measured at the pack; Stage 11 Phase 0 was waived by the operator. **Supervised runs only.** | before any unattended use, or any independent sign of battery trouble |
| Reboot while armed | Two reboots while armed/driving: 2026-10-10 and 2026-10-17 at 30.55 s of the first limit 400 run (USB dropped, BOOT 1.2 s later, disarmed, then fell). Power loss was inferred, not electrically measured. On 2026-10-24 CH340 remained present with the switch off, so adapter presence cannot establish MCU power. No third armed reboot shown today; cause still unknown. | before a run where a fall would cause damage, or a third occurrence |
| Commanded-turn tracking | ff=1/cap 100 now demonstrated both directions and short combined left motion. At zero forward target, ±30°/s requests averaged +8.0/−12.4°/s across mixed windows; max absolute tr 60. Direction works, precise tracking remains weak. Keep gains/cap unchanged through remaining regressions. Earlier ff 16/cap 500 proposal is not the next accepted change. | after link-loss/combined-motion regression, if stronger or more accurate turning is required |
| Intermittent serial startup | 2026-10-26 receiver-open physical reset captured genuine BOOT/config. Explicit drive-tool DTR/RTS settings restored output and reopening received telemetry. Automatic startup reliability and electrical cause remain unresolved; no pin measurements. See Now. | any recurrence; establish supported upright RX before arming, power off before handling when disarm is unacknowledged |
| Wheels stalled between the dead-zone offset and breakaway | Inference from 2026-10-17 telemetry, not a direct measurement: wheels stay at rest with commands of ≈1450–1600 PWM after the 1300 offset, so the integral cannot unwind and the car can stand on powered, stalled motors (9 s at ≈1450 in F5). **Raising `DEADZONE` to 1500 made balance oscillate within 0.5 s** (`bal` ±1800–2000 at ~3–5 Hz, `devlog/artefacts/2026-10-17-deadzone-1500.txt`); reverted. | stalls return on the wireless image, or a breakaway measurement is wanted |

**Lean versus creep.** On the earlier chassis, `Ki=4` crept backwards with the velocity integral
pinned at +200; `Ki=35` returned to stationary balance but still settled at non-zero angles.
The added stack has residual creep at `Ki=35` even with accepted limit 1000 (above). Cable tug,
centre-of-mass offset and closed-loop error are indistinguishable from lean alone. `enc_l`/`enc_r`
tell a stationary lean from creep.
