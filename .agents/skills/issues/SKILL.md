---
name: issues
description: Record technical problems from the current session in the project's dated issue log. Use when the user asks to document issues, errors, failed approaches, causes, resolutions, open technical questions, or explicitly invokes $issues.
---

# Session issues

1. Target `issues/YYYY-MM-DD.md` using today's actual date. Create `issues/` if needed.
2. Read today's file before writing and append or update rather than duplicating an issue.
3. Keep root `ISSUES.md` unchanged; it is a separate running backlog.
4. Use this structure:

```markdown
## Short title

**Status:** Resolved / Unresolved / Workaround

**What happened:** Observable behavior and context.

**Cause:** Established cause, or explicitly state that it remains unknown.

**Resolution:** What fixed it, what was tried, or the next discriminating test.
```

Separate evidence from inference. Preserve useful wrong turns when they explain how the cause was
isolated. If there were no issues, create or update today's file with `No issues this session.`
