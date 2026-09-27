---
name: learning
description: Maintain the project's topic-based learning notes. Use when the user asks to record what was learned, explain durable concepts from the session in the learning folder, update learning documentation, or explicitly invokes $learning.
---

# Learning notes

1. Review the session for genuinely new C++, embedded-systems, control-theory, physics, algorithm,
   API, or debugging concepts.
2. Inspect `learning/` and select the existing topic file that best fits. Files are one per topic,
   growing over time. Create a concise new topic file only when no existing file fits.
3. Read the destination before writing and do not duplicate existing material.
4. Append a dated `### YYYY-MM-DD` section under the relevant concept.
5. Explain why and how, not merely what happened:
   - how syntax or APIs actually work, with short examples where they help;
   - the physics or maths behind an algorithm where relevant;
   - why an approach was chosen over the alternatives;
   - gotchas and non-obvious behaviour.
6. Write for embedded developers learning C++ from a C background. The goal is a reference a
   future reader can learn from, not a log of what was done.
7. Leave `Learning-Journal/` unchanged; it holds the older numbered CPP01–CPP06 notes.

If nothing genuinely new was learned, report that briefly and write nothing. Do not pad the
knowledge base with routine chronology.
