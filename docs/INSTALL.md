# Installation and agent handoff

## Status

This is a source distribution, not a verified one-command installer. The existing `packaging/aur/PKGBUILD` downloads upstream WeTypeX releases and **does not install this fork's changes**. Other upstream package recipes are retained for reference and have not been validated as fork-specific releases. Do not substitute an upstream package and claim this fork is installed.

The maintenance machine validated an isolated build and three offline regression suites, plus specific desktop and device interactions recorded in [OMARCHY-VALIDATION.md](OMARCHY-VALIDATION.md). A clean-machine installation, portable package, general migration, and rollback have not yet been validated. The documented sample build is not identical to the previously tested sysroot/system-curl build.

## Before making changes

1. Read [AGENTS.md](../AGENTS.md), [README.md](../README.md), [LICENSES.md](../LICENSES.md), and the upstream [NOTICE](../NOTICE). Record the checkout commit (`git rev-parse HEAD`) and check for uncommitted changes.
2. Run `python3 tools/preflight.py`. It checks architecture, tool availability, selected package metadata and session-variable presence; it does not read clipboard data, account state, or personal configuration, and it cannot verify desktop connectivity.
3. With user authorization as needed, inspect the target desktop's actual Fcitx configuration, active input method, relevant user services, Wayland/XWayland, clipboard managers, and installed packages. Keep the existing input method and data available as a fallback.
4. Identify missing dependencies and their sources. The source build needs Fcitx5 and LibIME development files, Qt and WebEngine development files, Clang/libc++, CMake, a C/C++ toolchain, Python, cryptography, and further dependencies documented upstream. HEIF/AVIF conversion needs PyGObject, GdkPixbuf, and appropriate decoders. Do not assume packages, versions, or compatible ABI from tool names alone. See [build-dependencies.json](build-dependencies.json) for the previous build's four signed dependency packages; it is a historical record, not a complete lockfile.

## Isolated preparation

Use the source-build example in the [README](../README.md), select an explicit prefix, and inspect CMake's chosen Fcitx installation directories before installing. Build and run `ctest --test-dir build --output-on-failure`. Resolve build errors against the actual system instead of silently upgrading unrelated packages or using the old machine's paths. The fixed libkqueue commit and bundled curl inputs are specified in CMake; inspect all fetched dependencies and their licenses.

To inspect installation output without writing to the live prefix, use `DESTDIR="$PWD/stage" cmake --install build`. Check that the Fcitx plugin, Python image adapters, compatibility modules, service templates, MIT notice, GPL text, and dependency notices are present. `DESTDIR` is a staging root, not a portable installation: binaries and generated files can embed the configured final prefix.

## Runtime and activation require explicit scope

The source repository does not contain the official engine, dictionary, models, or extracted UI. The upstream setup tool supports an explicitly supplied archive or an explicit download with agreement acceptance; its extraction mechanism and fixed version are not a grant of Tencent rights. The user must decide whether to obtain and use official assets under the applicable terms. Do not read the user's existing device credentials or send pairing codes to an agent.

This repository currently provides a sync-service **template**, not an activated service. A proper installation must place and configure the service for its chosen prefix and ensure only one sync owner. Ask before enabling a user service, modifying Fcitx configuration, restarting Fcitx, disabling another clipboard tool, or changing firewall settings. Pair only through the official association flow, with one canonical Linux identity; do not clone identity state.

## Verification and recovery

Report build/test results separately from local desktop testing and official-device tests. Verify actual Chinese input; text and images in each Linux ↔ Mac/iPhone direction; and explicit file-transfer invitation, acceptance, connection, and saved result. Use synthetic data; compare hashes when both sides allow access. iPhone keyboard availability is not automatically the same as the iOS global clipboard. Direct file copy/paste is not implemented.

There is currently no general, tested uninstall or rollback command. Before installing anything, record the package owner and paths, original input method selection, service states, and any planned backups; obtain permission for changes. Remove or restore only artifacts created or modified by the installation, and preserve account and dictionary data unless the user explicitly requests otherwise. Existing maintenance-machine commands are described in [OMARCHY-OPERATIONS.md](OMARCHY-OPERATIONS.md); they are not universal instructions for a fresh machine.

## Remaining release work

A fork-specific package recipe must use this repository's pinned source and verified checksum, a distinct package name and conflict policy, complete runtime/build dependencies and licenses, a safe install/upgrade/uninstall lifecycle, and no proprietary assets. Exercise the resulting package on clean Omarchy/Arch systems, including rollback, desktop activation, official-device interaction, and sleep/network recovery before presenting it as a supported release. A source-only GitHub Release or CI pass does not by itself prove those scenarios.
