---
name: devlog
description: Capture public-facing, problem-led material from the current project session in devlog/notes.md. Use when the user asks to record devlog material, preserve a surprising failure or decision for a future article, capture perishable evidence, or explicitly invokes /devlog. Only operate when devlog/ exists.
---

<!-- Generated from .agents/skills/devlog/SKILL.md by .agents/sync-claude-skills.sh. Edit the source. -->

# Devlog capture

1. Stop with a brief explanation if `devlog/` does not exist.
2. Read `devlog/README.md` for audience, publishing target, and artefact conventions.
3. Read `devlog/notes.md` before writing. Extend related notes rather than duplicating them.
4. Select only material an outside reader would care about, a higher bar than learning or issues:
   - something that turned out differently from what was expected;
   - a decision with a real trade-off, where the rejected alternative is interesting;
   - a constraint discovered the hard way: a peripheral that disagrees with its datasheet, a
     toolchain limit, a timing budget;
   - a non-obvious failure mode, especially a silent one;
   - a number worth quoting: flash/RAM, loop time, a before/after.

   Skip routine implementation, tutorials for well-documented things, setup, and progress without
   insight.
5. Prefer two strong notes over many weak ones.
6. Append this structure to the bottom of `devlog/notes.md`:

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

If the hook cannot be written, the note is not worth keeping. Change the artefact box to `[x]` and
give its path once captured. Be honest about wrong turns; they are the most readable part. Say so
loudly when an artefact must be captured now or be lost. If nothing clears the threshold, say so
and write nothing.
