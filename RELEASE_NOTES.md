# SoraXHook v0.1.1

- 支持修改 X Android 客户端的网络国家码，默认返回 `us`，可通过 YAML 配置。
- 支持解析 ART 压缩符号表，修复 `Failed to find GetMethodShorty` 导致的初始化失败。
- 配置文件支持安装时自动创建及启动后重试创建。
- 增加 API 返回值自检日志，便于检查 Hook 状态。

# 要求

- `Android 8.1(API 27)`以上版本
- `Zygisk Next`