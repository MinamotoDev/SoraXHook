# SoraXHook

为 X Android 客户端修改网络国家码的独立 Zygisk 模块，默认返回 `us`。

## 要求

- `Android 8.1 (API 27)`及以上，`ARM64` 架构。
- 启用兼容的 Zygisk 环境。
- 已验证：`Android 17`、`NothingOS 5.0`、`Zygisk Next 1.5.2`。

## 安装

1. 从 Releases 下载模块 ZIP，在 Root 管理器中安装。
2. 确保 X（`com.twitter.android`）允许 Zygisk 注入。
3. 重启。

## 配置

安装或启动后自动创建：

```text
/sdcard/Documents/MinamotoDev/SoraXHook/config.yaml
```

填写两位国家码，无效时默认使用 `us`。

## 检查运行状态

```sh
adb logcat -s SoraXHook LSPlant
```

成功时显示：

```text
Self-check getNetworkCountryIso() -> us: PASS
```

当前作用于 X 及其子进程中的无参 `TelephonyManager.getNetworkCountryIso()`。
构建说明见 [USAGE.md](USAGE.md)。

## 致谢与许可

基于 [Zygisk API](https://github.com/topjohnwu/zygisk-module-sample)、
[LSPlant](https://github.com/LSPosed/LSPlant)、[Dobby](https://github.com/jmpews/Dobby)
和 [XZ Embedded](https://github.com/tukaani-project/xz-embedded)。

采用 [GPL-3.0-or-later](LICENSE)，第三方许可见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
