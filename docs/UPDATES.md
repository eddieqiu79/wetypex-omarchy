# Release and update channel

## Decision and current readiness

Use **immutable GitHub Releases from `eddieqiu79/wetypex-omarchy`**, containing reviewed Arch adapter packages. Stable is the default channel; `-rc.N` previews require an explicit opt-in. Installation remains a reviewed pacman operation. No separate update server, resident updater, root daemon, automatic upstream binary replacement, or automatic runtime download is required.

This source revision supplies release-draft preparation, an explicit read-only checker, a settings-window check button, and an upstream review workflow. It does **not** establish an operational stable channel by itself. At the 2026-10-29 check, the public repository had no Releases. The release workflow has not yet produced an accepted release, and fresh-machine upgrade/rollback acceptance remains outstanding. Existing installations keep their old updater until they actually install a build containing this change.

CI artifacts expire after 14 days; they remain build-review material. Release assets are the intended persistent distribution point. Enable GitHub immutable releases before the first publication: published assets and tags then cannot be replaced, and GitHub supplies a verifiable release attestation. See [immutable releases](https://docs.github.com/en/code-security/concepts/supply-chain-security/immutable-releases) and [verification](https://docs.github.com/en/code-security/how-tos/secure-your-supply-chain/secure-your-dependencies/verify-release-integrity).

## The two maintenance loops

1. **Original upstream → maintained fork:** `upstream-watch.yml` checks the original WeTypeX head weekly and on demand against `docs/upstream-source-manifest.json`. It records both SHAs and a comparison link in the Actions summary and emits a warning on drift. It does not merge, open a PR, publish a release, or modify a desktop. A maintainer must review the report, merge suitable changes, preserve local fixes and licensing, update the provenance record, and run affected regressions/device tests. Scheduled Actions may be delayed or disabled on inactive repositories; maintainers must monitor them and can run the workflow manually.
2. **Maintained fork → installed users:** users discover this fork's published stable/preview versions, verify exact release assets, then upgrade its Arch package. Original upstream binaries must never overwrite this fork. An update channel cannot guarantee ongoing maintenance if nobody reviews and releases upstream changes.

The proprietary runtime is a third independently pinned layer. A changed runtime requirement produces `runtime-review-required`, not an ordinary install recommendation. Device pairing, dictionary data and runtime acquisition are not part of an adapter update.

## Maintainer publishing workflow

1. Finish changes in a clean reviewed commit. Choose `VERSION` as `MAJOR.MINOR.PATCH-rc.N` for a preview or `MAJOR.MINOR.PATCH` for stable. `-dev` builds cannot become releases. Preserve Arch epoch 1. Every replacement build needs a new project version/tag; do not reuse an old release version even for a dependency rebuild.
2. Add `docs/releases/vVERSION.md` with changes, known limitations, compatibility, upgrade/recovery expectations and test evidence. See [release records](releases/README.md). Create a matching reviewed tag whose commit is on this fork's main history. Do not move an existing tag.
3. Run **Prepare reviewed release draft** with that tag. It verifies tag/VERSION/clean-checkout consistency and main ancestry, runs release policy tests, calls the existing clean Arch build/install/remove workflow on the exact SHA, validates package metadata and source checksum, and creates a **draft only**. Checkout and artifact actions are pinned. Build jobs have read-only repository permission; only the final draft-upload job has contents-write permission.
4. Inspect the adapter package, source archive, recipe, dependency list, source manifest, `release-manifest.json`, SHA256SUMS and release notes. No official engine, identities, clipboard payloads or user test files belong in the assets. The bundle tool rejects unexpected top-level assets; source/privacy and license review is still required for the package contents.
5. Exercise the exact candidate package on a target desktop: upgrade, input, affected synchronization/file directions, restart handling and rollback. Add evidence to the draft notes. Stable publication requires these checks; the workflow deliberately cannot certify a desktop from a container test.
6. Confirm immutable releases are enabled and publish the draft manually. Previews stay marked prerelease. Verify the published release and package with GitHub CLI. A failed/incorrect release is superseded by a higher version, never silently replaced. Keep prior packages available for recovery.

The workflow does not automatically publish, create tags, modify repository security settings or approve its own test claims. If draft generation fails, fix the candidate and inspect any existing draft before rerunning; uploads are not clobbered automatically.

## User discovery

In a build containing this revision, use the settings window's **Check stable updates** action or:

```sh
fcitx5-wetypex-update --check
fcitx5-wetypex-update --check --channel preview
```

Checks are explicit network requests for release metadata only. They do not download/install packages, change files, pair devices, restart Fcitx, or read clipboard/identity data. No-argument calls from legacy addon configurations are silent no-ops. `--status` describes the local policy; legacy `--force` prints guidance and cannot restore upstream downloads. `WETYPE_UPDATE_API` / `WETYPE_UPDATE_DIR` cannot redirect this checker.

Results distinguish `no-release`, `available`, `current`, `installed-newer`, `installed-version-unknown`, `runtime-review-required`, `unsupported-architecture`, `release-not-immutable` and `check-failed`. Failed or rate-limited requests never mean “up to date.” Stable excludes prereleases; preview includes stable and release candidates and chooses the highest supported version. It does not automatically downgrade a newer installation. Current means the checker/settings build version matches the release version; it does not certify that every resident process has restarted into it.

The checker validates repository, tag, channel, architecture, package metadata and runtime compatibility. An immutable API flag is **not** cryptographic verification: the output explicitly says `cryptographically_verified: false`. Verify downloaded bytes separately before installation.

## Verified package upgrade

Use a GitHub CLI version supporting `release verify` and `release verify-asset`. If unavailable, stop and arrange the supported tool version rather than skipping verification. These commands use GitHub's release attestation; checksums alone are not a separate authenticity mechanism.

1. Choose the exact tag reported by the checker. Read its notes and record the current package, runtime and activation state. Identify private-prefix installations separately; do not layer a system package over an unmanaged running deployment.
2. Download the exact package and accompanying manifests into a new directory through that release's page or `gh release download TAG --repo eddieqiu79/wetypex-omarchy`. Replace `TAG` with a reviewed immutable tag; do not use shell input received from unvalidated release text.
3. Run `gh release verify TAG --repo eddieqiu79/wetypex-omarchy`. Run `gh release verify-asset TAG FILE --repo eddieqiu79/wetypex-omarchy` for the package, `release-manifest.json`, and `SHA256SUMS`. Verify other assets too if using them. Require successful verification, then check the complete bundle with `sha256sum --check SHA256SUMS`.
4. Inspect the exact package with `pacman -Qip` and `pacman -Qlp`. Check the expected package name, architecture, epoch/version, dependency compatibility and installation conflicts against the manifest and system. An authenticated package is not automatically compatible with a partially upgraded Arch system.
5. Preserve the prior compatible package. Finish active composition/transfers; obtain the authorized installation/restart scope. Install the **single exact package path** using `sudo pacman -U /path/to/PACKAGE.pkg.tar.zst`. No unattended `--noconfirm` command is supplied for a user's desktop.
6. Restart affected processes/services at the agreed time. Package replacement does not magically update running processes. Verify the installed build manifest, input and relevant device directions before removing retained recovery material.

This is a stable distribution/discovery design with explicit installation, not an unattended auto-installer. There is no pacman repository registered yet, so `pacman -Syu` alone will not discover this package's releases. A signed pacman repository is a possible later step after versioning, upgrade and recovery have been exercised; it adds hosting and signing-key lifecycle responsibilities. An AUR package could deliver reviewed source builds later, but must track this fork rather than upstream binaries.

## Recovery and first rollout

Keep the previous package and its provenance/compatibility information. Rollback uses the retained exact package with pacman after checking dependency/state compatibility; it is not a rollback of the whole system. A downgrade cannot repair an incompatible state migration automatically. Updates must preserve identities, user dictionaries and settings. Any future migration needs versioned state, a scoped backup and a tested recovery plan before publication.

First rollout requires the reviewed channel changes on the default branch, enabling immutable releases in repository settings, preparing a candidate tag and notes, running the draft workflow, target upgrade/rollback/device acceptance, and manually publishing the first verified preview. Stable follows evidence, not a deadline. Existing users need one explicit upgrade to a build containing the new checker; it cannot bootstrap itself into installations that never had it.

## Validation of this implementation draft

Locally, updater inert-mode tests and release-channel tests cover no-release results, stable/preview filtering, version ordering, immutable/source-identity checks, runtime mismatch, sanitized network failure, tag restrictions and release-bundle allowlisting/checksums. Arch `vercmp` confirms development < release candidate < stable for the new version mapping. A live read-only query returned `no-release`; the upstream watcher detected source drift and did not merge it.

The complete adapter also built locally using the previously verified dependency sysroot and system libcurl; all three CTest suites passed. This is an isolated source build, not a clean Arch package or live upgrade test. Workflow syntax parsing and local tests are not a completed GitHub release run. Release attestations, published-asset verification, real upgrades/downgrades, and the new settings button in a live installed build remain separate validation steps. Check GitHub Actions for package results tied to the exact published commit.
