# balance-car-cpp

A C++ rewrite of the Yahboom Self-Balancing Car STM32 project, built alongside the PID course (lessons 08–13+). Three parallel goals: learn C++, build a clean embedded software architecture, and document the journey as blog-post-ready learning journal entries.

---

## Current Status

Phases 1 and 2 complete and pushed. All PID classes, HAL architecture, BSP, simulation, AppControl safety wiring, and tests done (14 passing).

**Next:** flash the GCC firmware to hardware via OpenOCD (needs the robot + ST-Link), then tune PID gains. The GCC firmware build is done — it compiles, links (~14% flash), and is guarded in CI; only flashing remains. Get the robot balancing before adding filters. See `PLAN.md` and `plans/2026-06-13-openocd-flash.md`.

See `PLAN.md` for the full implementation checklist and `ISSUES.md` for known bugs.

---

## How to Run

```bash
# Build and test (Docker)
docker compose run build

# Run tests directly (inside container or devcontainer)
cmake -B build && cmake --build build && cd build && ctest --output-on-failure

# Build STM32 firmware (GCC, native on Mac/Linux — needs arm-none-eabi-gcc on PATH)
cmake -B build-fw -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
cmake --build build-fw        # → build-fw/stm32/firmware.elf / .hex / .bin
```

Three build targets:
- **PC simulation** — `cmake -B build`, entry point `simulation/sim_main.cpp`, outputs CSV to stdout for Serial Studio
- **STM32 firmware (GCC/CMake)** — native on Mac/Linux via `cmake/arm-none-eabi-gcc.cmake`, entry point `stm32/USER/main.cpp`; build-checked in CI
- **STM32 firmware (Keil MDK5)** — Windows reference build, same `stm32/USER/main.cpp`

---

## Key Rules

- **The user does all the implementation.** Do not edit source files. Instead, hand over the exact change (full diff/snippet with reasoning) and let the user type it in themselves. This is a deliberate learning workflow — reviewing, explaining, and planning are welcome; authoring the edits is not.
- Comments only for non-obvious WHY — hardware constraints, magic numbers, workarounds. Never restate what the code says.
- `learning/`, `sessions/`, `issues/`, `plans/`, `devlog/`, and `PLAN.md` are local only — not pushed to GitHub. `devlog/` in particular holds material destined for the personal site, so it must stay out of the repo.
- PID classes in `src/pid/` are shared between sim and STM32. The HAL interfaces in `hal/interface/` are what make this possible — never put hardware knowledge into `src/`.
- `ISSUES.md` is a running list of known bugs and cleanups. The `issues/` folder holds per-session issue logs.

---

## Architecture

```
src/pid/, src/app/      ← pure C++ logic, no hardware knowledge
        ↓
hal/interface/          ← pure virtual interfaces (ISensorHAL, IMotorHAL)
        ↓
hal/stm32/ | hal/sim/   ← concrete HAL implementations
        ↓
bsp/                    ← Yahboom BSP rewritten as C++ classes
        ↓
FWLib/ + CMSIS/         ← ST's C drivers (kept as C, extern "C")
```

---

## Reference Documents

- `PLAN.md` — full project plan, implementation order, current status checklist
- `ISSUES.md` — running list of known bugs and cleanups
- `plans/` — standalone, ready-to-implement work specs (prospective handoffs)
- `sessions/` — per-session handoff documents
- `learning/` — topic-based C++ and embedded learning notes
- `issues/` — per-session issue logs
- `devlog/` — raw material captured for public write-ups; `devlog/README.md` holds the publishing target
- `claude-config/` — the reusable session-workflow spec this project's commands are derived from

---

## Custom Commands

- `/session` — write today's session handoff document in `sessions/`
- `/learning` — append new concepts learned this session to `learning/`
- `/issues` — log problems from this session to `issues/`
- `/plan` — write a standalone implementation plan to `plans/`
- `/session-finished` — run `/session`, `/learning`, `/issues` and `/devlog` in order (not `/plan` — plans are written on demand)

Devlog module (this project is written about publicly):

- `/devlog` — capture material from this session that an outside reader would find worth reading
- `/devlog-review` — sweep `sessions/`/`learning/`/`issues/` for missed material, report artefact debt, propose post groupings
- `/devlog-draft` — turn accumulated notes into a publishable draft in `devlog/drafts/`
