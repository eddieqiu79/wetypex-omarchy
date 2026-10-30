# Release and update channel

## Current distribution policy

Use **immutable GitHub Releases from `eddieqiu79/wetypex-omarchy`**, containing reviewed Arch adapter packages. The current native-desktop release candidate is `0.1.0-rc.2`; it includes the phone-handshake and local-runtime diagnostics fixes added after `0.1.0-rc.1`. Stable remains separate and excludes candidates. Publishing a package does not establish a stable desktop/device acceptance result.

See [Release installation](RELEASE-INSTALL.md) for executable download/install/update commands. The installed settings GUI also checks and downloads verified packages. No root daemon, automatic runtime download, upstream binary replacement, clipboard service activation or Fcitx restart is performed by the update client. `pacman -Syu` alone does not discover this fork's releases: no pacman repository is registered.

Release assets are persistent distribution points; CI artifacts expire after 14 days and are for review. Immutable releases lock published tags/assets and provide GitHub release attestations. See [GitHub's immutability documentation](https://docs.github.com/en/code-security/concepts/supply-chain-security/immutable-releases) and [verification commands](https://docs.github.com/en/code-security/how-tos/secure-your-supply-chain/secure-your-dependencies/verify-release-integrity).

## The two maintenance loops

1. **Original upstream → maintained fork:** `upstream-watch.yml` checks the original WeTypeX head weekly and on demand against `docs/upstream-source-manifest.json`. It records SHAs and comparison links in Actions, without automatically merging, opening a PR or publishing. A maintainer reviews changes, preserves local fixes/licensing, updates provenance and runs relevant checks. Scheduled Actions may be delayed/disabled on inactive repositories; monitor them and use manual dispatch when needed.
2. **Maintained fork → installed users:** published stable/preview packages are discovered and verified, then installed through an explicit pacman transaction. Only this fork's releases are selected. Upstream binaries never overwrite this fork. Continued updates require maintainer review and release work; a channel is not a promise of indefinite maintenance.

The proprietary runtime is independently pinned. A changed runtime requirement produces `runtime-review-required`; the package client refuses it pending migration review. Device identities, dictionaries and runtime acquisition are outside adapter updates.

## Discovery, download and installation

```sh
fcitx5-wetypex-update --check
fcitx5-wetypex-update --check --channel preview
wetypex-omarchy-package check --channel preview
wetypex-omarchy-package download --channel preview
```

Checks are explicit, read-only network requests. No-argument legacy updater calls remain silent no-ops; `--status` describes policy and `--force` cannot restore upstream downloads. Stable excludes prereleases; preview includes stable and candidates and selects the highest supported version. Network/rate-limit failure never means up to date. A newer installed version is not silently downgraded.

An immutable API flag is **not** cryptographic verification: checker results explicitly report `cryptographically_verified: false`. The package client additionally runs `gh release verify` and `gh release verify-asset` for the downloaded manifest/package, checks their identity, runtime, SHA-256, size and `.PKGINFO`, then makes the completed download available. An invalid or unsupported verifier stops the operation. It does not use signed URLs in diagnostics.

The helper needs Python 3.11+, Arch's `bsdtar`/pacman, and GitHub CLI supporting the release verification subcommands. Configure any GitHub CLI authentication directly, never through a shared token in chat. Verified assets may also be downloaded and inspected manually using GitHub CLI and `pacman -Qip`/`pacman -Qlp`; do not skip attestation verification.

Installation requires `install --confirm` and uses interactive `sudo pacman -U` on exactly one verified package. Dependency and conflict proposals remain visible. Current processes are not restarted. Private-prefix installations require migration review and must not be silently layered with the system package. See [the command guide](RELEASE-INSTALL.md) for separate first-install, update and recovery examples.

Downloads use private temporary directories, then move the complete verified pair to a new tag directory. Existing directories are retained, never overwritten. Normal failures clean up temporary files; forced process termination can leave an incomplete `.wetypex-download-*` directory. It is never treated as a verified download.

## Maintainer publishing workflow

1. Use a clean reviewed commit with `VERSION` set to `MAJOR.MINOR.PATCH-rc.N` or `MAJOR.MINOR.PATCH`. Development snapshots cannot become releases. Preserve Arch epoch 1. Every changed/rebuilt public package needs a new version/tag.
2. Add `docs/releases/vVERSION.md` with scope, evidence, compatibility and limitations. Create the matching tag on this fork's main history; do not move existing tags.
3. Run **Prepare reviewed release draft** with that tag. It checks tag/version/main ancestry, runs release-policy checks, invokes the clean Arch workflow for the exact SHA, validates bundle contents/metadata and creates a draft only.
4. Inspect package/source contents, licensing/privacy, recipe, dependency list, manifests and checksums. No official engine, credentials or user test files belong in the assets. Clean Arch CI exercises install, upgrade from the recorded development baseline, rollback/re-upgrade, remove, and synthetic user-state preservation. These are package transactions within the same dependency environment, not desktop/runtime acceptance.
5. Confirm immutable releases are enabled. A preview can be published with its outstanding validation explicitly stated. **Stable publication requires the exact candidate's real desktop/device, restart, upgrade and recovery acceptance**, recorded in release notes. The workflow does not certify these from container tests.
6. Publish the reviewed draft with the correct prerelease flag and verify its release/asset attestations. Incorrect public assets are superseded by a higher version, never replaced. Keep prior packages for recovery.

The draft workflow never publishes automatically. It has pinned checkout/artifact actions, read-only build permissions and contents-write only for draft upload. Upstream integration and final stable approval remain maintainer responsibilities.

## Recovery and first rollout

Retain a previous working compatible package and manifest. The helper requires an exact old tag, `--allow-downgrade` and `--confirm` for downgrade; use a new destination directory. A retained pre-release development package can instead be installed by exact path through pacman after inspection. Downgrading the adapter is not a whole-system rollback and cannot fix incompatible runtime, dependency or user-state migrations.

This candidate introduces no user-state schema migration. Package updates preserve identities, runtime, dictionaries and preferences. Future migrations need explicit versioning, scoped backup and recovery testing. Existing unmanaged installations need one reviewed migration; existing installed packages need an explicit upgrade to obtain the new checker/client. Processes already running remain on old code until an authorized restart.

## Validation boundaries

Offline tests cover channel/version ordering, immutable manifest identity, runtime mismatch, safe redirects, sanitized network failure, release bundle provenance, attestation command ordering, tampered package rejection, partial-download cleanup, no implicit installation, and retained download directories. GUI tests use isolated state and synthetic backend events. Exact CI and release verification results belong to the tagged SHA, not an unqualified future branch head.

Fresh-machine activation, real GUI input and all affected official-device directions, long-duration behavior, sleep/network recovery and an installed-desktop rollback remain separate acceptance layers. See [native desktop behavior](DESKTOP.md) and the specific [release record](releases/v0.1.0-rc.2.md).
