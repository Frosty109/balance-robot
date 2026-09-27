---
name: plan
description: Write or update a standalone, evidence-backed implementation plan in the project's plans folder. Use when the user asks to plan a change in detail, create a handoff specification, save the discussed plan, or explicitly invokes /plan. Do not implement the planned source change unless separately requested.
---

<!-- Generated from .agents/skills/implementation-plan/SKILL.md by .agents/sync-claude-skills.sh. Edit the source. -->

# Implementation plan

1. Inspect the repository, `STATUS.md`, and relevant existing plans before writing.
2. Update a suitable existing plan when one covers the same work. Otherwise create
   `plans/YYYY-MM-DD-short-kebab-slug.md`.
3. Start with:

```markdown
# Plan title

Status: ready-to-implement
Component / area: affected subsystem and files
Created: YYYY-MM-DD
```

4. Make the plan usable without conversation context, by a future session or a different model.
   Include verified starting state, goal, decisions already made, numbered steps, exact files or
   interfaces, safety constraints, edge cases, tests, acceptance gates, rollback criteria, and
   definition of done.
5. Include code only when exact syntax is load-bearing. Prefer precise interfaces or pseudocode
   when a full listing would obscure the design; the user writes the source themselves.
6. Distinguish measurements, source-verified facts, assumptions, and hypotheses.
7. Use status values `ready-to-implement`, `in-progress`, `done`, or `abandoned`, and update the
   header as the plan progresses so stale plans are easy to spot.

Planning does not authorize implementation. Write only the plan unless the user also asks for the
source change.
