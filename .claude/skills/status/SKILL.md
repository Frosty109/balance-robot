---
name: status
description: Reconcile STATUS.md, and the standing claims in AGENTS.md, against what is actually true in the repository now. Use when the user asks to update or check the project status, after a stage or version changes, when a line has been proved wrong, or explicitly invokes /status.
---

<!-- Generated from .agents/skills/status/SKILL.md by .agents/sync-claude-skills.sh. Edit the source. -->

# Status reconciliation

`STATUS.md` is read before planning, so a wrong line in it is worse than a missing one: it gets acted
on. It drifts because the other workflow skills write new files and nothing else revisits it.

**Verify, do not recall.** Check each item against the repository, a command, or the evidence it
cites, and correct what has moved.

1. **Now.** Is the active work still active, the next plan still next, and every plan path still
   present with the status its header claims?
2. **Accepted state.** Check each configuration value, version, pin or decision against the file or
   command that defines it.
3. **Closed milestones.** Every commit or artefact path cited must exist.
4. **Open risks.** Has any "reopen when" condition been met?
5. **New work.** Any new plan, stage, component, client or top-level folder that is not listed?
6. **AGENTS.md claims of fact.** Build directories, commands, paths and repo boundaries it states.
7. **Anything this session proved false.** The most valuable correction and the easiest to lose.

Project-specific checks:

- **Accepted image.** Compare the `BalancePD`, `VelocityPI` and `YawRateP` arguments and the yaw
  target in `stm32/USER/main.cpp` with the line in `STATUS.md`. A source change is not a flashed
  image; say which one a claim refers to.
- **Commits.** Run `git cat-file -e <hash>` for every hash cited, and `git log --oneline -15` for
  stage commits not yet listed.
- **Evidence.** Every `devlog/artefacts/` and `plans/` path cited must exist; expand brace lists.
- **Plan headers.** Compare the `Status:` line of each plan named under "Now" with what `STATUS.md`
  says about it, using `grep -n '^Status:' plans/*.md`.
- **Host test count.** If `STATUS.md` quotes one, compare it with the latest `ctest` run in this
  session; do not run hardware steps to check it.
- **Measurements versus inference.** Keep hardware claims tied to a capture or a dated operator
  report. Do not upgrade an inference to a measurement while reconciling.

Update `Last updated:` in `STATUS.md`. Report what you changed and what you verified as unchanged;
"checked, unchanged" is a useful outcome. If a fact could not be verified, say so rather than
leaving the line to stand by default.

Do not pad either file. Prefer correcting a line to adding one, and delete what stopped being true.
