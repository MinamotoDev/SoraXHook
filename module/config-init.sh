#!/system/bin/sh
# $2 allows isolated tests with a temporary shared-storage root.
MODDIR=$1
STORAGE_ROOT=${2:-/data/media/0}
[ -d "$STORAGE_ROOT/Documents" ] || exit 1
PARENT="$STORAGE_ROOT/Documents/MinamotoDev"
CONFIG_DIR="$PARENT/SoraXHook"
if [ ! -d "$PARENT" ]; then
  mkdir -m 2770 "$PARENT" || exit 1
  chown 1023:1023 "$PARENT" 2>/dev/null || true
fi
if [ ! -d "$CONFIG_DIR" ]; then
  mkdir -m 2770 "$CONFIG_DIR" || exit 1
  chown 1023:1023 "$CONFIG_DIR" 2>/dev/null || true
fi
[ ! -e "$CONFIG_DIR/config.yaml" ] || exit 0
if (set -C; umask 007; cat "$MODDIR/config.yaml" > "$CONFIG_DIR/config.yaml") 2>/dev/null; then
  chown 1023:1023 "$CONFIG_DIR/config.yaml" 2>/dev/null || true
  exit 0
fi
exit 1
