Write a publishable devlog post from accumulated notes.

Usage: `/devlog-draft [post slug or topic]`. With no argument, pick the strongest ready grouping in `devlog/notes.md` and say which you chose and why.

1. Read `devlog/README.md` for the publishing target — destination, frontmatter shape, audience. Match that format exactly. A draft that has to be reformatted by hand defeats the point.

2. Read every note in `devlog/notes.md` assigned to this post, plus any related `learning/` and `issues/` entries for technical detail.

3. Write the draft to `devlog/drafts/slug.md`, as one problem worked through:

   - Open with the problem and why it was harder than it looked. No throat-clearing, no "in this post I will".
   - Show the wrong turn before the right answer. The reasoning is the content; the final code is the least interesting part of it.
   - Include the artefacts — real numbers, real output, real screenshots. Reference them by path and flag any still missing.
   - Name the alternatives rejected and why. This is what separates a devlog from documentation.
   - Keep code excerpts short and load-bearing. Link to the repo for the rest.
   - End with what is still unsolved. Honesty about remaining rough edges reads as competence, not weakness.

4. Do not invent detail. Where the notes are thin, leave a `> TODO:` marker rather than filling the gap with plausible-sounding text. A draft with visible holes is useful; a draft with confident fabrication is worse than nothing.

5. Update the source notes to `**Status:** drafted` and set `**Post:**` to the slug.

Leave the draft in `devlog/drafts/`. Publishing to the site is a separate, manual step.
