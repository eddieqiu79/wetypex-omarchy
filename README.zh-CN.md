# WeTypeX Omarchy

[English](README.md) · **简体中文**

基于 [WeTypeX](https://github.com/panxuc/fcitx5-wetypex)，为 **Omarchy / Hyprland** 提供中文输入、剪贴板同步和文件传输。

Linux 使用 Fcitx5，Mac 和 iPhone 继续使用官方微信输入法，Apple 两端无需额外安装应用或常驻服务。

**当前版本：**[0.1.0-rc.3](https://github.com/eddieqiu79/wetypex-omarchy/releases/tag/v0.1.0-rc.3)，候选版。**目标平台：**Arch / Omarchy，x86_64。官方运行时单独固定为 `2.2.3.657`。

[安装与更新](docs/RELEASE-INSTALL.md) · [桌面使用指南](docs/DESKTOP.md) · [参与贡献](CONTRIBUTING.zh-CN.md) · [许可说明](LICENSES.zh-CN.md)

## 安装与更新

从[发布包安装指南](docs/RELEASE-INSTALL.md)开始，按顺序完成环境检查、下载校验、安装、启用与恢复准备。如果交给 AI Agent 安装，请提供本仓库地址，并要求它遵循 [AGENTS.md](AGENTS.md) 和[安装检查清单](docs/INSTALL.md)。

- **预构建 Arch 包：**使用本仓库的不可变 GitHub Releases，并明确选择 `preview` 通道。稳定通道目前没有发布版本。
- **自行构建 Arch 包：**按照[源码打包指南](docs/ARCH.md)操作。仓库中保留的上游 AUR 配方安装的是上游 WeTypeX，不包含本分支的改动。
- **获取更新：**更新工具验证发布及附件证明、文件哈希和包元数据，再由用户明确执行 pacman 安装。目前未配置 pacman 软件源，单独运行 `pacman -Syu` 不会发现本项目的新版本。

安装包包含适配器和桌面应用。官方运行时需按其适用条款另行获取；关联设备、启用服务和重启 Fcitx 是独立的设置步骤。已有私有目录安装的用户，应先检查迁移方案，再安装系统包。

## 功能

### 中文输入与桌面设置

保留 WeTypeX 的 Fcitx5 接入和 macOS 引擎兼容层，支持 `z/zh`、`c/ch`、`s/sh`、`in/ing` 等模糊音设置。设置和文件传输窗口采用深色、可调整大小的原生桌面界面。

剪贴板同步和文件传输与输入路径分开运行，切换到其他输入法后，同步服务仍可继续工作。中文输入已在 Hyprland 0.56.2 上人工验证；候选延迟及传输负载下的输入响应尚未系统测量。

![使用隔离测试配置的原生设置窗口](docs/images/native-settings.png)

### 文字与图片剪贴板

Linux 同步服务与其他功能共用一个关联身份，保留本地新复制内容的优先级，避免收到的内容再次上传，并在启动时建立基线，防止自动发送启动前的旧剪贴板。图片处理包含完整管道读取、下载完整性检查和失败重试。

PNG、JPEG/JPG 已通过真实设备测试。转换层也处理 HEIF/HEIC、AVIF、WebP、GIF、TIFF 和 BMP，但依赖系统已安装的解码器，部分格式只有离线样本测试。支持的图片会转换为静态 PNG；转换不保证保留动画、多页内容、元数据或原始文件字节。

iPhone 可能需要展开微信输入法键盘并打开剪贴板入口，才能接收或查看内容。在该入口可见，不代表已写入 iOS 系统剪贴板。同时使用其他跨设备剪贴板工具时，应协调运行方式，避免相互覆盖。

### 文件传输

在传输窗口选择文件和已关联设备，按提示完成邀请与连接。窗口显示任务进度、结果和接收目录；接收授权以当前连接会话为范围。

**暂不支持跨设备直接复制粘贴文件。** 在文件管理器中复制 `.md`、`.pdf` 等文件，不会将其发送到另一台设备。请使用文件传输窗口；复制路径或 URI 不会触发上传。测试中观察到的 Mac–iPhone 文件复制粘贴使用的是 Apple 原生接力功能。

暂不支持发送目录或由后端逐个取消文件。文件传输已在局域网内测试，其他网络环境下的连通性尚未确认。

![包含合成测试任务的原生文件传输窗口](docs/images/native-transfer.png)

## 验证情况

维护机器使用 Omarchy / Hyprland；官方客户端最后报告的版本为 **Mac 3.0.0 (670)** 和 **iPhone 3.5.5**。

| 方向 | 文字剪贴板 | 图片剪贴板 | 独立文件传输 |
| --- | --- | --- | --- |
| Linux → Mac | 人工确认收到 | 人工确认收到 | 用户确认原生 GUI 通过 |
| Mac → Linux | 已在 Linux 粘贴 | 已在 Linux 粘贴 | 用户确认原生 GUI 通过 |
| Linux → iPhone | 微信输入法内收到 | 微信输入法内收到 | 用户确认原生 GUI 通过 |
| iPhone → Linux | 已在 Linux 粘贴 | 已在 Linux 粘贴，含 JPEG、PNG | 用户确认原生 GUI 通过 |

剪贴板结果来自此前的实机测试。最近一轮验证的是原生文件传输 GUI：Mac 在手机专项修复前通过，iPhone 在部署 `95315f80a9431822461c5e2f476ade1950df65de` 的修复后通过。该修复已包含在 `rc.2` 中。本轮没有重测全部剪贴板方向，也没有比较两端文件哈希。

`rc.2` 安装包对应提交 `ae25324888bea252d24f6f1c4a925c32cfd46876`，通过了[干净 Arch 环境构建、10 组测试及包安装、升级、回退检查](https://github.com/eddieqiu79/wetypex-omarchy/actions/runs/40999204724)。公开下载通过了发布及附件证明和完整性验证，详见[发布说明](docs/releases/v0.1.0-rc.2.md)与[验收记录](docs/releases/v0.1.0-rc.2-validation.json)。

桌面安装迁移、重启、休眠唤醒、断网恢复和长时间运行仍待验收。拒绝、取消、超时和大文件性能也需要更广泛的设备测试。目前没有固定延迟、资源占用或“零开销”承诺。

## 从源码构建

先阅读[贡献指南](CONTRIBUTING.zh-CN.md)，运行 `python3 tools/preflight.py`。构建依赖包括 Fcitx5、LibIME 开发文件、带 WebEngine 的 Qt6、Clang/libc++、CMake、Python 和 cryptography。图片转换还需要对应的 GdkPixbuf/Qt 插件，完整包依赖见 [Arch 配方](packaging/arch/PKGBUILD.in)。

以下命令与 Arch CI 一样使用 Qt6、系统 libcurl，并关闭可选的 layer-shell 工具栏：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/lib/wetypex-omarchy" \
  -DCMAKE_INSTALL_LIBDIR=lib -DFCITX_INSTALL_USE_FCITX_SYS_PATHS=OFF \
  -DWETYPE_QT_VERSION=6 -DWETYPE_LAYER_SHELL=OFF -DWETYPE_BUNDLED_CURL=OFF
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

这些命令只构建和测试，不安装依赖或启用输入法。离线测试不需要账号或专有运行时；实际输入和设备互通测试需要另行准备运行时并关联设备。

## 文档导航

| 主题 | 文档 |
| --- | --- |
| 安装与恢复 | [发布包](docs/RELEASE-INSTALL.md) · [Arch 源码包](docs/ARCH.md) |
| 桌面使用 | [设置与文件传输](docs/DESKTOP.md) · [可调设置清单](docs/SETTINGS-CONTENT.md) |
| 维护 | [更新与上游审查](docs/UPDATES.md) · [版本规则](docs/VERSIONING.md) · [更新日志](CHANGELOG.md) |
| 开发 | [贡献指南](CONTRIBUTING.zh-CN.md) · [安全问题报告](SECURITY.md) |
| 工程记录 | [改动说明](docs/OMARCHY-CHANGES.md) · [运行操作](docs/OMARCHY-OPERATIONS.md) · [早期验证](docs/OMARCHY-VALIDATION.md) |
| 来源 | [上游源码清单](docs/upstream-source-manifest.json) · [归档的上游 README](docs/UPSTREAM-README.md) · [许可说明](LICENSES.zh-CN.md) |

README、贡献指南和许可说明提供中英文版本，其他指南目前分别使用英文或中文。历史记录只适用于各自注明的提交。

## 来源与许可

WeTypeX 基线为 [`291193a26938e0e8858d72b737066f37eb7a5f94`](https://github.com/panxuc/fcitx5-wetypex/tree/291193a26938e0e8858d72b737066f37eb7a5f94)，部分剪贴板兼容工作源于 [VType Linux](https://github.com/zhl-HFUT/vtype-linux)。仓库保留上游署名与声明。

许可按组件划分：WeTypeX 适配器及对应补丁采用 MIT；VType 衍生模块保留 GPLv3；新增 Omarchy 集成和测试采用 GPL-3.0-only。完整对应关系和许可证原文见 [LICENSES.zh-CN.md](LICENSES.zh-CN.md)。

WeTypeX Omarchy 是独立的非官方项目，与腾讯没有隶属、背书关系，也不宣称获得腾讯授权。产品名称和商标仅用于标识兼容对象。发行内容不包含官方可执行文件、词库、模型、提取的界面资源或私人用户数据。

开源许可证适用于适配代码，不覆盖腾讯的专有软件或服务。获取或使用运行时前，请阅读适用的[官方协议](https://wetype.weixin.qq.com/web/agreement)。免费发布不赋予这些专有软件的额外使用权。兼容性和服务可用性可能变化；担保条款以各组件许可证为准。本说明不替代相关许可证或适用法律。
