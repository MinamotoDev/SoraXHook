#!/system/bin/sh
if [ "$BOOTMODE" != true ]; then
  abort "Install from your root manager while Android is running."
fi
case "$ARCH" in
  arm64) ABI=arm64-v8a ;;
  x64) ABI=x86_64 ;;
  *) abort "This build supports arm64 and x86_64 only." ;;
esac
[ "$API" -ge 27 ] || abort "Android 8.1 (API 27) or newer is required."
[ -f "$MODPATH/zygisk/$ABI.so" ] || abort "This package does not contain $ABI."
ui_print "SoraXHook: getNetworkCountryIso() override for X"
ui_print "Default country: us"
ui_print "Config: /sdcard/Documents/MinamotoDev/SoraXHook/config.yaml"
ui_print "Restart X after editing the config."
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/service.sh" 0 0 0755
if sh "$MODPATH/config-init.sh" "$MODPATH"; then
  ui_print "Configuration ready."
fi
