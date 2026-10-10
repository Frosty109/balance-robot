# balance-car-cpp project instructions

## Project

A C++ rewrite of the Yahboom STM32 self-balancing car. It exists to teach modern C++, keep a clean
embedded architecture, and document the physical bring-up.

Where the project stands — the accepted image,
closed stages with their evidence, open risks, and the next plan — is in **`STATUS.md`**. Read it
before planning or advising on hardware work, and trust it over older plans or sessions.

## Operating rule on hardware

**Disarm and see `DISARMED` before lifting, carrying, repositioning, or touching the wheels.**

Send a disarm byte — a space, or any byte that is not `a` — and read `DISARMED reason=operator`
back before hands go near the wheels. A fault disarms for good; recovery prints `RECOVERED` and does
not drive; only an explicit `a` starts the motors again.

Power off is the fallback and the only stop that does not depend on the serial link. The link is
not a dead-man switch: if the terminal stops responding while armed, cut power rather than
approaching the car. A past-40° tilt stops the car too, but ends with it at an angle in your hands.

The battery cutoff is off. Supervised runs only.

## Working agreement

- The user authors firmware and application source as a deliberate learning workflow. Do not modify
  files under `src/`, `hal/`, `bsp/`, `stm32/`, `simulation/`, or `tests/` unless the user
  explicitly asks the agent to implement the change. Give focused snippets or diffs one step at a
  time, with the reasoning, and let the user type them.
- Reviewing, diagnosing, explaining, and planning are welcome. The agent may write requested
  documentation, plans, session notes, learning notes, issue logs, devlog material, and agent
  configuration.
- Never flash firmware without the user's confirmation that the car is disarmed or powered safely.
- Do not commit or push unless asked.
- Preserve unrelated working-tree changes. The tree is often intentionally dirty during hardware
  sessions.
- For physical-control changes, distinguish measurements from inference, change one control
  category at a time, and specify a safe restrained test.
- A successful build does not verify hardware behaviour. Hardware claims need controlled physical
  tests with telemetry or timing evidence.
- Comments explain non-obvious reasons, hardware constraints, magic numbers, or workarounds. Do not
  restate the code.

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

## Build, test and flash

Two build directories are in use: `build/` for the host tests and simulator, and
`build-fw-stage1-clean/` for the firmware.

```bash
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure

docker compose run build

cmake -B build-fw-stage1-clean -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
cmake --build build-fw-stage1-clean
```

On Windows the host build needs a CA bundle, because it fetches googletest via `FetchContent` and
MinGW's GnuTLS has no trust anchors:

```powershell
$env:SSL_CERT_FILE = 'C:\mingw64\bin\curl-ca-bundle.crt'
cmake -B build -DCMAKE_TLS_CAINFO=C:/mingw64/bin/curl-ca-bundle.crt
```

The firmware build returns early under `CMAKE_CROSSCOMPILING` and never reaches `FetchContent`, so
a green firmware build says nothing about the host build's TLS setup.

Flash through the CMake target, which rebuilds if needed, then programs, verifies and resets:

```powershell
cmake --build build-fw-stage1-clean --target flash
```

## Project records

| Path | Holds | In git |
|---|---|---|
| `STATUS.md` | current state, accepted image, evidence, open risks | yes |
| `PLAN.md` | broad roadmap | no |
| `ISSUES.md` | running bug and cleanup list | yes |
| `plans/` | prospective implementation handoffs | no |
| `sessions/` | dated session handoffs | no |
| `learning/` | topic-based references with dated additions | no |
| `issues/` | dated retrospective issue records | no |
| `devlog/` | public-facing notes and drafts; read `devlog/README.md` first | no |
| `reviews/` | review findings and adjudications | yes |
| `Yahboom-Self-Balancing-Car/` | reference source; verify values here, not from memory | no |

The local-only records are synced by MEGA and backed up with FreeFileSync to a second drive and to
Backblaze; they are deliberately kept out of git.

MEGA sometimes creates twin files such as `notes(1).md`. If one appears, merge it into the
unsuffixed file rather than reading or writing the twin.

## Agent workflow skills

One source, two agents. Edit only `.agents/skills/<name>/SKILL.md` (Codex, invoked as `$name`),
then run `sh .agents/sync-claude-skills.sh` to regenerate `.claude/skills/` (Claude Code, invoked
as `/name`). `--check` reports drift. Codex's `implementation-plan` becomes Claude's `plan`.

Skills: `session`, `learning`, `issues`, `implementation-plan` / `plan`, `session-finished`,
`status`, `devlog`, `devlog-review`, `devlog-draft`, `site-update`.

`site-update` drafts the profile site's dated updates with the local model and pushes them live to
the site's `main`; its config and ledger are in `devlog/site-update.json`.
