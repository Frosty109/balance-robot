Capture material from this session for the project's public devlog.

Only run this if `devlog/` exists. If it doesn't, say so and stop — this project isn't set up for public writing.

Read `devlog/README.md` first for the project's publishing target and audience.

Review this session and identify anything an OUTSIDE READER would find worth reading. That is a higher bar than /learning or /issues. Good candidates:

- Something that turned out differently from what was expected
- A decision with a real trade-off, where the rejected alternative is interesting
- A constraint discovered the hard way — a peripheral that doesn't behave like its datasheet, a toolchain limit, a timing budget
- A failure mode that is non-obvious, especially a silent one
- A number worth quoting: a flash/RAM figure, a loop time, a before/after

Bad candidates, skip these: routine implementation, anything that reads as a tutorial for a well-documented thing, "I set the project up", incremental progress with no insight.

Append each to `devlog/notes.md` in this format:

## [Short, specific title]

- **Date:** YYYY-MM-DD
- **Status:** raw
- **Post:** unassigned

**The hook:** One sentence on why a reader cares. If this sentence can't be written, the note isn't worth keeping.

**What I expected:**

**What actually happened:**

**The decision:** What was chosen — and what was rejected, and why.

**Artefact needed:** [ ] The screenshot, number, log excerpt or example output that would make this concrete. Change to `[x]` once captured and say where it's saved.

Rules:

- Scan `devlog/notes.md` first — never duplicate an existing note. Extend it instead.
- Be honest about wrong turns. The mistakes are the most readable part; sanding them off produces a devlog nobody wants to read.
- Two strong notes beat six weak ones. If nothing this session clears the bar, say so and write nothing.
- **Artefact needed** is the most important field. A scope trace of a bug that has since been fixed cannot be recreated. Say so loudly when an artefact must be captured now or be lost.
