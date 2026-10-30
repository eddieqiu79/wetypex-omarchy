# Licenses and source provenance

**English** · [简体中文](LICENSES.zh-CN.md)

WeTypeX Omarchy uses **component-specific licenses**. WeTypeX adapter code and its patches retain MIT; VType-derived modules retain GPLv3; the added Omarchy integration uses GPL-3.0-only. The root [LICENSE](LICENSE) is an index. It does not relicense the repository as a whole.

This guide records licenses, sources and distribution boundaries. It is not legal advice or a grant of rights from Tencent. The original license texts govern the code they cover; neither language edition of this guide replaces those texts.

## 1. Component map

| Component | License and source | Notices and scope |
| --- | --- | --- |
| WeTypeX adapter code and corresponding patches | MIT · [panxuc/fcitx5-wetypex](https://github.com/panxuc/fcitx5-wetypex) | Original [MIT text and copyright](LICENSES/WeTypeX-MIT.txt) and [NOTICE](NOTICE) retained. Includes native desktop patches in `src/settings/`, `src/transfer/` and `src/common/`, and the existing updater template. |
| `compat/cloud/剪贴板协议.py` and `compat/cloud/官方同步探针.py` | GPL version 3 · derived from [zhl-HFUT/vtype-linux](https://github.com/zhl-HFUT/vtype-linux) | See the [GPLv3 text](compat/LICENSE) and [provenance notice](compat/NOTICE). Source and local modifications are included. |
| `compat/cloud/图片格式.py` | GPL-3.0-only · added in this project | This file was not part of the recorded VType baseline. See [compat/NOTICE](compat/NOTICE). |
| `omarchy/`, `tests/`, `fixtures/`, `cmake/OmarchyCompatibility.cmake` | GPL-3.0-only · added in this project | Includes the package update client, integration tools and synthetic tests. License text: [compat/LICENSE](compat/LICENSE). |
| `demo/desktop/` | GPL-3.0-only · added in this project | Original Qt Widgets prototype and synthetic scenarios; no extracted official UI assets. |
| Fork-specific release tools and workflows | GPL-3.0-only · added in this project | Includes `tools/prepare-release.py`, `tools/check-upstream.py` and the fork's release workflows. This does not change the MIT license of the updater adapter template. |
| Newly added project documentation | GPL-3.0-only | Upstream documents retain their original notices and applicable licenses. |
| wx-ime-sdk | Apache-2.0 · [mkdir700/wx-ime-sdk](https://github.com/mkdir700/wx-ime-sdk) | Referenced in the VType protocol module's provenance. No SDK source or binary is bundled. |
| libkqueue, libcurl, OpenSSL, LibIME, Qt and other dependencies | Respective upstream licenses | Supplied through the build process or the system. Their distribution requirements apply separately. |
| Official WeType software and resources | Tencent and other relevant rights holders | Excluded from this distribution and from this project's open-source license grants. |

## 2. Recorded sources

| Project | Baseline commit |
| --- | --- |
| WeTypeX | [`291193a26938e0e8858d72b737066f37eb7a5f94`](https://github.com/panxuc/fcitx5-wetypex/tree/291193a26938e0e8858d72b737066f37eb7a5f94) |
| VType Linux | [`46b496ad9a3d23633400021ae92600d208d44bd0`](https://github.com/zhl-HFUT/vtype-linux/tree/46b496ad9a3d23633400021ae92600d208d44bd0) |
| wx-ime-sdk, historical research | [`1ec70393cc9524c6c341cf768f090b0176586f72`](https://github.com/mkdir700/wx-ime-sdk/tree/1ec70393cc9524c6c341cf768f090b0176586f72) |

The current WeTypeX file-transfer path does not use a separate, continuously running `wxc` service. A research reference does not mean that the referenced SDK is included in the package.

## 3. Redistribution and modifications

MIT and GPL permit distribution without charge, subject to their conditions. Publishing this project for free does not add a noncommercial restriction to either license.

Retain the applicable copyright, license and modification notices. Distribution involving GPL modules must meet the applicable corresponding-source requirements. MIT code can be compatible with GPL code, but the obligations for a combined distribution depend on how the components are combined and on their license terms. A process boundary alone does not establish an exemption.

The repository preserves upstream attribution. The Arch release bundle provides adapter code, the corresponding source archive, build recipe, provenance manifest and checksums. Review the dependencies and resources included in each binary distribution and meet their license and source-distribution requirements.

## 4. Official software and services

WeTypeX Omarchy is an independent, unofficial project. It has no Tencent affiliation or endorsement and claims no Tencent authorization. Product names and trademarks identify the software and services targeted for compatibility.

The repository and adapter packages exclude official installers, extracted executables, dictionaries, models and UI resources. They also exclude account identities, device private keys, tokens, signed download URLs, personal images, transferred files, runtime logs and existing user configuration. The official runtime must be obtained separately under its applicable terms. Upstream automated release workflows are not enabled here; this fork publishes its own adapter-only packages.

The adaptation code's open-source licenses do not grant rights to use, modify, reverse engineer or redistribute Tencent's proprietary software, or to access official services. Free distribution and a disclaimer do not establish such permission. The applicable agreement, law and facts determine whether a particular use is permitted. Compatibility and service availability may change. Warranty terms remain those of the applicable component licenses; this notice does not override those licenses or applicable law.

### Recorded agreement review

The [WeType Software License and Service Agreement](https://wetype.weixin.qq.com/web/agreement), reviewed on **2026-10-29**, displayed **2026-05-11** as both its update and effective date. The retrieved HTML had this SHA-256:

```text
f6cc0d2c3c0f1df6d3379d1a227c6361f46717de6c8ee82d5160568b1e39a0f8
```

This is a dated record, not a claim that the agreement remains unchanged. Consult the version applicable to your use. The recorded summary was:

| Sections | Subject |
| --- | --- |
| 3.2.1–3.2.3 | A personal, non-transferable, non-exclusive license; noncommercial use on a single terminal; a limited backup copy; reservation of rights not expressly granted. |
| 8.1–8.2 | Intellectual property in the official software and resources; restrictions on certain uses without written permission, whether commercial or noncommercial; restrictions concerning reverse engineering. |
| 11 | Third-party software and its applicable licenses. |

This summary is not a substitute for the agreement itself. The project distributes adaptation code with recorded provenance, not an official Tencent binary package or a Tencent authorization.

### Upstream wording

The [archived upstream README](docs/UPSTREAM-README.md) includes an MIT license indication alongside personal-study and 24-hour deletion language. This project has not obtained clarification of how that wording relates to the MIT grant. The original text is preserved; it is not treated as a legal exemption or authorization.

## 5. Original notices

- [Root license index](LICENSE)
- [WeTypeX MIT license and copyright](LICENSES/WeTypeX-MIT.txt)
- [Upstream NOTICE](NOTICE)
- [GPL version 3](compat/LICENSE)
- [VType-derived module provenance](compat/NOTICE)

The original notices and license texts are retained unchanged by this bilingual documentation update. The Chinese edition explains the same component mapping; it does not amend or replace a license.
