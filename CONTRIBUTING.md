# Contributing to WeTypeX Omarchy

Thank you for helping improve this unofficial compatibility project. Contributions should address a specific, reproducible problem and preserve upstream provenance and component licenses.

## Before you start

- Read the [README](README.md), [license and provenance notes](LICENSES.md), and [upstream notice](NOTICE).
- For substantial changes, describe the intended behavior and scope in an issue or draft pull request so maintainers can discuss the approach. This is guidance, not a requirement for small fixes.
- Distinguish Linux integration problems from compatibility problems with a particular official runtime version and changes in external services.
- Prefer focused patches that preserve the upstream module structure. Explain changes to engine ABI assumptions, supported runtime versions, and failure behavior.
- Preserve existing input methods and user data. Installation, enabling services, changing defaults, restarting Fcitx, modifying firewall rules, and pairing should be explicit actions rather than hidden effects of a build or test.

## Rights, privacy, and test data

- Do not commit or attach official installation packages, extracted executables, dictionaries, models, proprietary interface resources, or other material you lack permission to distribute. Obtain any required runtime separately and review the official terms.
- Retain copyright and license notices. The repository contains MIT and GPL components; see [LICENSES.md](LICENSES.md) before copying or modifying code. Identify the source and license of new third-party material.
- Never include account identities, private keys, tokens, pairing or transfer codes, signed URLs, clipboard contents, private messages, or personal files in issues, pull requests, logs, fixtures, or screenshots.
- Use synthetic text, images, and files for tests. Sanitize diagnostic output before sharing it, including usernames, device identifiers, local paths, and network addresses where appropriate.
- Do not report credentials or exploitable vulnerabilities in a public issue. Follow [SECURITY.md](SECURITY.md) and use private vulnerability reporting where available.
- Do not assume that a disclaimer or an upstream open-source license grants rights to Tencent's software or services.

## Development and build

The current validated target is Linux x86_64 with Fcitx5; the maintenance machine used Omarchy and Hyprland. Consult the README for dependencies and tested build limitations. A basic isolated build and offline test run is:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/lib/wetypex-omarchy" \
  -DCMAKE_INSTALL_LIBDIR=lib -DFCITX_INSTALL_USE_FCITX_SYS_PATHS=OFF
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

These are source-build instructions, not a validated one-command installer. The published export was tested with a verified dependency sysroot and system libcurl; the default bundled-curl combination above was not independently revalidated. Missing development dependencies, a suitable compiler and libc++, Qt image plugins, and other distribution-specific requirements may need separate setup. Consult the [validation record](docs/OMARCHY-VALIDATION.md) and [upstream documentation](docs/UPSTREAM-README.md).

To inspect installation contents without installing into the configured prefix:

```sh
DESTDIR="$PWD/stage" cmake --install build
```

The stage is not a relocatable installation: upstream executables may contain the configured final prefix. Building and offline tests do not require an account or official runtime. Actual input and interoperability require an independently obtained official runtime, explicit setup, and pairing; follow the applicable terms and record which operations were actually performed.

## Testing and evidence

Choose checks relevant to the change and describe their results accurately:

1. **Source and offline tests:** build affected targets and run relevant CTest checks. For clipboard changes, cover interruption, integrity, retries, and ordering of local and remote updates where relevant.
2. **Local desktop behavior:** identify the distribution, architecture, compositor, Fcitx version, application type, and whether the test used Wayland or XWayland.
3. **Official-device interoperability:** state official client versions and test each relevant direction separately. A service starting or a successful CLI response is not proof that another device received or saved data.
4. **Daily-use behavior:** where relevant, check reconnect, sleep and wake, coexistence with clipboard managers, and responsiveness during transfers. Clearly mark cases that were not tested.

For images, distinguish receipt in the iPhone WeType interface from availability in the iOS system clipboard. For files, distinguish an invitation from acceptance, an established data connection, and a completely saved file. Compare hashes when both endpoints make the original bytes available; otherwise state the level of verification.

Direct cross-device file copy and paste is not currently implemented. Do not treat a copied file path, a file-transfer window, or Apple-native continuity between Mac and iPhone as proof that this feature works on Linux.

## Coding and design expectations

- Follow the existing style and naming in the file you change. C++ standards vary by target; do not assume every target is compiled with the same language standard.
- Keep input responsive when synchronization or file transfers fail. Avoid blocking the input path on network operations.
- Preserve a single pairing identity and coordinate state writers and lock ownership. Do not duplicate private identity directories to work around concurrency.
- Do not expose sensitive responses in logs. Do not mark an operation successful before the relevant outcome has actually occurred.
- Preserve clipboard source, deduplication, acknowledgement, and local-content priority behavior. Do not automatically upload files merely because a clipboard contains a path or URI.
- For UI changes, identify the platform and version used as a reference and explain intentional differences.

## Issues and pull requests

A useful report or pull request includes:

- The problem, steps to reproduce it, and expected and observed behavior.
- The repository commit, distribution and version, CPU architecture, compositor, Fcitx version, and relevant official client/runtime versions.
- The affected direction and content type for synchronization or transfer issues, using only synthetic examples.
- Checks actually run and their outcomes, including failures and untested scenarios.
- Changes to dependencies, third-party material, license notices, compatibility assumptions, installation behavior, or user-visible configuration.

Do not include private logs or files. If you cannot test a scenario, say so; untested work can still be reviewed without presenting it as a completed end-to-end result.
