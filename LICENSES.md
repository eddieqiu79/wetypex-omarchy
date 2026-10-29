# Licenses and Source Provenance

This repository contains components under different licenses. It is **not exclusively MIT**, and it does not claim to relicense all upstream files under the GPL. The root [LICENSE](LICENSE) is an index, not a single license covering every file.

This document records source provenance and license boundaries. It is not a grant of rights from Tencent or legal advice. The licenses for this project's code, the license for the official WeType software, and the terms governing official services are separate matters.

## Component licenses

| Component | Source and license | Distribution in this repository |
| --- | --- | --- |
| WeTypeX's own adapter code and corresponding patches | [panxuc/fcitx5-wetypex](https://github.com/panxuc/fcitx5-wetypex), MIT | The original [MIT license and copyright notice](LICENSES/WeTypeX-MIT.txt) and upstream [NOTICE](NOTICE) are retained. Corresponding patches are distributed under MIT. |
| `compat/cloud/剪贴板协议.py` and `compat/cloud/官方同步探针.py` | Derived from [zhl-HFUT/vtype-linux](https://github.com/zhl-HFUT/vtype-linux), GPL version 3 | The source, [GPLv3 text](compat/LICENSE), [provenance notice](compat/NOTICE), upstream baseline, and local changes are retained. |
| `compat/cloud/图片格式.py` | Locally added image-format compatibility module, GPL-3.0-only | This file was not present in the recorded VType baseline. See [compat/NOTICE](compat/NOTICE). |
| `omarchy/`, `tests/`, `fixtures/`, and `cmake/OmarchyCompatibility.cmake` | This project's integration and tests, GPL-3.0-only | Source and synthetic fixtures are provided; the GPLv3 text is in [compat/LICENSE](compat/LICENSE). |
| Newly added project documentation | This project, GPL-3.0-only | Upstream documentation retains its original notices and applicable license. |
| wx-ime-sdk | [mkdir700/wx-ime-sdk](https://github.com/mkdir700/wx-ime-sdk), Apache-2.0 | Referenced in the VType protocol module's provenance. No SDK source or binary is bundled here. |
| libkqueue, libcurl, OpenSSL, LibIME, Qt, and other dependencies | Their respective upstream licenses | Supplied through the build process or the system. Their applicable distribution requirements remain separate from this repository's component licenses. |
| Official WeType software and resources | Tencent and relevant rights holders | Not included and not licensed as this project's open-source code. |

Recorded baselines:

- WeTypeX: [`291193a26938e0e8858d72b737066f37eb7a5f94`](https://github.com/panxuc/fcitx5-wetypex/tree/291193a26938e0e8858d72b737066f37eb7a5f94).
- VType Linux: [`46b496ad9a3d23633400021ae92600d208d44bd0`](https://github.com/zhl-HFUT/vtype-linux/tree/46b496ad9a3d23633400021ae92600d208d44bd0).
- Historical wx-ime-sdk research: [`1ec70393cc9524c6c341cf768f090b0176586f72`](https://github.com/mkdir700/wx-ime-sdk/tree/1ec70393cc9524c6c341cf768f090b0176586f72). The current WeTypeX file-transfer path does not use a separate, continuously running `wxc` service.

The MIT and GPL licenses both permit free distribution. Adding a restriction that prohibits commercial use would not be the same as distributing under those unmodified licenses. MIT-licensed code can be compatible with GPL-licensed code, but the obligations for a particular combined distribution depend on how its components are combined and on their applicable terms. Distribution involving the GPL modules must preserve the applicable license notices and modification notices and satisfy the corresponding-source requirements where applicable. A process boundary alone does not establish an exemption from third-party license obligations.

## Official WeType agreement

The [WeType Software License and Service Agreement](https://wetype.weixin.qq.com/web/agreement) reviewed on 2026-10-29 displayed an update date and an effective date of 2026-05-11. The SHA-256 of the HTML retrieved on that date was:

`f6cc0d2c3c0f1df6d3379d1a227c6361f46717de6c8ee82d5160568b1e39a0f8`

The agreement may change; consult the version applicable to your use. The following is a summary of the reviewed version, not a substitute for its text:

- Sections 3.2.1–3.2.3 describe a personal, non-transferable, non-exclusive license, noncommercial use on a single terminal, a limited backup copy, and the reservation of rights not expressly granted.
- Sections 8.1–8.2 address intellectual property rights in the official software and resources, restrictions on certain uses without written permission for commercial or noncommercial purposes, and restrictions concerning reverse engineering.
- Section 11 addresses third-party software and applicable third-party licenses.

Free and open-source distribution does not establish permission to redistribute the official engine, perform every compatibility operation, or use official services. This repository provides adaptation source code with recorded provenance; it does not provide an official binary package or claim Tencent's authorization. Whether a particular implementation or use is permitted depends on the applicable law, agreement, and facts.

The [archived upstream README](docs/UPSTREAM-README.md) contains both an MIT license indication and language about personal study and deletion within 24 hours. This project has not obtained an additional clarification about the relationship between that language and the MIT grant. The original text is retained for readers to assess; it is not treated as an exemption or authorization.

## Distribution boundary

The repository does not include official installation packages, extracted executables, dictionaries, models or UI resources, account identities, device private keys, tokens, signed download URLs, personal images, transferred files, runtime logs, or existing user configuration.

Upstream automated release workflows are not enabled here. The fork's Arch CI can publish adapter-only package artifacts together with the corresponding source archive, build recipe, provenance manifest and checksums. These artifacts do not contain the official runtime. Before publishing a stable binary release, review the included dependencies and resources and satisfy their respective license and source-distribution requirements.
