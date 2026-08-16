---
name: devlog-review
description: Audit the project's devlog pipeline for missed stories, uncaptured artefacts, thin periods, and candidate problem-led posts. Use when the user asks to review devlog coverage, find publishable material in project history, assess artefact debt, group notes into posts, or explicitly invokes $devlog-review.
---

# Devlog review

1. Stop if `devlog/` does not exist.
2. Read `devlog/README.md` and `devlog/notes.md`.
3. Determine the newest captured note date, then inspect relevant `sessions/`, `learning/`,
   `issues/`, and recent Git history from that point onward.
4. Identify missed material using the public-interest threshold in `../devlog/SKILL.md`. Append
   strong missing notes to `devlog/notes.md`; mark artefacts likely lost when the moment has passed.
5. Report every unchecked `Artefact needed` item and whether it remains capturable.
6. Group related notes by problem rather than chronology. Give each candidate post a working title
   and state whether it is ready to draft.
7. Flag important periods or subsystems with no useful captured material.

Present the review in the conversation. The only file this workflow may modify is
`devlog/notes.md`.
