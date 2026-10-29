# WeTypeX Omarchy

A community-maintained adaptation of [WeTypeX](https://github.com/panxuc/fcitx5-wetypex) for Omarchy and Hyprland, focused on Chinese input and reliable interoperability with the official WeType clients on macOS and iOS.

The project builds on the upstream Fcitx5 integration and macOS engine compatibility layer. It adds clipboard reliability fixes, image format handling, an independent synchronization service, and a dedicated file transfer entry point.

Mac and iPhone users keep their existing official WeType clients. No additional application, agent, or background service is required on either Apple device.

The current source snapshot is based on upstream commit `291193a26938e0e8858d72b737066f37eb7a5f94` and targets Linux x86_64.

## Features

### Chinese Input on Linux

- Fcitx5 integration for the Omarchy / Hyprland desktop.
- Preserves the upstream input engine integration rather than replacing it with a new implementation.
- Supports configurable fuzzy pinyin, including `z/zh`, `c/ch`, `s/sh`, and `in/ing`.
- Runs clipboard synchronization and file transfers separately from the input method's interactive path.

The input method has been manually tested on Omarchy with Hyprland 0.56.2. Typing comfort feedback is subjective; candidate latency and input responsiveness under transfer load have not yet been systematically measured.

### Bidirectional Clipboard Synchronization

Text and image synchronization has been manually tested in all four directions:

| Direction | Text | Images |
| --- | --- | --- |
| Linux → macOS | Manually confirmed | Manually confirmed |
| macOS → Linux | Pasted on Linux | Pasted on Linux |
| Linux → iPhone | Received in the official WeType interface | Received in the official WeType interface |
| iPhone → Linux | Pasted on Linux | Pasted on Linux, including JPEG and PNG samples |

Reliability improvements include:

- Reading image data until EOF to prevent truncated clipboard images.
- Resolving encrypted image references and checking download completeness and content integrity.
- Retrying transient image download failures.
- Rechecking the local clipboard before applying remote content, so a newer local copy takes priority.
- Preventing received clipboard content from being immediately uploaded again.
- Establishing a startup baseline instead of uploading pre-existing clipboard content.
- Removing temporary image files and limiting persisted synchronization metadata.

On iPhone, receiving or accessing synchronized content may require opening the official WeType keyboard and its clipboard interface. Content appearing there does not necessarily mean it has been written to the iOS system clipboard.

### Image Format Compatibility

The image compatibility layer handles:

- PNG
- JPEG / JPG
- HEIF / HEIC
- AVIF
- WebP
- GIF
- TIFF
- BMP

Supported images are normalized to a static PNG representation for clipboard compatibility. Actual decoding support depends on the installed system image plugins. Some formats have only been checked with synthetic offline fixtures, not in every device-to-device direction.

This is image clipboard interoperability, not lossless file transfer: animation, multiple pages, metadata, and original file bytes are not guaranteed to be preserved.

### File Transfers

A dedicated transfer window supports sending and receiving ordinary files with paired official WeType clients over the local network.

Manual transfers have succeeded between Linux and both macOS and iPhone in both directions. These results do not establish byte-for-byte verification for every direction or complete coverage of cancellation, rejection, and timeout behavior.

**File transfer is separate from clipboard synchronization.** The project does not currently provide direct cross-device file copy and paste for `.md`, `.pdf`, or other files selected in a file manager. Copying a file path or URI does not automatically upload that file.

The Mac ↔ iPhone file copy-and-paste behavior observed during testing used Apple's native continuity features. It is not evidence of a Linux-compatible file clipboard protocol.

## Linux Integration

- An independent synchronization service allows clipboard sync to operate separately from the currently selected input method.
- A managed file transfer launcher provides a dedicated transfer window.
- A single canonical pairing identity is used rather than duplicated account directories.
- Service templates are included, but building or staging the project does not automatically enable services or replace an existing input method.
- Cross-device clipboard tools should be coordinated to avoid competing writes and ambiguous synchronization results.

## Installation Status

Use [Arch / Omarchy installation](docs/ARCH.md) for this fork's `PKGBUILD` exporter, package build and explicit user-service setup. The old upstream AUR recipe is not this fork's installer. Start with the read-only `python3 tools/preflight.py`. The package does not obtain official runtime, pair devices, or silently change the desktop. Clean desktop/device validation remains necessary; CI packaging alone is not end-to-end acceptance.

## Building and Offline Tests

The project retains the upstream CMake structure. Build dependencies include Fcitx5 development files, LibIME, Qt with WebEngine, Clang/libc++, Python 3, and cryptography. HEIF/AVIF conversion also requires PyGObject, GdkPixbuf, and the appropriate image format plugins.

A basic isolated build uses:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/lib/wetypex-omarchy" \
  -DCMAKE_INSTALL_LIBDIR=lib -DFCITX_INSTALL_USE_FCITX_SYS_PATHS=OFF
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Missing development dependencies must be supplied separately; these commands do not install system packages or activate the input method.

The source export was validated using an existing, verified dependency sysroot and system libcurl. The default bundled-curl configuration shown above was not the configuration revalidated for this export. See the validation record for the tested dependency versions and build settings.

Building and running the offline tests does not require an account, device pairing, or the proprietary runtime. Actual input and device interoperability require separately obtaining the runtime and using the official association workflow, subject to the applicable software terms.

## Validation Status

A [clean Arch package build and install/remove check](https://github.com/eddieqiu79/wetypex-omarchy/actions/runs/40857038300) passed for commit `b88e6e87988685ba811c9c04e8048293651e18bc`. See [Arch installation](docs/ARCH.md) for the artifact, checksums, and validation boundaries. This does not certify later commits or real desktop/device behavior.

The published source snapshot has passed:

- A complete local CMake build.
- Three offline regression suites covering synchronization behavior, image integrity, format decoding, and fragmented pipe input.
- Staged installation checks for the compatibility helpers.
- A source and Git metadata privacy review.

Earlier device testing confirmed successful text, image, and file transfers in all four Linux ↔ Apple-device directions. These are results from specific devices and software versions, not a guarantee for every environment or a fresh end-to-end retest of the exported source snapshot.

The official clients used during testing were WeType 3.0.0 (670) on Mac and 3.5.5 on iPhone.

Long-duration operation, sleep and wake recovery, network interruptions, transfer cancellation, and large-file performance still need broader validation. No fixed latency, resource usage, or zero-overhead claims are made.

## Documentation

The detailed engineering records below are currently in Chinese:

- [Changes and source provenance](docs/OMARCHY-CHANGES.md)
- [Build, operation, and shutdown notes](docs/OMARCHY-OPERATIONS.md)
- [Validation scope and outstanding checks](docs/OMARCHY-VALIDATION.md)
- [Original upstream README](docs/UPSTREAM-README.md)
- [Component licensing and official agreement review](LICENSES.md)

Machine-readable records are also included:

- [Upstream source manifest](docs/upstream-source-manifest.json)
- [Build dependency versions and checksums](docs/build-dependencies.json)
- [Offline regression results](docs/source-export-validation.json)

## Independent Project and Rights Notice

WeTypeX Omarchy is an independent, unofficial compatibility project. It is not affiliated with, endorsed by, or authorized by Tencent or its affiliates. Names and trademarks belonging to Tencent or other rights holders are used only to identify the software and services with which this project seeks compatibility.

This repository distributes adaptation source code and synthetic test data; it does not distribute the official WeType engine, installation package, dictionaries, models, or extracted interface resources. Running the project requires separately obtaining the relevant official runtime. Review and comply with the applicable [official WeType agreement](https://wetype.weixin.qq.com/web/agreement) and third-party licenses before obtaining or using that software or its services.

The project does not claim that its open-source licenses or free distribution grant permission to use, modify, reverse engineer, or redistribute proprietary software, or to access official services. Compatibility and service availability may change. The software is provided without warranty as described in the applicable component licenses. Nothing in this notice overrides those licenses or any rights and obligations under applicable law. See [LICENSES.md](LICENSES.md) for sources and limitations.

## Upstream and Licensing

This repository uses component-specific licensing; it is not exclusively MIT or a blanket relicensing of all upstream files to GPL. It retains WeTypeX's upstream attribution and license notices. Compatibility modules derived from [VType Linux](https://github.com/zhl-HFUT/vtype-linux) retain their applicable GPLv3 licensing.

- WeTypeX's own adaptation code and corresponding patches: MIT; see [LICENSES/WeTypeX-MIT.txt](LICENSES/WeTypeX-MIT.txt) and [NOTICE](NOTICE).
- VType image compatibility components and their Python adapters: GPLv3; see [compat/LICENSE](compat/LICENSE).
- Additional Omarchy tools, integration files, and tests: GPL-3.0-only.
- Tencent's proprietary software, resources, and services are not covered by these open-source licenses.

The repository contains adaptation source code and synthetic test fixtures. It does not distribute proprietary WeType executables, dictionaries, language models, extracted interface resources, pairing identities, or private user data.

WeTypeX Omarchy is an independent, unofficial project. It is not developed or endorsed by Tencent, and no Tencent authorization is claimed. Free and open-source distribution of the adaptation code does not grant rights to Tencent's proprietary software or services. The official agreement includes restrictions concerning unauthorized intellectual property use and reverse engineering.

The original upstream README also contains personal-study and 24-hour deletion language alongside its MIT license notice. That wording is preserved in the archived upstream README and is not treated as a legal exemption or authorization.

See [LICENSES.md](LICENSES.md) for component-specific licensing and the official software agreement review. Free distribution does not add a noncommercial restriction to the applicable MIT or GPL licenses.
