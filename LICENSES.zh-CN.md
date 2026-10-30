# 许可与源码来源

[English](LICENSES.md) · **简体中文**

WeTypeX Omarchy **按组件适用不同许可**：WeTypeX 适配器及其补丁保留 MIT，VType 衍生模块保留 GPLv3，新增 Omarchy 集成采用 GPL-3.0-only。根目录的 [LICENSE](LICENSE) 是索引，不对整个仓库重新授权。

本指南记录许可、来源和分发范围，不构成法律意见，也不是腾讯的授权。各许可证原文适用于其覆盖的代码；本指南的中英文版本均不替代许可证原文。

## 1. 组件对应关系

| 组件 | 许可与来源 | 声明及范围 |
| --- | --- | --- |
| WeTypeX 适配器及对应补丁 | MIT · [panxuc/fcitx5-wetypex](https://github.com/panxuc/fcitx5-wetypex) | 保留原始 [MIT 许可及版权声明](LICENSES/WeTypeX-MIT.txt)和 [NOTICE](NOTICE)。包括 `src/settings/`、`src/transfer/`、`src/common/` 的原生桌面补丁，以及原有更新器模板。 |
| `compat/cloud/剪贴板协议.py`、`compat/cloud/官方同步探针.py` | GPL 第 3 版 · 源于 [zhl-HFUT/vtype-linux](https://github.com/zhl-HFUT/vtype-linux) | 见 [GPLv3 原文](compat/LICENSE)和[来源声明](compat/NOTICE)，仓库包含源码与本地修改。 |
| `compat/cloud/图片格式.py` | GPL-3.0-only · 本项目新增 | 该文件不在已记录的 VType 基线中，见 [compat/NOTICE](compat/NOTICE)。 |
| `omarchy/`、`tests/`、`fixtures/`、`cmake/OmarchyCompatibility.cmake` | GPL-3.0-only · 本项目新增 | 包括安装包更新客户端、集成工具和合成测试，许可原文见 [compat/LICENSE](compat/LICENSE)。 |
| `demo/desktop/` | GPL-3.0-only · 本项目新增 | 原创 Qt Widgets 原型和合成场景，不含提取的官方 UI 素材。 |
| 本分支的发布工具和工作流 | GPL-3.0-only · 本项目新增 | 包括 `tools/prepare-release.py`、`tools/check-upstream.py` 及本分支的发布工作流，不改变更新器适配模板的 MIT 许可。 |
| 新增项目文档 | GPL-3.0-only | 上游文档保留原有声明及适用许可。 |
| wx-ime-sdk | Apache-2.0 · [mkdir700/wx-ime-sdk](https://github.com/mkdir700/wx-ime-sdk) | VType 协议模块的来源说明引用了该项目，仓库不捆绑其源码或二进制。 |
| libkqueue、libcurl、OpenSSL、LibIME、Qt 等依赖 | 各自的上游许可 | 由构建过程或系统提供，分别遵守其分发要求。 |
| 官方微信输入法软件与资源 | 腾讯及相关权利人 | 不包含在本项目发行内容中，也不在本项目开源授权范围内。 |

## 2. 已记录的源码基线

| 项目 | 基线提交 |
| --- | --- |
| WeTypeX | [`291193a26938e0e8858d72b737066f37eb7a5f94`](https://github.com/panxuc/fcitx5-wetypex/tree/291193a26938e0e8858d72b737066f37eb7a5f94) |
| VType Linux | [`46b496ad9a3d23633400021ae92600d208d44bd0`](https://github.com/zhl-HFUT/vtype-linux/tree/46b496ad9a3d23633400021ae92600d208d44bd0) |
| wx-ime-sdk，早期研究 | [`1ec70393cc9524c6c341cf768f090b0176586f72`](https://github.com/mkdir700/wx-ime-sdk/tree/1ec70393cc9524c6c341cf768f090b0176586f72) |

当前 WeTypeX 文件传输路径不使用独立常驻的 `wxc` 服务。引用研究来源，不代表安装包包含该 SDK。

## 3. 再分发与修改

MIT 和 GPL 均允许在遵守各自条件的前提下免费分发。本项目免费发布，不会给这些许可证额外增加“禁止商用”的限制。

应保留适用的版权、许可和修改声明。涉及 GPL 模块的分发须满足适用的对应源码要求。MIT 代码可以与 GPL 代码兼容，但组合分发的义务取决于组合方式及具体许可条款；仅有进程边界，不足以认定豁免相关义务。

仓库保留上游署名。Arch 发布包组提供适配代码、对应源码归档、构建配方、来源清单和校验和。每次分发二进制时，都应审查包含的依赖和资源，并满足其许可及源码提供要求。

## 4. 官方软件与服务

WeTypeX Omarchy 是独立的非官方项目，与腾讯没有隶属或背书关系，也不宣称获得腾讯授权。产品名称和商标用于标识兼容的软件及服务。

仓库与适配器安装包不包含官方安装程序、提取的可执行文件、词库、模型或界面资源，也不包含账号身份、设备私钥、令牌、带签名的下载 URL、个人图片、传输文件、运行日志或已有用户配置。官方运行时需按其适用条款另行获取。本仓库未启用上游自动发布工作流，而是通过本分支流程发布仅含适配器的安装包。

适配代码的开源许可证，不赋予使用、修改、逆向工程、再分发腾讯专有软件或访问官方服务的权限。免费分发和免责声明本身不能证明已获得这些许可。具体用途是否被允许，取决于适用协议、法律和事实。兼容性与服务可用性可能变化；担保条款仍以各组件许可证为准，本说明不替代相关许可证或适用法律。

### 已记录的协议审阅

**2026-10-29** 审阅的[《微信输入法软件许可及服务协议》](https://wetype.weixin.qq.com/web/agreement)，页面标注的更新日期和生效日期均为 **2026-05-11**。当日获取的 HTML 的 SHA-256 为：

```text
f6cc0d2c3c0f1df6d3379d1a227c6361f46717de6c8ee82d5160568b1e39a0f8
```

这是带日期的记录，不代表协议此后没有变化。请查看适用于实际使用情形的版本。原审阅摘要如下：

| 条款 | 主题 |
| --- | --- |
| 3.2.1–3.2.3 | 个人、不可转让、非独占的许可；单一终端上的非商业使用；有限备份；保留未明确授予的权利。 |
| 8.1–8.2 | 官方软件及资源的知识产权；无论商业或非商业用途，部分行为须经书面许可；有关逆向工程的限制。 |
| 11 | 第三方软件及其适用许可。 |

摘要不替代协议原文。本项目提供具有来源记录的适配代码，不提供腾讯官方二进制安装包，也不代表腾讯授权。

### 上游措辞

[归档的上游 README](docs/UPSTREAM-README.md) 在标注 MIT 许可的同时，包含个人学习和 24 小时内删除的措辞。本项目尚未获得对这些措辞与 MIT 授权之间关系的进一步澄清。原文予以保留，不将其视为法律豁免或授权。

## 5. 原始声明

- [根目录许可索引](LICENSE)
- [WeTypeX MIT 许可及版权](LICENSES/WeTypeX-MIT.txt)
- [上游 NOTICE](NOTICE)
- [GPL 第 3 版](compat/LICENSE)
- [VType 衍生模块来源](compat/NOTICE)

本次双语文档更新未修改以上原始声明或许可证正文。中文版解释同一组件对应关系，不修改或替代任何许可证。
