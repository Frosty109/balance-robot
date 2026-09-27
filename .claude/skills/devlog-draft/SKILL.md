---
name: devlog-draft
description: Turn selected project devlog notes into a publishable, problem-led draft in the project's configured format. Use when the user asks to draft a devlog post or article from accumulated notes, names a devlog topic or slug, or explicitly invokes /devlog-draft.
---

<!-- Generated from .agents/skills/devlog-draft/SKILL.md by .agents/sync-claude-skills.sh. Edit the source. -->

# Devlog draft

1. Stop if `devlog/` does not exist.
2. Read `devlog/README.md` and follow its audience, destination, frontmatter, and asset rules
   exactly. A draft that has to be reformatted by hand defeats the point.
3. Resolve the requested slug or topic. If none is supplied, select the strongest ready grouping
   from `devlog/notes.md` and state the choice and why.
4. Read every assigned note plus relevant `learning/` and `issues/` entries.
5. Write `devlog/drafts/slug.md` as one problem worked through:
   - open with the problem and why it was harder than it looked, with no throat-clearing;
   - show the consequential wrong turn before the solution;
   - include real measurements, output, and artefacts by path, flagging any still missing;
   - name rejected alternatives and trade-offs;
   - keep code excerpts short and load-bearing, and link to the repo for the rest;
   - end with remaining uncertainty or unfinished work.
6. Do not fabricate missing details. Insert `> TODO:` markers for evidence or context that must be
   supplied.
7. Update source notes to `**Status:** drafted` and set `**Post:**` to the slug.

Leave publishing as a separate manual operation unless the user explicitly requests it.
