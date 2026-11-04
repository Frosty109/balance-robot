---
name: session-finished
description: Complete the project's end-of-session documentation workflow. Use when the user says the session is finished, asks to wrap up or save all session records, or explicitly invokes /session-finished.
---

<!-- Generated from .agents/skills/session-finished/SKILL.md by .agents/sync-claude-skills.sh. Edit the source. -->

# Finish the session

Complete these operations in order:

1. Read `../session/SKILL.md` completely and create or update today's session handoff.
   Defer its required status reconciliation to step 6 below.
2. Read `../learning/SKILL.md` completely and append only genuine new learning.
3. Read `../issues/SKILL.md` completely and create or update today's issue record, including a
   no-issues entry when appropriate.
4. If `devlog/` exists, read `../devlog/SKILL.md` completely and capture only material that clears
   its public-interest threshold. Skip this step silently when `devlog/` does not exist.
5. If `devlog/site-update.json` exists, read `../site-update/SKILL.md` completely and publish
   today's site update. Skip this step silently when the file does not exist.
6. Always read `../status/SKILL.md` completely and reconcile `STATUS.md` after the other
   session records. Do not gate this step on milestones or other triggers, defer it to
   another session, or require a separate request. If substantive status is unchanged,
   report that it was checked; do not invent progress or pad the file.
7. Confirm the files created or updated, summarize any deliberately empty category, and state
   the result of the required status reconciliation.

Do not run `/plan`. Plans are created on demand, not automatically at session end.
Do not pad learning or devlog records merely to make every category non-empty.
