# balance-car-cpp project instructions

## Project

This is a C++ rewrite of the Yahboom STM32 self-balancing car project. It exists to teach modern
C++, preserve a clean embedded architecture, and document the physical bring-up.

The current phase is hardware safety work, not tuning.

**Closed milestones.** Fresh-packet-gated control (`fa6eb90`); the 1 kHz TIM2 monotonic clock behind
an `IMonotonicClock` seam (`1bd5bb4`), tick rate measured at 1.001 rather than assumed; **Stage 2B
stale-data motor shutdown** (all four gates passed, verified on a clean image 2026-09-05); **Stage 5
encoder-sign normalisation** (`42ce5c6`); **Stage 6 velocity proportional control** at the reference
`BalancePD(10200, 78, 0)` / `VelocityPI(7000, 0, 200)`, gate met on restrained hardware 2026-09-09.

**The active milestone is the pre-free-balance safety pair**, promoted out of the deferred table on
2026-09-12 on hardware evidence:

1. **Intentional launch gate** — an explicit disabled/armed application state. Specified in full,
   host-test list included, under "Pre-free-balance prerequisite" in
   `plans/2026-08-16-balance-bring-up.md`. The firmware currently drives whenever the sample is
   fresh and `|angle| < 40`, so lifting the car disarms nothing: a lift capture showed both motors
   at full PWM for the entire 8.5 s run with the wheels free.
2. **Encoder backlog drain on fault/recovery** — **CLOSED 2026-09-18 in `6cad87d`.** The fault and
   stale branches returned before the encoder reads, so one read on recovery delivered the whole
   fault's coast-down and produced a post-recovery command of 43402 against a 2600 clamp. The
   encoders are now read on every fresh sample, ahead of every gate. Verified on restrained
   hardware, n = 3 recoveries: `devlog/artefacts/2026-09-18-backlog-drain-verification.txt`.
   Details in follow-up 1 of `plans/2026-09-05-stage5-encoder-signs-velocity.md`.

Original evidence for both: `devlog/artefacts/2026-09-12-stage7-integral-windup-and-fault-recovery.txt`.
Working plan for the pair: `plans/2026-09-14-pre-free-balance-safety-pair.md` — Phase A done,
Phase B (the launch gate) next.

**Stage 7 (velocity integral) is attempted and blocked** behind those two — its acceptance gate
needs a freely balancing car and was not assessable under hand-held restraint. Do not resume the
integral-limit walk without reading the correction in that plan; `Ki` is back to 0, the last
gate-passing value.

### Free balancing is prohibited

Not until **all** of these are closed:

| Prerequisite | State |
|---|---|
| Stage 2B gates (TIM2 rate, host tests, poll duration, fault injection) | **closed** |
| Intentional launch gate | **open — active** |
| Encoder backlog drain on fault/recovery | **closed** — `6cad87d`, 2026-09-18 |
| Stage 3 FIFO/EXTI acquisition decision | open |
| `mid_angle` trim (Stage 4b) | open; three unreconciled candidates, 0.0f accepted by decision |

Also open, on their own triggers rather than this gate: battery cutoff restoration (commented out at
`src/app/app_control.cpp:60`, needed before unattended running) and the turn sign fix (Stage 9, moot
while `TurnPD(0,0)`).

Tuning gains and enabling velocity control are **no longer prohibited** — Stages 5 and 6 closed that.
Turn control and the battery cutoff remain off.

### Operating rule on hardware

**Power off before lifting, carrying, repositioning, or touching the wheels.**

Do not tilt the car past the angle-fault threshold as a way of stopping the motors. The fault is not
a stop: returning inside ±10° re-arms automatically and drives on that same sample. The backlog
kick is gone, but a car held in the air after recovery still runs straight back to full PWM. This
rule stands until the launch gate exists.

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
