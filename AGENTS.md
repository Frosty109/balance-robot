# balance-car-cpp project instructions

## Project

This is a C++ rewrite of the Yahboom STM32 self-balancing car project. It exists to teach modern
C++, preserve a clean embedded architecture, and document the physical bring-up.

The current phase is hardware safety work, not tuning.

**Closed milestones.** Fresh-packet-gated control (`fa6eb90`); the 1 kHz TIM2 monotonic clock behind
an `IMonotonicClock` seam (`1bd5bb4`), tick rate measured at 1.001 rather than assumed; **Stage 2B
stale-data motor shutdown** (all four gates passed, verified on a clean image 2026-09-05); **Stage 5
encoder-sign normalisation** (`42ce5c6`); **Stage 6 velocity proportional control** at the reference
`BalancePD(10200, 78, 0)` / `VelocityPI(7000, 0, 200)`, gate met on restrained hardware 2026-09-09;
**Stage 7 velocity integral control** at `VelocityPI(7000, 35, 200)` (`7aa0053`), accepted on a
freely balancing car 2026-09-24.

**The car balances freely.** Recorded 2026-09-22 on the operator's report: pressing `a` stands it
up and it holds. It has been doing so for some time; the first free-standing run remains undated,
but free-balance and push-recovery telemetry was captured and accepted on 2026-09-24.

**The pre-free-balance safety pair is closed**, both halves verified on restrained hardware:

1. **Intentional launch gate** — **CLOSED 2026-09-22** in `09009ca` (application state and
   command decoder) and `ab08218` (serial operator channel). An explicit disarmed/armed application
   state; the car only drives after an operator `a` byte. Specified under "Pre-free-balance
   prerequisite" in `plans/2026-08-16-balance-bring-up.md`, built in Phase B of the working plan.
   Before it, the firmware drove whenever the sample was fresh and `|angle| < 40`, so lifting the
   car disarmed nothing: a lift capture showed both motors at full PWM for an entire 8.5 s run with
   the wheels free. Evidence: `devlog/artefacts/2026-09-22-launch-gate-verification.txt` — 11 arm
   transitions across the session, every one immediately preceded by an operator byte, none
   spurious, over 12 faults, 8 recoveries and 14 boots.
2. **Encoder backlog drain on fault/recovery** — **CLOSED 2026-09-18 in `6cad87d`.** The fault and
   stale branches returned before the encoder reads, so one read on recovery delivered the whole
   fault's coast-down and produced a post-recovery command of 43402 against a 2600 clamp. The
   encoders are now read on every fresh sample, ahead of every gate. Verified on restrained
   hardware, n = 3 recoveries: `devlog/artefacts/2026-09-18-backlog-drain-verification.txt`.
   Details in follow-up 1 of `plans/2026-09-05-stage5-encoder-signs-velocity.md`.

Original evidence for both: `devlog/artefacts/2026-09-12-stage7-integral-windup-and-fault-recovery.txt`.
Working plan for the pair: `plans/2026-09-14-pre-free-balance-safety-pair.md` — Phases A, B and C
done. Phase D followed in `plans/2026-09-22-phase-d-to-first-free-balance.md`; the active plan is
now Stage 9 (see below).

**Stage 7 (velocity integral) is closed.** On 2026-09-24 the operator changed only `Ki`, from 0 to
the Yahboom reference value 35, while retaining the conservative integral limit of 200. On the
freely balancing car the integral left the clamp, wheel speed returned to zero after disturbances,
and the car showed no runaway or growing oscillation. The accepted capture spans 21.74 s and 218
telemetry rows: angle -8.39° to +8.42°, peak absolute motor command 1040, `poll` at 3–4 ms, and
39/218 rows at the integral clamp. Evidence:
`devlog/artefacts/2026-09-24-stage7-ki35-limit200-verification.txt`.

**Stage 4b (`mid_angle`) is closed at `0.0f`.** The operator reconfirmed on 2026-09-24 that it had
already been checked and was fine. Earlier records contain a clean but unrepeated 3.3° hand
measurement, later attempts spread from 0–4°, and Yahboom uses integer zero in its base mode. The
car now balances and recovers from pushes at zero without the direction-dependent convergence
failure that would identify a bad midpoint. Reopen only after an IMU/mass-layout change or if that
signature appears.

**Stage 3 FIFO/backlog acquisition is deferred by accepted-risk decision.** FIFO depth and worst-case
sample age remain unmeasured because `Imu::read()` discards the DMP `more` count. The operator chose
on 2026-09-24 not to add diagnostic/interface work while the car balances and recovers from pushes
reliably at 199.6 Hz with 3–4 ms worst-case polls. Reopen on balance/timing trouble, added runtime
load, or FIFO overflow; expose the `more` high-water mark before redesigning acquisition.

**Battery measurement and cutoff restoration (D4) is deferred by accepted-risk decision.** The
operator reports that the battery works with both this firmware and Yahboom's firmware. Both use
the same ADC scale and 4.03 divider ratio, but Yahboom averages 50 readings over about one second
where ours averages four immediate conversions, so a reading/filtering problem is plausible but
unproven. Our cutoff remains off: continue supervised runs only and reopen before unattended use or
on any independent sign of battery trouble.

**B7 row 1 passed by operator report on 2026-09-24.** One `a` received during boot was purged: no
`ARMED`, no wheel movement, and disarmed telemetry continued. No raw capture was supplied.

**B7 row 8 passed on restrained hardware on 2026-09-24.** The accepted-sample injection produced
stale shutdown at exactly 25 ms, no restart for 17.2 s, and a later arm rejection. Evidence:
`devlog/artefacts/2026-09-24-b7-row8-stale-launch-gate.txt`. B7 is now 10/10.

**The clean image was restored on 2026-09-25.** `FAULT_INJECT` has been removed from source, and a
clean reflash was captured at the start of Stage 9 Phase 1.

**Stage 9 (yaw-rate turn control) is closed, 2026-09-26.** Plan:
`plans/2026-09-24-stage9-yaw-rate-turn-control.md`.

- **Controller.** `TurnPD` is replaced by
  `YawRateP(kp_pwm_per_dps, max_target_dps, max_effort)`, working in deg/s. Positive yaw means
  left/CCW, as measured. Committed in `0d867d6`, `f94d16c` and `1240e22`; 68/68 host tests pass.
- **Accepted image.** `YawRateP(1.0f, 30.0f, 100)` with a zero target, harness removed. The
  `main.cpp` change is uncommitted.
- **What kp=1.0 does.** It cancels the yaw that the ~10% stronger left motor adds during every
  pitch recovery. kp=0.5 was too weak to do so.
- **Turn directions.** A +15 deg/s pulse gave a slight, operator-confirmed left turn. Neither −15
  pulse moved the car, because the wheels were stalled. The visible right turn and symmetry check
  were waived, since Phase 6 demonstrates both actuator directions behaviourally.
- **Carried to Stage 10: turn authority.** Commanded turns are weak at a 100-count cap when the
  car is parked with stalled wheels.

Evidence: `devlog/artefacts/2026-09-25-stage9-yaw-sign.txt` and the
`2026-09-26-stage9-{phase4-plumbing,phase5-pulse,phase6-kp05,phase6-kp10,phase7-pos15,phase7-neg15,final}.txt`
files.

### Free balancing — achieved, and what that changed

This section was headed "Free balancing is prohibited" until 2026-09-22. It no longer is: the car
stands up on an `a` and holds. The table below was written as a set of gates *to* free balancing;
since free balancing happened without two of them, they are recorded here for what they actually
are — safety work that is done, and robustness work that is not.

**Closed, and load-bearing:**

| Item | State |
|---|---|
| Stage 2B gates (TIM2 rate, host tests, poll duration, fault injection) | **closed** |
| Intentional launch gate | **closed** — `09009ca` + `ab08218`; all 10 B7 hardware rows passed by 2026-09-24 |
| Encoder backlog drain on fault/recovery | **closed** — `6cad87d`, 2026-09-18 |
| Stage 4b `mid_angle` | **closed** — retain `0.0f`, reconfirmed 2026-09-24 after free-balance and push recovery |

**Open — robustness, not gates.** These were listed as blocking free balancing and demonstrably
did not block it. They still matter, for different reasons than originally claimed:

| Item | Why it still matters |
|---|---|
| Stage 3 FIFO/EXTI acquisition decision | **deferred by accepted risk** — revisit only on balance/timing trouble or added runtime load |
| Battery cutoff restoration | **deferred by accepted risk** — telemetry is untrustworthy; supervised runs only; reopen before unattended operation |
| Commanded turns (Stage 10) | Stage 9 closed with yaw damping at kp=1.0. Turn *authority* on command is unproven; tune it with real serial commands, behind a watchdog |

**The observed symptom, and what it means.** Before Stage 7 the car tended to move backwards, with
the velocity integral state pinned at +200 when tested at a deliberately weak `Ki=4`. At `Ki=35`
the state repeatedly left the clamp and the car returned to stationary balance after disturbances.
It still settles at non-zero angles, so cable tug, centre-of-mass offset and ordinary closed-loop
error remain indistinguishable from lean alone. `enc_l`/`enc_r` discriminate a stationary lean from
creep. `mid_angle=0.0f` is accepted; do not reopen it merely because one capture parks at a non-zero
angle.

The battery cutoff remains off. Yaw damping runs at kp=1.0 with a zero target. There is no way to
command a turn until Stage 10's watchdogged command transport exists.

### Operating rule on hardware

**Disarm and see `DISARMED` before lifting, carrying, repositioning, or touching the wheels.**

Send a disarm byte — a space, or any byte that is not `a` — and read the `DISARMED reason=operator`
line back before your hands go near the wheels. Verified 2026-09-22: a fault now disarms for good,
recovery prints `RECOVERED` and does not drive, and only an explicit `a` starts the motors again.

Power off remains the fallback, and the only stop that does not depend on the serial link. The link
is not a dead-man switch: if the terminal stops responding while the car is armed, cut power rather
than approaching it. This matters more now the car balances freely than it did when every run was
hand-held — an armed car on the floor is not under anyone's hand.

A past-40° tilt is now a real stop rather than a pause, but it is a worse one than a keystroke —
it ends with the car at an angle in your hands. Use the disarm byte.

## Working agreement

- The user normally authors firmware and application source as a deliberate learning workflow.
  Do not modify files under `src/`, `hal/`, `bsp/`, `stm32/`, `simulation/`, or `tests/` unless the
  user explicitly asks the agent to implement the change. Give focused snippets or diffs one step at
  a time, with the reasoning, and let the user type them.
- Reviewing, diagnosing, explaining, and planning are welcome. The agent may write requested
  documentation, plans, session notes, learning notes, issue logs, devlog material, and agent
  configuration.
- Do not commit or push unless asked.
- Preserve unrelated working-tree changes. The tree is often intentionally dirty during hardware
  sessions.
- For physical-control changes, distinguish measurements from inference, change one control
  category at a time, and specify a safe restrained test.
- Comments should explain non-obvious reasons, hardware constraints, magic numbers, or
  workarounds. Do not restate the code.

## Architecture boundaries

```text
src/pid/, src/app/       pure C++ control and application logic
        ->
hal/interface/           hardware-independent interfaces
        ->
hal/stm32/ | hal/sim/    concrete platform adapters
        ->
bsp/                     C++ board-support classes
        ->
stm32/FWLib + CMSIS      retained C vendor libraries
```

- Keep hardware knowledge out of `src/`.
- Keep PID classes usable by both simulation and STM32 targets.
- Treat the legacy C sensor boundary as fallible; do not assume sample freshness or successful I2C.

## Build and test

```bash
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure

docker compose run build

cmake -B build-fw-stage1-clean -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
cmake --build build-fw-stage1-clean
```

**Use `build-fw-stage1-clean`.** It is the only firmware build directory with a valid toolchain in
its cache. `build-fw/` and `build-fw-stage1/` both cache
`CMAKE_TOOLCHAIN_FILE:UNINITIALIZED=cmake/arm-none-eabi-gcc` — missing the `.cmake` extension — and
are dead; `build/` and `build-stage1/` have no usable cache. Earlier versions of this file told you
to configure `build-fw` here and then flash from `build-fw-stage1-clean` below, which could not both
be right.

On the Windows machine the host build additionally needs a CA bundle, because it fetches googletest
via `FetchContent` and MinGW's GnuTLS has no configured trust anchors:

```powershell
$env:SSL_CERT_FILE = 'C:\mingw64\bin\curl-ca-bundle.crt'
cmake -B build -DCMAKE_TLS_CAINFO=C:/mingw64/bin/curl-ca-bundle.crt
```

That variable does **nothing** for the firmware build, which returns early at `CMakeLists.txt:8-12`
under `CMAKE_CROSSCOMPILING` and never reaches `FetchContent`. A green firmware build is not
evidence that the host build's TLS problem is solved.

Flashing goes through the CMake custom target, not a hand-typed OpenOCD image path. It rebuilds
`firmware.elf` if needed, then programs, verifies and resets:

```powershell
cmake --build build-fw-stage1-clean --target flash
```

A successful host build does not verify hardware behavior. Hardware claims require controlled
physical tests and telemetry or timing evidence.

## Project records

- `PLAN.md`: broad roadmap.
- `ISSUES.md`: running bug and cleanup list.
- `plans/`: prospective implementation handoffs.
- `sessions/`: dated session handoffs.
- `learning/`: topic-based references with dated additions.
- `issues/`: dated retrospective issue records.
- `devlog/`: public-facing notes and drafts; read `devlog/README.md` first.
- `reviews/`: review findings and adjudications.
- `Yahboom-Self-Balancing-Car/`: retained reference source; verify values here instead of relying
  on recollection.

The documentation workflow exists twice, once per agent, with the same content:

- Codex: repository skills under `.agents/skills/`, invoked as `$session`, `$learning`, `$issues`,
  `$implementation-plan`, `$session-finished`, `$devlog`, `$devlog-review`, `$devlog-draft`.
- Claude Code: slash commands under `.claude/commands/`, invoked as `/session`, `/learning`,
  `/issues`, `/plan`, `/session-finished`, `/devlog`, `/devlog-review`, `/devlog-draft`.

Keep the two in sync when either changes.

Note that `sessions/`, `learning/`, `issues/`, `plans/`, `devlog/` and `PLAN.md` are gitignored and
local-only. They are the project's memory but they are not in the repository history, so they cannot
be recovered from git if lost.

`reviews/` is **not** gitignored and has one tracked file
(`reviews/2026-07-12-dmp-fifo-review-findings.md`). An earlier version of this file listed it with
the others; that was wrong. Do not start ignoring a directory that already has history.
