# Changelog

## 0.1.0-dev — development work

### Desktop interaction prototype

- Add a standalone Qt Widgets demo with resizable native windows, keyboard navigation, a dark `#282827` background and `#24C891` accents.
- Retain the source-backed input, fuzzy-pinyin, voice, phrase, appearance, shortcut and device settings in an in-memory prototype.
- Add device selection, multi-file picking/drop, selection deduplication, invitation gating, independent simulated tasks, cancellation/retry and recent history.
- Keep the demo out of the default installation. It does not read real configuration or pairing, transfer data, or replace the installed GUI. See [demo instructions](docs/DESKTOP-DEMO.md).

### Update and release infrastructure

- Replace the inherited upstream binary installer with a fork-specific, explicit metadata checker. Background/legacy invocations cannot download or install upstream binaries.
- Add an asynchronous stable-release check and upgrade-guide link to the existing settings application; installation remains a separate, authorized package-manager operation.
- Separate stable and opt-in release candidates; reject mutable releases, unexpected metadata and incompatible runtime requirements without installing anything.
- Add exact-tag draft release preparation, Arch package/source manifests and checksums, and a weekly read-only upstream review report. No workflow automatically merges upstream changes or publishes a release.
- Document immutable release verification, manual upgrades, recovery boundaries and first-rollout requirements. No stable release or general upgrade/rollback guarantee is implied by adding this infrastructure.

### Validation scope

The demo and release/update policies have isolated regression coverage. Full adapter build checks, CI results and historical device tests remain separate evidence. New GUI backend integration, first immutable release, real package upgrade/rollback and fresh end-to-end device validation remain outstanding.
