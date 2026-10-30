# balance-car-cpp status

Where the project stands, with the evidence for each claim. Reconciled by the `status` skill;
`AGENTS.md` holds the standing rules and does not repeat what is here.

Last updated: 2026-10-30.

## Now

**The car balances freely, drives and turns on command.** Loaded-car gains and yaw ff=1 are
retained for supervised development after the 2026-10-24 holds, small-push recovery,
direction-reversal and turn captures. Residual creep, cycling, hesitation and weak turn-rate
tracking remain. Stage 10 is still open. Battery cutoff remains off.

**C++23 build/test validation completed 2026-10-30; hardware smoke passed by operator report.**
`CMakeLists.txt` sets 23; generated host and firmware Ninja files contain `-std=gnu++23`.
Windows host and Ubuntu 24.04 Docker each passed 96/96 tests. Firmware linked with
text/data/bss **49140/140/2548 bytes**, identical to the pre-rebuild ELF size recorded today.
Bare-metal syscall-stub linker warnings were supplied; whether they are new was not established.
After the transfer/flash instructions, the operator reported successful operation and supplied
`sessions/artefacts/2026-10-30/cpp23-smoke-pasted.txt`: moving/turn-command telemetry,
`poll=4`, operator DISARMED at host 60.558 s, then an angle fault at 62.306 s while disarmed.
The excerpt starts mid-run; BOOT/initialisation and programmer output are not included, although
the tool summary counts one BOOT. **Current source velocity integral limit is 2000**, and the
excerpt's `i` exceeds 1000. Do not describe this as an isolated standard-only hardware comparison
or promote limit2000 to the earlier limit1000 acceptance. Full runtime configuration and the
reason/timing of the limit change remain to be confirmed. Changes remain uncommitted.

**Radxa Dragon Q6A stack mounted 2026-10-17:** about +360 g on a ~1 kg car (Q6A/heatsink
179 g, power bank/cable 164 g, standoffs). With corrected motor plugs it balanced on the earlier
gains (`devlog/artefacts/2026-10-17-q6a-balance.txt`: untouched ±3.3°, holds +48/−174/+185 mm).
Cable-free operation works: car Type-C to Q6A USB (`/dev/ttyUSB0`, ch341 built in), driving
through `tools/drive.py` over SSH, flashing through CH340 with `stm32flash`/`tools/flash.sh`.
Quit disarm is exercised; dropped-SSH-session disarm is written but not hardware-verified.
Workflow: `radxa/balance-car-README.md`. Full logs are on the Q6A; local evidence below is
labelled `*-pasted.txt` where supplied as terminal excerpts rather than complete remote files.

**Serial startup remains intermittent; communication restored for testing.** The completed
exit sequence `-rts,-dtr,dtr` remains in `tools/flash.sh` with write/verify and `-R`. Its
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

Historical comparison: 2026-10-21 limit400 saturated windows averaged 91/97 mm/s backward;
limit800 settled saturated windows averaged 27/25 (`i=800` throughout). Those selected windows
and today's whole holds are different statistics, not a controlled percentage-improvement claim.
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
2026-10-17 at integral400, 5/5 forward and 4/4 backward holds). At that image F5 passed:
`LINK LOST` at 250–251 ms, targets reached zero in ~0.3 s, balance continued; only ~25 mm/s
actual speed at pause. F6/F8 were weak, F7 not properly tested, F9 yaw partial/100 mm/s untried.
Historical evidence: `devlog/artefacts/2026-10-17-stage10-F5{,b,-F9}.txt`.

**2026-10-24 turn update: ff=1, cap100 retained.** User added optional fourth yaw constructor
argument; feedforward uses the clamped target before the combined effort clamp. Default ff=0
preserves callers. Host build and **96/96 tests passed**, including two new feedforward tests.
`YawRateP(1,30,100,1)` floor capture demonstrated both turn directions up to ±30°/s and short
+50 mm/s/left-yaw motion, operator-confirmed. At forward target zero, aggregate mean yaw
was +2.4°/s at +10, +8.0°/s at +30 and −12.4°/s at −30. At +50/+30, ten rows averaged
+12.9°/s. Mixed windows include cycling/transients: direction/useful rotation demonstrated,
not accurate rate tracking or stationary in-place turns. Pitch −2.36°..+4.59°, peak |tr|60
below cap100, `poll=4`, acknowledged operator disarm, no armed fault/reboot. Evidence:
`devlog/artefacts/2026-10-24-yaw-ff1-left10-pasted.txt`. F6/F7 direction now demonstrated;
F8 left-curve evidence improved. Failure-path regression on this image remains open.

**Next: establish the intended velocity limit and current image identity**, then resume the
combined forward/right motion, return-to-zero and remaining F9 limits. The 2026-10-30 excerpt
is a smoke test, not closure of that prescribed sequence. Repeat F2/F5 regressions were deliberately
deferred by the operator on 2026-10-26; historical passes remain historical. Optional Phase 8
physical cable pull still needs a run/defer decision. Dropped-SSH-session disarm remains
unverified on hardware. Keep control settings fixed through the remaining motion tests;
stronger turn effort is not the immediate next change.
The SBC plan `plans/2026-09-09-sbc-integration-and-avoidance.md` is direction-setting and
not updated for USB/weights/power. Onboard BLE is a fallback: UART5 PC12/PD2, 9600 baud in
reference, `YahBoom_BL`, FFE0/FFE1 verified with nRF Connect; PC MediaTek GATT discovery hangs.

**C++23:** validation complete with the hardware-comparison qualification above;
`plans/2026-09-26-cpp23-migration.md`. No migration commit made.
**Queued:** simulation/self-righting/open-source rebuild
(`plans/2026-09-28-simulation-self-righting-open-source-roadmap.md`, after Stage 10).

**Last documented control acceptance (2026-10-24).** Configuration:
`BalancePD(12240,94,0)`, `VelocityPI(7000,35,1000)`, `YawRateP(1,30,100,1)`, yaw target from
link, `mid_angle=0`, `DEADZONE=1300`. User reported build/flash; supplied programmer output
verifies a 49,280-byte binary followed by running turn telemetry. `i` reaches1000 and `tr`
is consistent with ff=1. On 2026-10-26 a boot banner directly confirmed limit1000, but not every
gain. Current source instead sets limit2000, as qualified above; the boot print shares that
constant with the constructor. Source/test changes remain **uncommitted**. Last committed
firmware baseline remains `076b4a6`: balance `(10200,78,0)`, velocity `(7000,35,400)`, yaw
`(1,30,100)`; repository HEAD is docs commit `a39c693`. Historical Stage 11 remains closed
at that earlier configuration. Battery cutoff off; `FAULT_INJECT` absent. Governor unchanged:
±100 mm/s, ±30 deg/s, 200 mm/s², 60 deg/s², 250 ms. Latest host build/CTest **96/96 passed**.
Latest user firmware build succeeded; 2026-10-30 ELF text/data/bss are 49140/140/2548 bytes.
The historical 49,280-byte programming image is not the ELF text section.

## Closed milestones

| Milestone | Where | Evidence |
|---|---|---|
| C++23 build/test validation and reported hardware smoke | 2026-10-30, uncommitted; hardware comparison qualified by limit2000 source | `sessions/2026-10-30.md` |
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
| Reboot while armed | Two reboots while armed/driving: 2026-10-10 and 2026-10-17 at 30.55 s of the first limit400 run (USB dropped, BOOT 1.2 s later, disarmed, then fell). Power loss was inferred, not electrically measured. On 2026-10-24 CH340 remained present with the switch off, so adapter presence cannot establish MCU power. No third armed reboot shown today; cause still unknown. | before a run where a fall would cause damage, or a third occurrence |
| Commanded-turn tracking | ff=1/cap100 now demonstrated both directions and short combined left motion. At zero forward target, ±30°/s requests averaged +8.0/−12.4°/s across mixed windows; max absolute tr60. Direction works, precise tracking remains weak. Keep gains/cap unchanged through remaining regressions. Earlier ff16/cap500 proposal is not the next accepted change. | after link-loss/combined-motion regression, if stronger or more accurate turning is required |
| Intermittent serial startup | 2026-10-26 receiver-open physical reset captured genuine BOOT/config. Explicit drive-tool DTR/RTS settings restored output and reopening received telemetry. Automatic startup reliability and electrical cause remain unresolved; no pin measurements. See Now. | any recurrence; establish supported upright RX before arming, power off before handling when disarm is unacknowledged |
| Wheels stalled between the dead-zone offset and breakaway | Inference from 2026-10-17 telemetry, not a direct measurement: wheels stay at rest with commands of ≈1450–1600 PWM after the 1300 offset, so the integral cannot unwind and the car can stand on powered, stalled motors (9 s at ≈1450 in F5). **Raising `DEADZONE` to 1500 made balance oscillate within 0.5 s** (`bal` ±1800–2000 at ~3–5 Hz, `devlog/artefacts/2026-10-17-deadzone-1500.txt`); reverted. | stalls return on the wireless image, or a breakaway measurement is wanted |

**Lean versus creep.** On the earlier chassis, `Ki=4` crept backwards with the velocity integral
pinned at +200; `Ki=35` returned to stationary balance but still settled at non-zero angles.
The added stack has residual creep at `Ki=35` even with accepted limit1000 (above). Cable tug,
centre-of-mass offset and closed-loop error are indistinguishable from lean alone. `enc_l`/`enc_r`
tell a stationary lean from creep.
