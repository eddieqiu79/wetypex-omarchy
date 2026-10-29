# Arch / Omarchy package installation

This fork has its own package recipe and source exporter in `packaging/arch/` and `tools/build-arch.py`. It does not fetch upstream WeTypeX release binaries. The old `packaging/aur/` recipe remains an upstream reference and is not this fork's installation entry point.

## Scope and status

Target: Omarchy/Arch Linux x86_64. Build uses system libcurl and Qt6, and disables the optional layer-shell toolbar. A pinned libkqueue source is fetched by CMake. Arch package versions remain rolling; this is not a completely frozen dependency lockfile. CI records the installed package list.

The adapter package excludes official runtime, dictionary, model and extracted UI assets. It does not pair devices, modify a default input method, activate services, or restart Fcitx during package installation. Packaging checks are separate from a clean Hyprland session and official-device acceptance tests.

## 1. Inspect and build the exact checkout

```sh
git clone https://github.com/eddieqiu79/wetypex-omarchy.git
cd wetypex-omarchy
# Prefer a reviewed commit; record the actual checkout.
git rev-parse HEAD
python3 tools/preflight.py
python3 tools/build-arch.py --output "$PWD/build/arch-package"
cd build/arch-package
less PKGBUILD
makepkg --syncdeps --cleanbuild --clean
```

Run makepkg as an ordinary user. `--syncdeps` may request elevated privileges to install dependencies: obtain the user's authorization first. The exporter rejects dirty checkouts and existing output directories, archives only tracked committed files, and writes a concrete source checksum and commit manifest. Use a new output directory for a retry. `makepkg` runs all three CTest suites; do not bypass checks or checksum failures. Package building does not install the resulting package.

Do not mix arbitrary Arch mirrors with an Omarchy mirror or perform an unsupported partial upgrade to satisfy missing dependencies. If dependency versions are unavailable, report the exact mismatch and use the target system's supported update route with user approval.

## 2. Review and install

```sh
pacman -Qip ./wetypex-omarchy-*.pkg.tar.zst
pacman -Qlp ./wetypex-omarchy-*.pkg.tar.zst
sudo pacman -U ./wetypex-omarchy-*.pkg.tar.zst
```

Use the exact single package filename after inspection. The recipe declares conflicts with packaged upstream WeTypeX variants because they own the same plugin and executables. Do not automatically accept removing a conflicting input method without user authorization. Unmanaged files and user-prefix installations are not handled by pacman conflicts: inspect `wetypex-omarchy-setup status`, existing Fcitx addon paths and prior services. The maintenance machine's existing private-prefix deployment must be migrated separately, not silently overwritten.

## 3. Prepare the official runtime and input method

Read LICENSES.md and the official agreement first. After explicit user consent, select one source:

```sh
fcitx5-wetypex-setup --archive /path/to/the-supported-official-package.zip
# Or, only after the user agrees to the official terms and download:
fcitx5-wetypex-setup --download --accept-upstream-license
fcitx5-wetypex-setup --check
```

These commands run as the desktop user, not root. A successful runtime check is not an input or synchronization test. Use the existing Fcitx configuration interface to add WeTypeX while retaining the previous input method. Do not overwrite the whole Fcitx profile. Pair through the official account/settings interface; never share pairing codes with the agent or copy identity directories. Existing state is not reset automatically.

## 4. Opt in to independent clipboard synchronization

For a desktop whose active Fcitx user service is `omarchy-fcitx5.service`:

```sh
wetypex-omarchy-setup status
wetypex-omarchy-setup activate --fcitx-service omarchy-fcitx5.service --confirm
```

The explicit activation creates one Fcitx drop-in to suppress its own sync launcher, restarts that Fcitx service and enables/starts the packaged `wetypex-sync.service`. It refuses an existing user sync unit, existing activation drop-in, or externally enabled/running sync service. It does not change clipboard preferences, obtain runtime, pair devices, disable CrossPaste, or modify firewalls. Review other clipboard tools with the user and authorize any changes separately. If your desktop starts Fcitx differently, do not invent a service name: use a reviewed integration for that startup method.

`wetypex-files` opens the file-transfer window. Ordinary file transfer and file clipboard copy/paste are different: the latter is not implemented.

## 5. Updates, deactivation, removal and recovery

Updates use a newly built, inspected package via `pacman -U`. Retain the previous package for rollback; restart the affected user services only when authorized and after checking for active transfers/input. Runtime updates and account migration are separate operations and are not automatic.

Before removing the package, undo this tool's activation as the desktop user:

```sh
wetypex-omarchy-setup deactivate --confirm
```

This stops/disables sync, removes only its unchanged drop-in, and restarts the recorded Fcitx service. It preserves accounts, runtime and dictionaries. It refuses to overwrite a modified drop-in; investigate rather than deleting user changes. Switch to a retained fallback input method, close file transfers, then remove the adapter package with `sudo pacman -R wetypex-omarchy` after authorization. Do not delete user data to uninstall the adapter.

For package rollback, reinstall a retained compatible older package with `pacman -U`. This is not a system rollback: dependency ABI changes or runtime/state schema changes can prevent an older version from working. A failed activation attempts to restore the previous Fcitx sync-launch behavior; if restoration itself fails, the activation record is kept for inspection. No claim of automatic recovery from every system failure is made.

## Acceptance

Verify real input in Wayland and XWayland applications, switching back to the old input method, sync independence, and every relevant text/image/file direction with synthetic content. Test login, sleep/wake, disconnect/reconnect, cancellation, package update and removal on a clean target system. A CI-built package is not evidence that these desktop and device tests passed.
