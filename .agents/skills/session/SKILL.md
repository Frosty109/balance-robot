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
- Concrete outcomes, including important files, measurements, fixes, and decisions.

### What's next
1. Ordered immediate steps with relevant files and verification work.
```

Make the document sufficient for a future session to resume without the conversation. Preserve
useful existing details. Distinguish completed work from proposed work and record consequential
unresolved uncertainty.
