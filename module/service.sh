#!/system/bin/sh
# Retry until the owner's encrypted shared storage is available after unlock.
MODDIR=${0%/*}
attempt=0
while [ "$attempt" -lt 150 ]; do
  for storage in /data/media/0 /sdcard; do
    if sh "$MODDIR/config-init.sh" "$MODDIR" "$storage"; then
      log -p i -t SoraXHook "Configuration ready: /sdcard/Documents/MinamotoDev/SoraXHook/config.yaml"
      exit 0
    fi
  done
  attempt=$((attempt + 1))
  sleep 2
done
log -p e -t SoraXHook "Configuration creation failed; using default us"
