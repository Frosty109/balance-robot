---
name: site-update
description: Draft, check and publish short dated progress updates for this project on the profile site, using the local model. Use when the session is finished, when the user asks to update the site or backfill updates, or explicitly invokes /site-update. Only operate when devlog/site-update.json exists.
---

<!-- Generated from .agents/skills/site-update/SKILL.md by .agents/sync-claude-skills.sh. Edit the source. -->

# Publish site updates

Skip silently when `devlog/site-update.json` does not exist. Design and rationale:
`plans/2026-10-01-site-update-skill.md`.

The script is `.agents/skills/site-update/site_update.py`. Run it from the repository root with
`python` on Windows or `python3` on macOS.

The operator authorised live publication on 2026-10-01. Updates carry the site's AI-summary tag,
so publishing does not need a further confirmation. It still needs every check below to pass.

## Steps

1. Run `status`. When nothing is pending or staged, report that and stop.
2. Draft:
   - after a session: `draft --date <today's session date>`;
   - for a backlog: `draft --catch-up --limit 5`.

   Drafting takes minutes per date. Run it in the background when the harness allows. Exit code
   3 means the local model is unreachable (it runs only on the Windows PC). Report the update as
   deferred and stop; the next run catches up through the ledger.
3. Check each staged draft in `devlog/site-updates/staging/` against its `<date>.sources.md`.
   Read the session itself, not only the draft. Correct or delete any bullet that:
   - drops a qualifier the source has (suspected, not yet confirmed, only built, on a stand);
   - states as fact a finding the same or the later session withdrew;
   - reports an event from after the update's date;
   - overclaims ("verified", "fixed", "solved") beyond the evidence;
   - says anything a stranger could not follow without the project's internal stage names;
   - covers workflow or note-keeping rather than the robot.

   Keep 2–4 bullets and about 60–100 words, in past tense and plain style. Only edit the bullets;
   the script owns the frontmatter.
4. If a date has nothing worth publishing, run `skip --date <date> --note "<reason>"`.
5. Run `check`, then `publish`. `publish` commits only the staged files to the site's `main`
   through a temporary worktree and pushes, which is the production deploy. It never touches
   the operator's site checkout or branch. Use `publish --dry-run` to see the commit without
   pushing.
6. Report:
   - the dates published, with the commit hash;
   - the dates skipped, with reasons;
   - anything deferred;
   - any edit made to a model draft and why.

## Rules

- Never publish a draft that has not been read against its sources in this run.
- Never edit `ledger` entries by hand except to re-open a date by deleting its entry.
- A wrong update that is already live is fixed by an ordinary edit on the site's `main`, never by
  rewriting history.
