# Contributing

Keep changes focused and describe the behavior being changed. Include the build or test command used, its result, and any external fixture requirements.

For reconstructed behavior, record the research binary hash, native addresses, relevant evidence, and unresolved assumptions. Distinguish observations from decompiler hypotheses and host-side adaptations. Do not treat a successful compile or a passing isolated test as proof of whole-server equivalence.

Preserve existing provenance and partial-implementation notes. Update documentation when a limitation is resolved, and add a focused regression when it meaningfully distinguishes the corrected behavior.

Commit source, reproducible tools, and useful evidence. Keep local configuration, credentials, compiled binaries, runtime data, crash dumps, and analysis databases out of commits. Generated parameter definitions and the curated `docs/evidence/` text files belong in version control.
