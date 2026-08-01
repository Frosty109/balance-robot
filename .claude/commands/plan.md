Write the implementation plan discussed in this session to the `plans/` folder as a standalone handoff document. Create the `plans/` directory if it doesn't exist.

File name: `plans/YYYY-MM-DD-slug.md` using today's date and a short kebab-case slug describing the work (e.g. `2026-06-13-openocd-flash.md`).

Start the file with a status header:

# [Plan title]

Status: ready-to-implement
Component / area: which part of the codebase this touches
Created: YYYY-MM-DD

Then the plan itself. Make it complete enough that a different model (or a future session) can implement it without the conversation context — number the steps, name the exact files and line areas to change, include the actual code to add, and call out edge cases and rationale. Finish with any follow-up notes (testing, CI, version bumps).

Status values: `ready-to-implement` -> `in-progress` -> `done` (or `abandoned`). Update the header as the plan progresses so stale plans are easy to spot. Once `done`, keep the file as a record of intent vs. outcome, or archive it.

If a suitable plan file already exists for this work, update it rather than creating a duplicate.