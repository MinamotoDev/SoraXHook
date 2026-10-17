#!/usr/bin/env bash
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TEST_DIR=$(mktemp -d)
trap 'rm -rf "$TEST_DIR"' EXIT
if sh "$ROOT/module/config-init.sh" "$ROOT/module" "$TEST_DIR/not-ready"; then
  echo "Expected unavailable storage to fail" >&2
  exit 1
fi
mkdir "$TEST_DIR/Documents"
# NTFS-backed MSYS cannot apply Android permission bits; test file behavior there.
case "$(uname -s)" in
  MINGW*|MSYS*)
    mkdir "$TEST_DIR/bin"
    cat > "$TEST_DIR/bin/mkdir" <<'SHIM'
#!/usr/bin/env sh
if [ "${1:-}" = -m ]; then shift 2; fi
exec /usr/bin/mkdir "$@"
SHIM
    chmod +x "$TEST_DIR/bin/mkdir"
    export PATH="$TEST_DIR/bin:$PATH"
    ;;
esac
sh "$ROOT/module/config-init.sh" "$ROOT/module" "$TEST_DIR"
CONFIG="$TEST_DIR/Documents/MinamotoDev/SoraXHook/config.yaml"
cmp "$ROOT/module/config.yaml" "$CONFIG"
printf 'network_country_iso: jp\n' > "$CONFIG"
sh "$ROOT/module/config-init.sh" "$ROOT/module" "$TEST_DIR"
grep -qx 'network_country_iso: jp' "$CONFIG"
echo "Configuration creation and existing-file tests passed"
