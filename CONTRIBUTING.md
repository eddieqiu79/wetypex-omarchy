# Contributing to WeTypeX Omarchy

**English** · [简体中文](CONTRIBUTING.zh-CN.md)

Contributions should solve a concrete problem, preserve upstream provenance and include enough evidence for someone else to review the result. Small fixes can go straight to a pull request. For changes to architecture, runtime compatibility or installation behavior, open an issue or draft PR to discuss the approach first.

## 1. Understand the project

Read the [README](README.md), [component licenses](LICENSES.md), [upstream notice](NOTICE) and [agent instructions](AGENTS.md). The maintained target is Arch / Omarchy x86_64 with Fcitx5 and Hyprland. Other environments need their own validation.

Identify which part is affected: Linux desktop integration, the pinned official runtime, or an external service. Keep patches focused and follow the existing module structure. Changes to engine ABI assumptions, dependencies or supported runtime versions need an explicit explanation.

Preserve existing input methods, settings and user data. Installing packages, enabling services, changing defaults, restarting Fcitx, changing firewall rules and pairing devices must be explicit setup actions, not side effects of a build or test.

## 2. Build in isolation

Record the checkout and inspect the environment:

```sh
git rev-parse HEAD
git status --short
python3 tools/preflight.py
```

Use the [README build commands](README.md#build-from-source). They select Qt6 and system libcurl, matching the Arch recipe. Check [package dependencies](packaging/arch/PKGBUILD.in) before building; preflight reports availability but does not install missing dependencies or prove desktop connectivity. The [Arch guide](docs/ARCH.md) covers source exports and package builds.

To inspect installation output without writing to the configured prefix:

```sh
DESTDIR="$PWD/stage" cmake --install build
```

A staged tree is not a portable installation: generated files and binaries can contain the configured final prefix. Offline tests need no account or official runtime. Actual input and cross-device tests need separately prepared runtime and official pairing. Never duplicate identity directories to create parallel clients.

## 3. Test the behavior you changed

| Layer | What to check | What to report |
| --- | --- | --- |
| Build and offline tests | Build affected targets; run relevant CTest suites with synthetic inputs. | Exact commit, commands, results and failures. |
| Linux desktop | Use the affected applications in the target session. | Distribution, architecture, compositor, Fcitx version, Wayland/XWayland and application type. |
| Official devices | Test each affected direction and content type separately. | Client/runtime versions, what arrived, and what could actually be pasted or saved. |
| Daily use | Where relevant, test reconnect, sleep/wake, concurrent clipboard tools and transfers during typing. | Observed failures and scenarios not tested. |

For clipboard changes, cover incomplete reads, integrity failures, retries, duplicate handling and ordering between local and remote copies where relevant. For images on iPhone, distinguish content visible in WeType from content available in the system clipboard.

For files, distinguish an invitation, acceptance, a data connection and a completely saved file. Compare hashes when both endpoints expose the original bytes; otherwise state the verification limit. A running service or successful CLI response does not prove that the other device received the data.

Direct cross-device file copy/paste is not implemented. A copied path, a transfer window or Apple continuity between Mac and iPhone does not establish Linux file-clipboard support.

## 4. Keep runtime behavior predictable

- Follow the style of the file you edit. C++ language standards differ by target.
- Keep network waits out of the interactive input path. Sync and transfer failures must not block typing.
- Use one canonical pairing identity. Coordinate state writers, locks and renewal instead of copying private state or weakening locks to bypass contention.
- Preserve clipboard origin, deduplication, acknowledgement and local-content priority. Do not upload files merely because a path or URI appears on the clipboard.
- Report success only after the relevant outcome. Keep sensitive responses out of logs.
- For UI work, identify the reference platform/version and explain intentional differences. Use synthetic state for screenshots and automated tests.

## 5. Protect data and preserve licenses

Use synthetic text, images and files. Before sharing diagnostics, remove account identities, private keys, tokens, pairing/transfer codes, signed URLs, clipboard contents, private messages and personal files. Also check screenshots and local paths for usernames, device identifiers and network details that are unnecessary to the report.

Do not commit official installers, extracted executables, dictionaries, models or proprietary interface resources. Obtain required runtime separately and review its terms. Retain copyright and license notices; identify the source and license of any new third-party material. See [LICENSES.md](LICENSES.md) for the MIT/GPL component boundaries. A disclaimer or an adapter's open-source license does not grant rights to Tencent's software or services.

Report exploitable vulnerabilities and credential exposure privately, following [SECURITY.md](SECURITY.md). Use GitHub's private vulnerability reporting where available; do not place secrets in a public issue.

## 6. Submit a useful issue or pull request

Include:

- **Problem:** steps to reproduce, expected behavior and actual behavior.
- **Environment:** commit or release, distribution, architecture, compositor, Fcitx and relevant official client/runtime versions.
- **Scope:** affected direction and content type; for example, iPhone → Linux JPEG clipboard or Linux → Mac file transfer.
- **Change:** the resulting behavior and any dependency, license, installation or configuration changes.
- **Validation:** checks actually run, their results, known failures and untested cases.

Keep the description about the final patch. Link related issues and explain non-obvious tradeoffs. Untested work can still be reviewed when the missing evidence is stated clearly. Do not attach raw private logs or user files to fill that gap.

For release work, follow [the publishing workflow](docs/UPDATES.md#maintainer-publishing-workflow). Published assets are immutable; changed packages need a new version. Offline or container checks alone do not qualify a candidate as a stable desktop release.
