# balance-car-cpp project instructions

## Project

This is a C++ rewrite of the Yahboom STM32 self-balancing car project. It exists to teach modern
C++, preserve a clean embedded architecture, and document the physical bring-up.

The current phase is hardware balance-loop tuning. Removing the fixed `delay_ms(5)` after
`AppControl::update()` substantially reduced jitter because the consumer had been slower than the
200 Hz DMP producer. The next milestone is fresh-packet-gated control, encoder-sign validation,
balance tuning, and then velocity hold. Use `plans/2026-08-16-balance-bring-up.md` as the current
detailed handoff.

## Working agreement

- The user normally authors firmware and application source as a deliberate learning workflow.
  Do not modify files under `src/`, `hal/`, `bsp/`, `stm32/`, `simulation/`, or `tests/` unless the
  user explicitly asks Codex to implement the change.
- Reviewing, diagnosing, explaining, and planning are welcome. Codex may write requested
  documentation, plans, session notes, learning notes, issue logs, devlog material, and agent
  configuration.
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

Repository skills under `.agents/skills/` implement the documentation workflow. Invoke them with
`$session`, `$learning`, `$issues`, `$implementation-plan`, `$session-finished`, `$devlog`,
`$devlog-review`, or `$devlog-draft`.
