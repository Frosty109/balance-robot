---
name: devlog-draft
description: Turn selected project devlog notes into a publishable, problem-led draft in the project's configured format. Use when the user asks to draft a devlog post or article from accumulated notes, names a devlog topic or slug, or explicitly invokes $devlog-draft.
---

# Devlog draft

1. Stop if `devlog/` does not exist.
2. Read `devlog/README.md` and follow its audience, destination, frontmatter, and asset rules.
3. Resolve the requested slug or topic. If none is supplied, select the strongest ready grouping
   from `devlog/notes.md` and state the choice.
4. Read every assigned note plus relevant `learning/` and `issues/` entries.
5. Write `devlog/drafts/slug.md` as one problem worked through:
   - open with the problem and why it was unexpectedly difficult;
   - show the consequential wrong turn before the solution;
   - include real measurements, output, and artefacts;
   - explain rejected alternatives and trade-offs;
   - keep code excerpts short and necessary;
   - end with remaining uncertainty or unfinished work.
6. Do not fabricate missing details. Insert `> TODO:` markers for evidence or context that must be
   supplied.
7. Update source notes to `**Status:** drafted` and set `**Post:**` to the slug.

Leave publishing as a separate manual operation unless the user explicitly requests it.
