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
done. **The active plan is now `plans/2026-09-22-phase-d-to-first-free-balance.md`**, which breaks
Phase D into steps with acceptance gates and is written to be actionable without prior context.

**Stage 7 (velocity integral) is closed.** On 2026-09-24 the operator changed only `Ki`, from 0 to
the Yahboom reference value 35, while retaining the conservative integral limit of 200. On the
freely balancing car the integral left the clamp, wheel speed returned to zero after disturbances,
and the car showed no runaway or growing oscillation. The accepted capture spans 21.74 s and 218
telemetry rows: angle -8.39° to +8.42°, peak absolute motor command 1040, `poll` at 3–4 ms, and
39/218 rows at the integral clamp. Evidence:
`devlog/artefacts/2026-09-24-stage7-ki35-limit200-verification.txt`.

**Stage 4b (`mid_angle`) is now the active task.** Use the freely balancing evidence rather than a
passive resting angle. The 2026-09-24 captures contain several stationary equilibria, so do not
select one telemetry window by convenience; follow the two-direction measurement method in Phase
D2 of `plans/2026-09-22-phase-d-to-first-free-balance.md`.

### Free balancing — achieved, and what that changed

This section was headed "Free balancing is prohibited" until 2026-09-22. It no longer is: the car
stands up on an `a` and holds. The table below was written as a set of gates *to* free balancing;
since free balancing happened without two of them, they are recorded here for what they actually
are — safety work that is done, and robustness work that is not.

**Closed, and load-bearing:**

| Item | State |
|---|---|
| Stage 2B gates (TIM2 rate, host tests, poll duration, fault injection) | **closed** |
| Intentional launch gate | **closed** — `09009ca` + `ab08218`, 2026-09-22; B7 rows 1 and 8 not run |
| Encoder backlog drain on fault/recovery | **closed** — `6cad87d`, 2026-09-18 |

**Open — robustness, not gates.** These were listed as blocking free balancing and demonstrably
did not block it. They still matter, for different reasons than originally claimed:

| Item | Why it still matters |
|---|---|
| Stage 3 FIFO/EXTI acquisition decision | bounded sample age is a phase-lag question; it affects how *reliably* the car balances, not whether it can |
| `mid_angle` trim (Stage 4b) | a midpoint error biases every control decision; the car balancing at `0.0f` is itself evidence worth reading — see below |
| Battery cutoff restoration | commented out at `src/app/app_control.cpp:60`, and the threshold is `9.6f` against an **8.4 V pack**, so it would fire permanently if uncommented. Blocked behind a known-bad ADC measurement chain. Needed before unattended running |
| B7 rows 1 and 8 | the startup purge is verified nowhere — not by any host test |
| Turn sign fix (Stage 9) | moot while `TurnPD(0,0)` |

**The observed symptom, and what it means.** Before Stage 7 the car tended to move backwards, with
the velocity integral state pinned at +200 when tested at a deliberately weak `Ki=4`. At `Ki=35`
the state repeatedly left the clamp and the car returned to stationary balance after disturbances.
It still settles at non-zero angles, so cable tug, centre-of-mass offset and `mid_angle` error remain
indistinguishable from lean alone. `enc_l`/`enc_r` discriminate a stationary lean from creep. The
stationary angles are useful D2 evidence, but the captures contain multiple equilibria rather than
one value that can be copied directly into `mid_angle`.

Turn control and the battery cutoff remain off.

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
