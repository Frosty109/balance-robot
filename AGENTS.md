# balance-car-cpp project instructions

## Project

This is a C++ rewrite of the Yahboom STM32 self-balancing car project. It exists to teach modern
C++, preserve a clean embedded architecture, and document the physical bring-up.

The current phase is hardware safety work, not tuning. Fresh-packet-gated control is done and
accepted (`fa6eb90`), and a 1 kHz TIM2 monotonic clock is in place behind an `IMonotonicClock` seam
(`1bd5bb4`), with its tick rate measured on hardware at 1.001 rather than assumed.

The active milestone is **Stage 2B: stale-data motor shutdown** — an early return on a rejected poll
currently leaves the previous PWM latched, so a dead IMU means the car drives on its last output
forever. Use `plans/2026-08-25-stage2b-stale-shutdown.md` as the current detailed handoff;
`plans/2026-08-16-balance-bring-up.md` remains the reference for what follows (FIFO/EXTI decisions,
balance tuning, encoder-sign validation, velocity hold).

**Free balancing is prohibited** until four gates pass: TIM2 rate measured (closed), 28 host tests
passing (closed), worst-case poll duration comfortably below the stale timeout (closed), and
fault injection demonstrating zero PWM within the deadline on restrained hardware (open). Do not tune gains, restore the
battery cutoff, or enable velocity/turn control before then.

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

cmake -B build-fw -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
cmake --build build-fw
```

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

Note that `sessions/`, `learning/`, `issues/`, `plans/`, `devlog/`, `PLAN.md` and `reviews/` are all
gitignored and local-only. They are the project's memory but they are not in the repository history,
so they cannot be recovered from git if lost.
