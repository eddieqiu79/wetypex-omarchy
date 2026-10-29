# 构建、运行与停用

本仓库是源码交付。当前维护机已经部署的版本使用独立用户前缀；此次源码整理不会再次安装、切换输入法或改变服务。

## 隔离检查

完成 README 中的构建后，运行 `ctest --test-dir build --output-on-failure`。测试不访问真实剪贴板、官方网络或账号。`WETYPE_NATIVE_CLIPBOARD_HELPER` 可指定已构建的 Qt 辅助程序，供手工运行 tests 下脚本使用。

可用 `DESTDIR="$PWD/stage" cmake --install build` 只检查安装文件。它不会把运行时或配对凭据装入 stage，也不会启用服务。不要把 stage 当成可以直接运行的可重定位包：上游编译时记录最终安装前缀。

实际运行还需要准备官方运行时。上游工具支持显式提供安装包或同意下载；请先核对官方协议与上游 docs/distribution.md。源码构建成功不代表运行时准备、配对或设备互通成功。

## 当前维护机已经验证的操作

- Fcitx 中选用 `wetypex` 输入法，备用输入法仍保留。
- 通过官方展示的关联码流程管理身份；共享一个规范账号目录，不复制身份制造并行客户端。
- `systemctl --user status wetypex-sync.service` 查看独立同步服务状态。
- `wetypex-files` 打开文件传输窗口；默认接收位置为用户下载目录的 `WeTypeX` 子目录，界面/配置可能改变实际位置。
- `systemctl --user stop wetypex-sync.service` 停止文字与图片后台同步。
- `systemctl --user stop wetypex-transfer.service` 关闭由入口启动的文件传输窗口。
- `systemctl --user start wetypex-sync.service` 恢复已经配置好的同步服务。

以上服务命令仅适用于已经按此布局配置的机器；新 checkout 不会自动产生或启用这些服务。停止同步不等于卸载输入法或撤销设备关联。

`omarchy/wetypex-sync.service.in` 是当前部署服务的可配置模板；CMake 输出含最终路径的版本。使用独立同步服务时，应在已有 Fcitx 用户服务中设置 `WETYPE_SYNCD_LAUNCHER=/usr/bin/true`，由一个服务负责同步，避免输入法附带启动器重复拉起同步。示例 drop-in 不会自动安装或启用。

CrossPaste 或其他跨设备同步程序同时写剪贴板会影响归因和覆盖行为。维护机已关闭 CrossPaste 自启动及旧 VType 服务；本仓库不替其他用户自动关闭程序、修改防火墙或替换输入法。

## 仍未完成的通用安装工作

此仓库尚不提供经多机验证的一键迁移、卸载或回滚脚本。上游 packaging/ 保留作参考，但本衍生版所有打包路径尚未重新验证，尤其需复核 GPL 兼容模块及其许可证是否被完整带入。发布安装包前还需做干净系统安装、重登录、休眠唤醒、断网恢复及实际设备回归。
