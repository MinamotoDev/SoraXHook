#!/usr/bin/env bash
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
XZ_ROOT=$1
TEST_DIR=$(mktemp -d)
trap 'rm -rf "$TEST_DIR"' EXIT
python3 "$ROOT/tests/make_symbol_fixture.py" "$TEST_DIR/fixture.elf"
for name in xz_crc32 xz_crc64 xz_sha256 xz_dec_stream xz_dec_lzma2; do
  cc -O2 -DXZ_USE_CRC64 -DXZ_USE_SHA256 -I"$XZ_ROOT/linux/include/linux" -I"$XZ_ROOT/userspace" \
    -c "$XZ_ROOT/linux/lib/xz/$name.c" -o "$TEST_DIR/$name.o"
done
c++ -std=c++23 -Wall -Wextra -Werror -DXZ_USE_CRC64 -DXZ_USE_SHA256 \
  -I"$ROOT/native" -I"$XZ_ROOT/linux/include/linux" \
  "$ROOT/native/debug_data.cpp" "$ROOT/tests/symbols_test.cpp" "$TEST_DIR"/*.o -o "$TEST_DIR/symbols_test"
"$TEST_DIR/symbols_test" "$TEST_DIR/fixture.elf"
c++ -std=c++23 -Wall -Wextra -Werror -I"$ROOT/native" "$ROOT/tests/config_test.cpp" -o "$TEST_DIR/config_test"
"$TEST_DIR/config_test"
