Periodic sweep for missed devlog material, and a check on what's ready to publish.

Only run this if `devlog/` exists.

1. **Find missed material.** Read the recent contents of `sessions/`, `learning/` and `issues/`, plus recent git history, going back to the date of the newest note in `devlog/notes.md`. Look for anything that clears the /devlog bar but was never captured. Resolved entries in `issues/` are the richest seam — a debugging story with a real, identified cause is the most readable kind of post. Append what you find to `devlog/notes.md` in the standard format, marking the artefact as likely-lost where the moment has passed.

2. **Report artefact debt.** List every note with an uncaptured **Artefact needed**. For each, say whether it is still capturable or already gone. This is the part that decays — surface it every time.

3. **Propose post groupings.** Devlog posts should be organised BY PROBLEM, not chronologically. A chronological log is boring to read and impossible to finish. Group related raw notes into candidate posts, each with a working title that names a problem. Say which are substantial enough to draft now and which need more material.

4. **Flag the thin spots.** If a significant stretch of the project has no notes at all, say so — that's usually work that felt routine while doing it and will be invisible in the finished series.

Output the review in the conversation. The only file written is `devlog/notes.md`.
