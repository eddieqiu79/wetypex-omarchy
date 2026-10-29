# Release records

For each proposed release, add `vMAJOR.MINOR.PATCH[-rc.N].md` before tagging.
Do not copy an old pass result into a new release record.

Required review content:

- Scope and user-visible changes; upstream commits integrated and patches retained.
- Exact supported runtime and supported target architecture/system versions.
- CI build/package install/remove results for the candidate.
- Real desktop activation, upgrade from the previous package, recovery and rollback results.
- Input, clipboard and file-transfer directions affected by the change; record failures and untested paths explicitly.
- State/configuration migration impact and restart requirements.
- Known issues, previous compatible package and recovery procedure.

`-rc.N` tags are opt-in previews. Stable publication requires completed relevant
acceptance testing, including an actual package upgrade; a successful CI build
alone does not justify stable status. Complete draft release notes with evidence
for the exact candidate after it is built. The workflow creates a draft only.
Enable repository immutable releases before publication, review the assets and
notes, then publish manually. Never delete/reuse a tag to repair a release;
publish a higher version and retain the old package for recovery.
