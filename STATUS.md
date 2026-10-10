# balance-car-cpp status

Where the project stands, with the evidence for each claim. Reconciled by the `status` skill;
`AGENTS.md` holds the standing rules and does not repeat what is here.

Last updated: 2026-10-10.

## Now

**The car balances freely.** Pressing `a` stands it up and it holds, and it recovers from pushes.
Free-balance and push-recovery telemetry was captured and accepted on 2026-09-24; the first
free-standing run is undated.

**Active: Stage 10, safe serial motion-command link.** Plan:
`plans/2026-09-26-stage10-motion-command-link.md`; §2 decisions accepted 2026-09-28. Phases 0–3
done: frame parser, governor and byte ring in `a52210b` (host 94/94); USART RX ISR → ring in
`72af2ab`, with `takeCommands()` kept as a bridge through the old operator decoder until Phase 4.
Operator keys re-verified on that image 2026-10-10:
`devlog/artefacts/2026-10-10-stage10-phase3-ring-keys.txt` (boot purge not exercised).
**Next: Phase 4**, main-loop glue and telemetry; the car does not yet act on motion frames.

**Queued behind it:**

- C++17 → C++23 build migration — `plans/2026-09-26-cpp23-migration.md`, decided, not started,
  its own commit.
- Simulation, self-righting and an open-source rebuild —
  `plans/2026-09-28-simulation-self-righting-open-source-roadmap.md`, direction only, starts after
  Stage 10 closes.

**Accepted image.** `BalancePD(10200, 78, 0)`, `VelocityPI(7000, 35, 200)`,
`YawRateP(1.0f, 30.0f, 100)` with a zero yaw target, `mid_angle = 0.0f`. Battery cutoff off.
`FAULT_INJECT` removed from source. Flashed 2026-10-10: the `72af2ab` Phase 3 build, same gains;
firmware `text` 45652 (45304 before Stage 10).

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

Original evidence for the safety pair: `devlog/artefacts/2026-09-12-stage7-integral-windup-and-fault-recovery.txt`.
Plans: `plans/2026-09-14-pre-free-balance-safety-pair.md`, then
`plans/2026-09-22-phase-d-to-first-free-balance.md`.

### Notes on closed items

**Launch gate.** An explicit disarmed/armed state; the car only drives after an operator `a` byte.
Before it, the firmware drove whenever the sample was fresh and `|angle| < 40`, so a lift capture
showed both motors at full PWM for 8.5 s with the wheels free. Verification: 11 arm transitions,
each preceded by an operator byte, none spurious, over 12 faults, 8 recoveries and 14 boots.
B7 row 1 (an `a` during boot is purged) passed by operator report without a raw capture. B7 row 8
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

## Open, by accepted risk

These were once listed as gates to free balancing and demonstrably were not. They still matter.

| Item | State | Reopen when |
|---|---|---|
| Stage 3 FIFO/backlog acquisition | FIFO depth and worst-case sample age unmeasured; `Imu::read()` discards the DMP `more` count. Balancing reliably at 199.6 Hz with 3–4 ms worst-case polls. | balance/timing trouble, added runtime load, or FIFO overflow — expose the `more` high-water mark before redesigning acquisition |
| Battery measurement and cutoff (D4) | Cutoff off. Same ADC scale and 4.03 divider as Yahboom, but Yahboom averages 50 readings over ~1 s where ours averages four immediate conversions; a filtering problem is plausible but unproven. **Supervised runs only.** | before any unattended use, or any independent sign of battery trouble |
| Commanded-turn authority | Weak with the car parked and wheels stalled. The 100-count cap is not the limit: with kp = 1.0 and targets clamped to 30 deg/s, `tr` ≤ 30. | Stage 10, with real serial commands behind a watchdog |

**Lean versus creep.** At `Ki=4` the car crept backwards with the velocity integral pinned at +200.
At `Ki=35` it returns to stationary balance, but still settles at non-zero angles, so cable tug,
centre-of-mass offset and closed-loop error are indistinguishable from lean alone. `enc_l`/`enc_r`
tell a stationary lean from creep.
