# 变更与来源

这是从已在本机使用的修改源码整理的独立衍生仓库，不是 GitHub fork 对象；没有伪造上游提交历史。原始文件摘要与来源见 [清单](upstream-source-manifest.json)。

| 模块 | 修改原因 |
| --- | --- |
| src/clipboard/main.cpp | 循环读完管道到 EOF，保留大小上限，修复图片截断 |
| scripts/fcitx5-wetypex-syncd.sh.in | 图片归一化与加密引用处理、本地复制优先、临时文件清理、状态最小化 |
| scripts/fcitx5-wetypex-account.sh.in | 不再将原始错误响应直接写入诊断日志 |
| src/transfer/main.cpp | 可配置运行时、下载及日志路径；默认避免把引擎原始输出写入日志 |
| src/backend/business_services.hpp、file_clipboard_diagnostics.hpp | 可选消息元数据诊断，只用于调查；不实现文件剪贴板通道 |
| compat/cloud/ | VType 的剪贴板协议和协议辅助模块，含本地修改；图片格式模块是本地新增，不存在于记录的 VType 基线 |
| omarchy/ | 图片适配器、独立同步服务模板、文件窗口启动器 |
| tests/ | 同步竞争条件、图片完整性、分段管道输入的离线回归 |
| cmake/OmarchyCompatibility.cmake | 把此前由本机准备脚本复制的兼容组件纳入安装规则及 CTest |

导出整理额外调整：同步脚本默认指向随仓库安装的图片适配器；Python 辅助工具不再引用个人工程绝对路径；本机身份迁移、旧输入法卸载及 CrossPaste 禁用脚本不作为通用安装器发布。

WeTypeX 原始 MIT 许可全文保留于 LICENSES/WeTypeX-MIT.txt，NOTICE 保留不变，原 README 保存为 UPSTREAM-README.md；未继承上游 build/release workflows，避免未经复核即发布自动构建产物。未更改或发布正在使用的官方引擎。新增文档是本项目的说明，不应被理解为上游声明。
