# balance-car-cpp status

Where the project stands, with the evidence for each claim. Reconciled by the `status` skill;
`AGENTS.md` holds the standing rules and does not repeat what is here.

Last updated: 2026-10-17.

## Now

**The car balances freely.** Pressing `a` stands it up and it holds, and it recovers from pushes.
Free-balance and push-recovery telemetry was captured and accepted on 2026-09-24; the first
free-standing run is undated.

**The car drives on command.** Stage 11 closed 2026-10-17 at velocity integral limit 400: held
50 mm/s commands moved the car the commanded way in 5/5 `w` and 4/4 `s` holds (see Notes below).

**Stage 10, safe serial motion-command link — link accepted, turn runs open.** Plan:
`plans/2026-09-26-stage10-motion-command-link.md`; §2 decisions accepted 2026-09-28. Phases 0–6
done 2026-10-10: parser, governor and ring `a52210b` (host 94/94); USART ISR → ring `72af2ab`;
main-loop glue, link events and `vx`/`wz` telemetry `00735e0`; host drive tool `tools/drive.ps1`
(gitignored, not in git); bench regression with every B7 row except a hardware receive error,
`devlog/artefacts/2026-10-10-stage10-phase6-bench.txt`. Bytes are now handled in arrival order, so
space then `a` in one loop pass arms. **The car now acts on motion frames when armed.**
Phase 7 (floor) partly done 2026-10-10: link behaviour on the floor passed (F1/F2), and hold-to-drive
at 50 mm/s confirmed the **drive direction** — every hold that moved the car moved it the commanded
way — but about half the holds stalled (`devlog/artefacts/2026-10-10-stage10-phase7-hold.txt`).
F5–F9 and Phase 8 not run. The stalls were the velocity integral clamp, fixed by Stage 11.

**Next: Stage 10 turn runs F6–F8** on the Stage 11 image, with D8 feedforward as the first remedy
if commanded turns are still weak; then F5, F9 and Phase 8 to close Stage 10.

**Queued behind it:**

- C++17 → C++23 build migration — `plans/2026-09-26-cpp23-migration.md`, decided, not started,
  its own commit.
- Simulation, self-righting and an open-source rebuild —
  `plans/2026-09-28-simulation-self-righting-open-source-roadmap.md`, direction only, starts after
  Stage 10 closes.

**Accepted image.** `BalancePD(10200, 78, 0)`, `VelocityPI(7000, 35, 400)`,
`YawRateP(1.0f, 30.0f, 100)` with a zero yaw target, `mid_angle = 0.0f`. Battery cutoff off.
`FAULT_INJECT` removed from source. Flashed 2026-10-17: `00735e0` plus the one-line Stage 11
integral-limit change, governor limits ±100 mm/s, ±30 deg/s, 200 mm/s², 60 deg/s², 250 ms;
firmware `text` 48860 at `00735e0` (45304 before Stage 10), not re-recorded for the Stage 11 build.

## Closed milestones

| Milestone | Where | Evidence |
|---|---|---|
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
| Stage 3 FIFO/backlog acquisition | FIFO depth and worst-case sample age unmeasured; `Imu::read()` discards the DMP `more` count. Balancing reliably at 199.6 Hz with 3–4 ms worst-case polls. | balance/timing trouble, added runtime load, or FIFO overflow — expose the `more` high-water mark before redesigning acquisition |
| Battery measurement and cutoff (D4) | Cutoff off. Same ADC scale and 4.03 divider as Yahboom, but Yahboom averages 50 readings over ~1 s where ours averages four immediate conversions; a filtering problem is plausible but unproven. 2026-10-10: ~7.5 V at run start but ~5.6 V straight after every disarm in later runs (2S pack, ~8.4 V full); one run with zero wheel motion at ±4800 commanded. 2026-10-17: `battery=` jumps between ~5.6 V and ~7.6 V within one 100 ms row, with the motors off, during disarmed tipping and after boot. Still not measured at the pack; Stage 11 Phase 0 was waived by the operator. **Supervised runs only.** | before any unattended use, or any independent sign of battery trouble |
| Power loss while armed | Two reboots while armed and driving: 2026-10-10, and 2026-10-17 at 30.55 s of the first limit-400 run (USB link dropped, `BOOT` 1.2 s later, came up disarmed, then fell over). The CH340 is car-powered, so the board lost power. Cause not identified. The car drops rather than stopping under control. | before any run where a fall would cause damage, or a third occurrence |
| Commanded-turn authority | Weak with the car parked and wheels stalled. The 100-count cap is not the limit: with kp = 1.0 and targets clamped to 30 deg/s, `tr` ≤ 30. The watchdogged link exists (Stage 10) and forward drive now has authority (Stage 11), but turn runs F6–F8 have not been done. | next: Stage 10 F6–F8 on the Stage 11 image, with D8 feedforward as the first remedy |

**Lean versus creep.** At `Ki=4` the car crept backwards with the velocity integral pinned at +200.
At `Ki=35` it returns to stationary balance, but still settles at non-zero angles, so cable tug,
centre-of-mass offset and closed-loop error are indistinguishable from lean alone. `enc_l`/`enc_r`
tell a stationary lean from creep.
