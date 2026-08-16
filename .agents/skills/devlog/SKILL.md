---
name: devlog
description: Capture public-facing, problem-led material from the current project session in devlog/notes.md. Use when the user asks to record devlog material, preserve a surprising failure or decision for a future article, capture perishable evidence, or explicitly invokes $devlog. Only operate when devlog/ exists.
---

# Devlog capture

1. Stop with a brief explanation if `devlog/` does not exist.
2. Read `devlog/README.md` for audience, publishing target, and artefact conventions.
3. Read `devlog/notes.md` before writing. Extend related notes rather than duplicating them.
4. Select only material an outside reader would care about: a surprising result, meaningful
   trade-off, non-obvious failure, rejected alternative, or concrete before/after number.
5. Prefer two strong notes over many weak ones. Routine progress is not devlog material.
6. Append this structure:

```markdown
## Short, specific title

- **Date:** YYYY-MM-DD
- **Status:** raw
- **Post:** unassigned

**The hook:** One sentence explaining why an outside reader cares.

**What I expected:**

**What actually happened:**

**The decision:** What was chosen, what was rejected, and why.

**Artefact needed:** [ ] The concrete screenshot, trace, log, or number needed.
```

Be honest about wrong turns. Flag artefacts that must be captured now before the broken state is
lost. If nothing clears the threshold, say so and write nothing.
