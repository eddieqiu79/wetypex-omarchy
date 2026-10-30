# Install or update from GitHub Releases

Target: Arch/Omarchy **x86_64**. Use this fork's release packages. AUR recipes retained from original WeTypeX are not this installer. The first published version is a preview candidate; do not interpret the existence of a package as stable desktop/device acceptance.

## Prerequisites

Inspect the target host using [the installation checklist](INSTALL.md) and `python3 tools/preflight.py`. Keep the existing input method available. You need Python 3.11+, Arch's `pacman`/`bsdtar`, and `github-cli` with `gh release verify` and `gh release verify-asset`. Resolve missing tools through the target's supported package update/install process, with the user's authorization. Do not use partial Arch upgrades or change Omarchy mirrors just to satisfy dependencies. GitHub CLI may require authentication; complete that directly with GitHub, never paste a token into an agent conversation.

## First installation

Clone and inspect the exact candidate source; record its SHA:

```sh
git clone --branch v0.1.0-rc.3 --single-branch https://github.com/eddieqiu79/wetypex-omarchy.git
cd wetypex-omarchy
git rev-parse HEAD
python3 tools/preflight.py
python3 omarchy/wetypex-omarchy-package download --channel preview --tag v0.1.0-rc.3
```

The helper first checks immutability, verifies the release attestation, downloads and verifies the manifest, then downloads and verifies the exact package. It additionally checks SHA-256, byte count, package name, architecture, epoch and version. Failed verification never proceeds to installation. Temporary downloads are removed on normal failure; interruption/forced termination can leave a `.wetypex-download-*` directory containing partial data, which is never treated as a verified release.

For one explicit verified-download-and-install operation, use a **new** directory (existing verified downloads are never overwritten):

```sh
python3 omarchy/wetypex-omarchy-package install --channel preview --tag v0.1.0-rc.3 --directory "$HOME/.cache/wetypex-omarchy/first-install" --confirm
```

Run as your ordinary desktop user. Only `sudo pacman -U` is elevated, and pacman still prompts interactively. Review dependency/conflict/removal proposals. No service is started and no running process is restarted. An unmanaged `~/.local/lib/fcitx5/wetypex.so` causes the helper to refuse installation until its migration is reviewed. Other unmanaged/custom prefixes still need explicit inspection; no universal migration detector is claimed.

The adapter excludes official runtime. Follow [Arch runtime and activation instructions](ARCH.md#3-prepare-the-official-runtime-and-input-method) after installation. Obtaining runtime, pairing and activating an input method are separate consent steps, not silently accepted by `--confirm`. Existing canonical state is retained.

## Installed-user updates

```sh
wetypex-omarchy-package check
wetypex-omarchy-package check --channel preview
wetypex-omarchy-package download --channel preview
# After reviewing the selected release and finishing active input/transfers:
wetypex-omarchy-package install --channel preview --confirm --directory "$HOME/.cache/wetypex-omarchy/next-install"
```

Omit `--channel preview` when following stable releases. Until the first stable release is published, stable reports `no-release`; it does not silently substitute the preview. Use `--tag vX.Y.Z` to select an exact release. These are package upgrades, not automatic upstream merges. No pacman repository is registered, so `pacman -Syu` alone does not discover this fork's new releases.

The settings window also checks and downloads verified packages. Installation remains an explicit terminal/pacman operation. Running programs keep their old code until restarted; confirm actual input and sync after your authorized restart. Do not restart during active composition or transfer.

## Retention and recovery

Each verified download is retained in its own tag directory, including its release manifest. Keep a working older package before updating. The helper refuses automatic downgrades; recovery requires an exact old tag, `--allow-downgrade`, `--confirm`, and a fresh destination directory. Dependencies/runtime/state must remain compatible. Read [recovery boundaries](UPDATES.md#recovery-and-first-rollout).

The release page also offers source, PKGBUILD, source/build manifests, dependency versions and checksums. GitHub-generated source ZIPs do not contain a built adapter or official runtime. Building from source remains available via [Arch packaging](ARCH.md).
