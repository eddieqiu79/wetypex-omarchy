# Installation and update implementation (unreleased)

This document describes the working-tree changes after `v0.1.0-rc.4`, not an available release. The current installed/public rc.4 does not contain these new entry points. No stable acceptance is implied.

## Roles and entry points

| Purpose | Entry point | Scope |
| --- | --- | --- |
| Ordinary use, including the maintainer's daily installation | Settings → upgrades, or `wetypex-omarchy-package` | Check/download/verify/install published packages |
| First package installation or migration from rc.4 | Release asset `wetypex-install.pyz` | Same download/verification core, then visible interactive pacman transaction |
| Installed-app onboarding | Settings → installation guide | Explicit runtime preparation, input configuration, sync activation and device page |
| Local development | `tools/develop.py` | Build/test/isolated preview only |
| Release preparation | `prepare-release.yml` | Exact reviewed tag, declared Arch archive snapshot, tested package, draft only |

Stable and preview are independent of developer identity. A local build is labeled as a development build and has no production update controls. Arch packages explicitly build with `WETYPE_DEVELOPMENT_BUILD=OFF`. The maintainer's daily installation uses the same updater as everyone else. No source tree or developer home path is required by the installed application.

## Shared updater

The existing package helper now supports `check`, `download`, `install`, `resume`, `status`, and `preflight`. `--json` emits newline-delimited events with stages, safe messages and retryable error codes. `status` reads package/update metadata only; it does not read pairing identities or claim that running input/sync processes have reloaded.

The default cache is `$XDG_CACHE_HOME/wetypex-omarchy/packages`; the operation journal and per-user lock are in `$XDG_STATE_HOME/wetypex-omarchy/update`. Conventional XDG defaults apply. A cached manifest and package are reverified on every resume. New releases include `release-attestation.sigstore.json`: assets download over public HTTPS and `gh attestation verify --bundle` checks the repository, exact tag/workflow certificate identity, source and signer commit, and hosted runner. This route uses an empty GH config and strips authentication tokens. Legacy releases retain `gh release verify` / `verify-asset`; a failed bundle never falls back to legacy verification or hashes. Journals cannot authorize installation. Cancelled authentication preserves downloaded content. `resume --confirm` pins the previous operation's tag, channel and directory, even if the default channel is now stable. Downloads interrupted before a file is complete restart that file; byte-range continuation is not implemented.

A corrupt cache is rejected. The GUI offers a fresh download using `--fresh-cache`, which creates a new directory and preserves the old one. There is no automatic recovery-material pruning. Missing dependencies, private-prefix plugins, downgrades and package-manager locks stop normal installation. New schema-1 manifests include all dependencies so incompatible systems can be stopped before the package download; older manifests are checked against the verified `.PKGINFO` afterwards. Unknown manifest schemas remain rejected.

No automatic system upgrade or source-build fallback is performed. Both the normal tool and bootstrap fail rather than ignore dependencies. Package preparation never requires admin rights. Ordinary runtime preflight checks package/update prerequisites, not CMake or compilers.

## System authorization and installation

The GUI uses Polkit to invoke only `/usr/lib/wetypex-omarchy/install-package`. It is installed only by a `/usr` build. The action requires fresh administrator authorization, not retained authorization. The helper runs isolated Python, resets its environment and uses a fixed repository and command.

Before invoking pacman it independently fetches the immutable release's manifest and asset SHA-256 digests over restricted HTTPS, rejects missing/mismatched metadata, copies the caller-owned regular package into a root-private temporary directory, then verifies the protected bytes, package identity, runtime and dependencies. For new bundled releases the helper additionally verifies the manifest and protected package with the same tag-bound Sigstore policy, without user credentials. Legacy releases retain the independent HTTPS digest check at this boundary, following cryptographic verification by the ordinary-user client. Neither route substitutes HTTPS hashes for the ordinary-user cryptographic verification requirement.

Only the protected copy enters pacman. Conflict removals retain their default refusal; replacement and ignored-package default yes answers are inverted with pacman's question mask. Dependency checks and signature policy are not disabled. No arbitrary repository, executable, destination or shell string is accepted. Package transaction failure is reported without forwarding raw command/network output. Installation does not activate services or restart Fcitx.

After success, the helper records the package version, source commit, asset digest and installed core-file digests in `/var/lib/wetypex-omarchy/installed.json`. Local status labels a recorded release only while version and those core-file hashes match. Other installations remain unrecorded; existence of a development checkout is not an installation source. This does not prove every installed file or any live device connection.

The GUI shows installation separately from activation. It blocks ordinary window closing while a package operation is active. There is no automatic input/sync restart or claimed detection of every active composition/transfer; restarting remains a separate explicit desktop operation. Force-killing the application or power loss cannot be prevented by window controls.

## First installation and migration

A release draft now includes the deterministic, allowlisted `wetypex-install.pyz`, built from that exact source. It needs system Python, pacman, bsdtar and a release-verification-capable GitHub CLI. It does not need Git, a source checkout or a compiler. Verify the bootstrap before executing it. For a new bundled release, download both `wetypex-install.pyz` and `release-attestation.sigstore.json` from the same immutable release, then run (replace `vX.Y.Z` with that exact tag):

```sh
gh attestation verify wetypex-install.pyz --bundle release-attestation.sigstore.json \
  --repo eddieqiu79/wetypex-omarchy \
  --cert-identity "https://github.com/eddieqiu79/wetypex-omarchy/.github/workflows/prepare-release.yml@refs/tags/vX.Y.Z" \
  --source-ref refs/tags/vX.Y.Z --deny-self-hosted-runners
```

The verifier supplies authenticated Sigstore trust roots; do not accept a custom trust root downloaded with an untrusted installer. Then:

```sh
python3 wetypex-install.pyz --check
python3 wetypex-install.pyz --channel preview
```

The bootstrap uses the same package core and operation lock. It asks for an explicit `yes`, then invokes interactive `sudo pacman -U`, with normal package-manager dependency/conflict questions. This initial terminal transaction is necessary before the packaged desktop helper exists; it is not advertised as a zero-terminal installer. Rerunning revalidates and reuses its cache. Subsequent installed-app updates use desktop authorization.

Once the package is installed, the onboarding dialog exposes separate runtime checks, supplied official archive preparation, license-gated official download, input-method configuration, explicit sync activation and device association. Opening the dialog performs none of these mutations. Existing identity is neither copied nor removed. Service-unit selection remains an advanced manual step; custom desktop setups are not guessed. Closing the guide while a preparation/activation process is active is blocked.

### Open distribution gates

An isolated test of GitHub CLI **2.102.0**, with an empty home/config and no tokens, returned exit code **4** requesting login for legacy `gh release verify`. In contrast, bundle verification proceeded without a login prompt but could not initialize its Sigstore verifier in this network-restricted environment. The new anonymous route and workflow are implemented, with routing, identity-policy and fail-closed fixture tests; **successful verification of an actual newly generated public release bundle is still required** before claiming end-to-end no-account installation. Trust-root network availability is a prerequisite, even though no GitHub account is required by the bundle command.

Dispatch release preparation using the exact reviewed tag as the workflow ref. The workflow requires its ref and SHA to equal that tag's checked-out source, signs all finished assets, adds the public bundle, and only then creates a draft. No release assets are signed or published by the user updater. Stable acceptance still requires clean-machine and device evidence.

Release CI now requires an explicit Arch archive date. Rolling CI remains a development signal, not the release baseline. No archive date has yet been validated for the maintainer's Qt **6.11.2** environment. The actual selected snapshot, resulting package dependency set, clean installation and rollback must be tested before publishing. The workflow does not alter user mirrors or downgrade user dependencies.

## Developer workflow

```sh
python3 tools/develop.py build --build-dir /absolute/path/to/new-build
python3 tools/develop.py test --build-dir /absolute/path/to/new-build
python3 tools/develop.py preview --build-dir /absolute/path/to/new-build
```

Build directories are explicitly selected, bound to the source checkout and never reused if unowned or bound elsewhere. Build/test does not install. Preview uses temporary XDG directories, disables the account/backend launchers and session bus access, and preserves access to the display only. Real-device development requires a separately scoped single-owner transition; identities must never be duplicated. Release export continues to require clean committed source; development tools do not create branches, commit or publish.

## Acceptance remaining after local implementation

Run the offline suites and real settings-window tests, inspect staged `/usr` output, and review privilege-boundary behavior separately. Root package transactions, actual Polkit authentication, clean-host installation, supported-baseline CI, real input and Linux/Mac/iPhone file transfers are separate acceptance results. Published rc.4 is unchanged until a new release is explicitly prepared and published; no immutable asset replacement is allowed. Keep a prior compatible package for recovery; package downgrade is not a promise of state/runtime rollback.

## Local implementation validation

Source base: `294347e33f7808c7be16cf66fff6309fd9a59a49`; changes remain uncommitted in the existing checkout. The pre-existing responsive-settings edits were preserved. No branch, service, installed package or release was changed by this implementation run.

- Production-mode and development-mode CMake builds succeeded with system Python 3.14.7 and Qt 6.11.2. The developer tool used its own bound build directory and private install prefix.
- Package client (16 tests), release policy/bundling (9), privilege-boundary fixtures (3), and bootstrap artifact (1) passed. Cryptographic routing tests use synthetic subprocess/network fixtures; they do not certify a real newly signed release.
- GUI fixtures exercised check → download → authorization cancellation → resume → installed/pending activation, and confirmed onboarding starts no process on open. Both passed.
- The actual settings-window harness exercised all pages at 600/720/960/1280 widths and saved inspected offscreen screenshots. The harness now requires proof that a window was exercised instead of accepting an early successful exit. Only this test build bypasses single-instance IPC; the independent IPC test remains enabled.
- Full production-mode CTest: **11 of 13 suites passed**. `omarchy-image-stability` failed on AVIF/HEIC/HEIF with GdkPixbuf reporting a D-Bus `Operation not permitted` error. `omarchy-settings-gui` passed its new interaction cases but failed the existing single-instance socket case; a separate AF_UNIX socket probe returned errno 1 (`Operation not permitted`). These are outstanding host verification requirements, not skipped or passed tests.
- A separate developer-mode real-window test passed. Staged `/usr` output contains the package client, executable fixed-purpose installer and Polkit action with expected permissions. This was DESTDIR staging, not live installation or a pacman transaction.
- Workflow YAML, embedded shell syntax, Python syntax, PKGBUILD shell syntax and `git diff --check` passed. Release-baseline selection, real CI execution, actual administrator authorization, clean-machine bootstrap verification and device acceptance remain pending.
- The live package remained `wetypex-omarchy 1:0.1.0rc4-1`.

## User-selected installation acceptance target

The user selected this existing maintenance machine for the ordinary-user reinstall flow instead of a separate clean machine. Retain the existing account, official runtime, paired devices and input configuration. Reinstall through the published-package bootstrap/update route once the candidate passes its required checks; do not remove the existing installation first or substitute a direct source-prefix copy. This proves an existing-host reinstall/update path, not pristine-machine onboarding.

A system-only `PATH=/usr/bin:/bin` preflight reports the verifier `gh` missing, although the developer PATH contains it. The `github-cli` package must therefore be handled as an explicit system prerequisite rather than relying on the developer tool manager. No system dependency was installed during these checks.

The retained rc.4 package was rechecked against its original installation digest (`a564a90e2e728e0a872fbfa20ae6520ca7ad8110e87d60a2456df0eb110a8268`); its package identity/version match `wetypex-omarchy 1:0.1.0rc4-1`, and all declared dependencies are currently satisfied. This is a verified recovery package, not a new restoration test or an account backup.

Host execution remains necessary: this agent environment denies user-bus access and sets `no new privileges`, preventing sudo elevation. `/usr/bin` is not writable here. Complete the previously blocked host tests before integration/release, then verify the newly published candidate's anonymous proof and actual package transaction on this host. The user has approved this installation target; this is an execution-environment limitation, not a request for renewed installation permission.

## Settings-window shutdown regression

The subsequent user-run host CTest passed 12 of 13 suites, including image stability and single-instance GUI checks. The remaining `omarchy-settings-window` failure was a real SIGSEGV, not a sandbox restriction. Core metadata for PID 369012 showed `QLabel::setText` called from the update process's completion callback while `QProcess` and `UpdatePanel` were being destroyed. The process destructor emitted `finished` after sibling labels and derived-class fields had been destroyed; the receiver's automatic QObject disconnection had not happened yet.

`UpdatePanel` and the equivalent `SetupDialog` now disconnect their direct child processes from the panel at the start of their destructors. Ordinary close guards remain in place. A synthetic slow-process regression forces destruction during an active operation: before the fix the update case exited with SIGSEGV (-11); after the fix both update-status and setup-runtime-check cases passed, including confirmation that the process emitted its completion signal. This checks the failing lifetime boundary without running an installer or touching account data.

After the fix, the production settings executable and both settings test binaries rebuilt successfully. The update cancellation/resume and no-work-on-open onboarding tests also passed. `ctest --test-dir /home/eddieqiu/Projects/omarchy-wetype/build/update-experience -R '^omarchy-settings-window$' --repeat until-fail:20 --output-on-failure` passed all 20 iterations in the agent environment. `git diff --check` passed. The changed GUI suites still require a host rerun; these results do not claim a post-fix host 13/13 run, package installation or release.

After the session switched to unrestricted host execution, the agent ran `ctest --test-dir /home/eddieqiu/Projects/omarchy-wetype/build/update-experience --output-on-failure` directly: **13 of 13 suites passed**, exit code 0, in 8.22 seconds. This includes image stability, settings GUI (with the new destruction regressions and existing IPC check), and the actual settings-window harness. The host-test gate above is now satisfied. Release verification and the ordinary-user package reinstall remain separate, unfinished steps; no package or service was changed by this test run.

## rc.5 release and maintenance-host installation

The selected source, excluding the original local responsive-layout edits, was independently rebuilt and passed all 13 suites before commit `20ec1cac92e5966268eba61a55fa29ac3f902d5c`. The exact-tag Arch archive workflow passed; rc.5 was published as an immutable preview and verified. The maintenance host then completed the published bootstrap path, installing `1:0.1.0rc5-1`. All 165 package files passed integrity checks, the installed preflight reported ready, and settings/transfer/Fcitx-plugin link checks found no missing libraries. An initial mirror 404 was resolved using signed matching dependency packages from the official Arch archive, without changing mirror configuration.

This supersedes the earlier environment and installation blockers above. The bootstrap uses an interactive pacman transaction and does not create the privileged helper's origin receipt, so installed status correctly reports `unrecorded` for that receipt field even though the release assets were verified. Services remained active without being restarted; running-version activation and new real-device acceptance are not claimed. See [the rc.5 validation record](releases/v0.1.0-rc.5-validation.json).
