---
name: session
description: Create or update the current project's dated session handoff. Use when the user asks to record the session, save a handoff, summarize today's work for the next session, or explicitly invokes $session.
---

# Session handoff

1. Use today's actual date and target `sessions/YYYY-MM-DD.md`.
2. Read an existing file for today before writing. Update it rather than creating a duplicate.
3. Use the conversation, relevant diffs, and repository state as evidence. Do not invent work.
4. Write this structure:

```markdown
# Session — YYYY-MM-DD

### What we did
- Concrete outcomes: files, functions, measurements, fixes, and decisions.

### What's next
1. Ordered immediate steps, naming files and what to verify.
```

Make the document sufficient for a future session to resume without the conversation. Preserve
useful existing details. Distinguish completed work from proposed work and record consequential
unresolved uncertainty.

Before completing every session-document creation or update, read `../status/SKILL.md`
completely and reconcile `STATUS.md` as part of the same workflow. This is required even
when no stage, configuration or milestone changed; do not leave status reconciliation as
a task for the next session or require a separate request. Report any substantive changes
or that status was checked and unchanged, without inventing progress or padding the file.

When called by `session-finished`, defer this reconciliation to its final status step so
it runs once after all session records are complete.
