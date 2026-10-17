# SoraXHook

An independent Zygisk module that replaces only
`android.telephony.TelephonyManager.getNetworkCountryIso()` (the no-argument
overload) in `com.twitter.android` and its `:subprocess` processes. It does not
require LSPosed. The package's app-data directory is also checked before loading
the hook engine. System server and unrelated apps unload the module.

## Configuration

`/sdcard/Documents/MinamotoDev/SoraXHook/config.yaml`

```yaml
network_country_iso: us
```

The root companion reads the configuration on each target process start.
The default is `us`; missing, unreadable, oversized (>4096 bytes) or malformed
configuration falls back to `us`. ASCII uppercase values are normalized.
The supported YAML schema is one flat `network_country_iso` string containing
two ASCII letters, with optional quotes/comments. Unknown/duplicate keys and
unsupported YAML constructs are rejected. This is not a general YAML parser.

The installer creates the default config when storage is available; the boot
script retries for up to five minutes while waiting for storage. Shared-storage
fallback paths refer to the primary Android user's directory (`/data/media/0`).
Restart all X processes after configuration changes; settings are not live-reloaded.

## Build

Prerequisites: Python 3.11+, Git, JDK 17+, Android NDK r29 (29.0.14206865), Android
Build Tools 36.0.0, CMake 3.31.6 and Ninja 1.12+. Android 27 is the build minimum.
The first native configuration downloads pinned LSPlant, Dobby and XZ Embedded source revisions.

```powershell
python scripts/build.py --ndk C:/Android/Sdk/ndk/29.0.14206865 --build-tools C:/Android/Sdk/build-tools/36.0.0 --java-home 'C:/Program Files/Android/Android Studio/jbr' --cmake C:/Android/Sdk/cmake/3.31.6/bin/cmake.exe --ninja C:/Android/Sdk/cmake/3.31.6/bin/ninja.exe
```

Output: `dist/SoraXHook-0.1.1.zip` and SHA256 checksum. Default architecture is
`arm64-v8a`; pass `--abis arm64-v8a x86_64` to also build x86_64.
Install through a Zygisk-compatible root manager, enable Zygisk Next, and reboot
when ready. `adb logcat -s SoraXHook LSPlant` shows initialization diagnostics.
`Self-check getNetworkCountryIso() -> us: PASS` records a successful direct API
call from inside the target process after hook installation.

The ART resolver reads `.dynsym`, `.symtab`, and the XZ-compressed mini ELF in
`.gnu_debugdata`. XZ dictionary and decompressed output are each limited to 64 MiB.

## Validation boundaries

Hook installation and the in-process API self-check passed on Android 17 /
NothingOS 5.0 / Zygisk Next 1.5.2. Other device configurations have not been tested.
App callers that already inlined this method may bypass its hook; this module
does not globally deoptimize X. The integer overload, SIM country, cell identity,
system properties and telephone service are outside the initial hook scope.

## License

GPL-3.0-or-later for SoraXHook. Third-party components retain their own licenses;
see THIRD_PARTY_NOTICES.md and licenses/. Keep corresponding source available
when distributing compiled packages.
