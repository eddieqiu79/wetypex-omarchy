# Agent instructions

This repository is an unofficial WeTypeX compatibility project. Read README.md, docs/INSTALL.md, and LICENSES.md before installing, packaging, or changing runtime behavior.

- Inspect the target host, existing Fcitx installation, clipboard managers, and repository status before making changes. Preserve unrelated work and user data.
- Start with `python3 tools/preflight.py` and isolated build/testing. Do not treat it as proof of installation or interoperability.
- Ask for the user's explicit authorization before installing system packages, changing an input method or desktop configuration, restarting Fcitx, enabling services, changing a firewall, obtaining official runtime assets, or pairing devices. Follow existing user authorization within its stated scope.
- Do not read or publish account identity, private keys, tokens, pairing codes, clipboard payloads, signed URLs, or personal files. Use synthetic test data. Never duplicate an identity directory to create parallel clients.
- Do not run upstream packaging/aur/PKGBUILD as this fork's installer: it points to upstream releases. Use the fork-specific recipe and explicit activation lifecycle in docs/ARCH.md; clean desktop/device validation remains separate.
- Preserve the official runtime/license boundary and source attribution. Do not distribute proprietary runtime, dictionaries, models, or extracted UI assets.
- Report actual commit SHA, commands, scope, and results. Separate offline checks, local desktop behavior, and each official-device direction. A successful build or active service is not an end-to-end acceptance test.
- Direct cross-device file copy/paste is not implemented. A file-transfer UI, copied path, or Mac/iPhone continuity does not prove otherwise.
