---
name: implementation-plan
description: Write or update a standalone, evidence-backed implementation plan in the project's plans folder. Use when the user asks to plan a change in detail, create a handoff specification, save the discussed plan, or explicitly invokes $implementation-plan. Do not implement the planned source change unless separately requested.
---

# Implementation plan

1. Inspect the repository and relevant existing plans before writing.
2. Update a suitable existing plan when one covers the same work. Otherwise create
   `plans/YYYY-MM-DD-short-kebab-slug.md`.
3. Start with:

```markdown
# Plan title

Status: ready-to-implement
Component / area: affected subsystem and files
Created: YYYY-MM-DD
```

4. Make the plan usable without conversation context. Include verified starting state, goal,
   decisions already made, ordered steps, exact files or interfaces, safety constraints, edge
   cases, tests, acceptance gates, rollback criteria, and definition of done.
5. Include code only when exact syntax is load-bearing. Prefer precise interfaces or pseudocode
   when a full listing would obscure the design.
6. Distinguish measurements, source-verified facts, assumptions, and hypotheses.
7. Use status values `ready-to-implement`, `in-progress`, `done`, or `abandoned`.

Planning does not authorize implementation. Write only the plan unless the user also asks for the
source change.
