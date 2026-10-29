# WeTypeX Omarchy

面向 Omarchy / Hyprland 的 WeTypeX 兼容修改版本。保留上游 Fcitx5 输入法与 macOS 引擎接入，补充文字、图片剪贴板的稳定性修复和文件传输入口。

基于 [panxuc/fcitx5-wetypex](https://github.com/panxuc/fcitx5-wetypex) 的 `291193a26938e0e8858d72b737066f37eb7a5f94`，不是腾讯官方产品。此仓库发布适配源码，不包含微信输入法官方程序、词库、模型、提取的界面资源或已配对身份。Mac、iPhone 继续使用官方微信输入法，不需额外安装客户端。

## 当前能力与边界

| 能力 | 本项目验证情况 |
| --- | --- |
| 中文输入 | Omarchy x86_64 / Hyprland 0.56.2 上已试打；用户主观反馈比之前 VType 方案更顺手，不代表候选延迟已量化 |
| 模糊音 | 本机启用了 z/zh、c/ch、s/sh、in/ing；其他安装需自行配置 |
| 文字剪贴板 | Linux ↔ Mac、Linux ↔ iPhone 四个方向均有人工成功记录 |
| 图片剪贴板 | 四方向有成功记录；修复 JPEG 管道截断、图片引用解析与格式归一化；iPhone 入口可能需展开微信输入法键盘 |
| 普通文件发送 | 四方向有人工成功记录，使用独立的文件传输窗口和局域网通路 |
| `.md` / `.pdf` 文件直接复制粘贴 | **未实现**。Finder/文件管理器复制文件不等于发送文件；不会监听路径并自动上传 |
| 长期稳定性与性能 | 已做离线回归和本机安装检查；尚未完成长期运行、全部休眠重连与负载测试 |

这些是特定设备与版本的阶段性结果，不是所有版本或网络环境的兼容保证。Mac ↔ iPhone 的文件直接复制粘贴经用户澄清使用 Apple 原生接力，不能当作 Linux 协议验收证据。剪贴板图片转换为可粘贴静态 PNG，不承诺保留 HEIF 元数据、动图、多页或原文件字节。

## 相对上游的主要变化

- 剪贴板读取持续到 EOF，避免管道只读到一部分导致 JPEG 下半幅灰块。
- 图片同步解析官方加密引用，校验下载长度、内容摘要与格式；暂时性下载失败保留重试机会。
- 接收写入前重新检查本地剪贴板，避免网络请求期间的新复制被覆盖；接收后不回传。
- 图片归一化支持 JPEG/JPG、PNG、HEIF/HEIC、AVIF、WebP、GIF、TIFF、BMP；实际可解码格式取决于系统图像插件。
- 临时文件及时清理，持久化同步状态仅保留所需元数据，避免输出原始敏感响应。
- 提供独立同步服务模板和单窗口文件传输入口；不会自动启用服务或替换已有输入法。

[变更与来源](docs/OMARCHY-CHANGES.md) · [构建和运行说明](docs/OMARCHY-OPERATIONS.md) · [验收范围](docs/OMARCHY-VALIDATION.md) · [上游原 README](docs/UPSTREAM-README.md)

## 构建与离线回归

保留上游 CMake 结构，具体依赖见上游文档。需要 Fcitx5 开发文件、LibIME、Qt WebEngine、Clang/libc++、Python 3、cryptography。HEIF/AVIF 转换还需 PyGObject、GdkPixbuf 及相应格式插件。

下面是隔离构建的基本形式；发行版缺少开发包时需要补齐依赖，不会代为安装系统包：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/lib/wetypex-omarchy" \
  -DCMAKE_INSTALL_LIBDIR=lib -DFCITX_INSTALL_USE_FCITX_SYS_PATHS=OFF
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

本次导出验证复用了本机已经校验的依赖 sysroot，并使用系统 libcurl；以上默认 bundled-curl 路径并非本次重新验证的构建组合。完整构建结果和依赖版本见验收文档。构建和离线测试不需账号、配对或官方运行时；实际输入和互通需要用户另行获取运行时并使用官方关联流程。

## 许可

本项目免费提供，保留各部分原许可；“免费”不是额外的非商业限制，也不代表腾讯授权。

- WeTypeX 自有代码及对应补丁：MIT，保留 [LICENSE](LICENSE) 与 [NOTICE](NOTICE)。
- VType 图片兼容模块与其 Python 适配工具：GPLv3，见 [compat/LICENSE](compat/LICENSE)。
- 新增 Omarchy 工具、测试与集成文件：GPL-3.0-only。
- 腾讯的程序、词库、模型、界面资源和服务不受以上开源许可覆盖，不随源码分发。

[许可清单及官方协议核对](LICENSES.md) 说明各部分边界。官方协议包含对未授权知识产权使用和逆向工程的限制；源码开源、免费提供或不分发运行时都不构成已取得官方授权的证明。上游 README 另有个人学习及 24 小时删除表述，本仓库保留原文，没有据此作合法性保证。
