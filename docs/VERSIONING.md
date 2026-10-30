# Version identifiers

The project uses distinct identifiers for separate purposes:

- **Project version:** the root `VERSION` file, currently `0.1.0-rc.2`. It identifies this fork's adaptation and integration work. The `-rc.2` suffix marks the second release candidate, not a stable release.
- **Source commit:** the exact Git SHA used to build the package. This is required when reporting a development-build problem.
- **Official runtime version:** currently `2.2.3.657`, pinned independently by the upstream compatibility/setup code. It is not this fork's version and does not indicate the installed Mac/iPhone client version.
- **Arch package version:** development exports use `0.1.0dev.r<commit-count>.g<short-SHA>`, plus `pkgrel`. Arch `epoch=1` ensures these packages supersede the earlier previews numbered after the engine. The epoch must be retained in subsequent package metadata. makepkg may include the epoch and a colon in the package filename. CI replaces the colon with an underscore only in the uploaded artifact filename because GitHub artifacts reject colons; the package metadata and pacman version remain unchanged.

`wetypex-omarchy-setup --version` prints the installed project version. `pacman -Q wetypex-omarchy` prints the package version. Arch packages also install `/usr/share/fcitx5-wetypex/build-info.json`, which records the source commit, project/package versions, source checksum and supported runtime version. The settings version label identifies the fork and engine separately.

Release candidates use `MAJOR.MINOR.PATCH-rc.N` and matching `vMAJOR.MINOR.PATCH-rc.N` tags; Arch maps this to `MAJOR.MINOR.PATCHrcN`. Local `vercmp` checks confirm development < rc < stable. The release channel refuses `-dev` tags, mismatched versions and reused release assets. See [updates](UPDATES.md).

A stable release requires removing the development suffix in VERSION, validating the package and target environments, and publishing a matching reviewed tag and release. A tag or version label is not test evidence. No stable tag or release was created merely by introducing this version scheme. Build/release tools must not silently obtain or upgrade the proprietary runtime when the project version changes.

The old engine-numbered CI artifact recorded in ARCH.md remains a historical validation result; it is not renamed or represented as a test of a newer commit.
