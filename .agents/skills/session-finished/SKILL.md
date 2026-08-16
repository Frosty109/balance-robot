---
name: session-finished
description: Complete the project's end-of-session documentation workflow. Use when the user says the session is finished, asks to wrap up or save all session records, or explicitly invokes $session-finished.
---

# Finish the session

Complete these operations in order:

1. Read `../session/SKILL.md` completely and create or update today's session handoff.
2. Read `../learning/SKILL.md` completely and append only genuine new learning.
3. Read `../issues/SKILL.md` completely and create or update today's issue record, including a
   no-issues entry when appropriate.
4. If `devlog/` exists, read `../devlog/SKILL.md` completely and capture only material that clears
   its public-interest threshold. Skip this step silently when `devlog/` does not exist.
5. Confirm the files created or updated and summarize any deliberately empty category.

Do not run `$implementation-plan`. Plans are created on demand, not automatically at session end.
Do not pad learning or devlog records merely to make every category non-empty.
