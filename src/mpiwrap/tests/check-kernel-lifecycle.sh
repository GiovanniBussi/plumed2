#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
CXX=${CXX:-c++}
TMP=$(mktemp -d "${TMPDIR:-/tmp}/mpiwrap-kernel-test.XXXXXX")
trap 'rm -rf "$TMP"' EXIT

case "$(uname -s)" in
  Darwin) SHARED_FLAGS=(-dynamiclib) ;;
  *) SHARED_FLAGS=(-shared) ;;
esac

"$CXX" -std=c++17 -fPIC "${SHARED_FLAGS[@]}" \
  "$ROOT/src/mpiwrap/tests/fake-kernel.cpp" -o "$TMP/libkernel-v5.so"
"$CXX" -std=c++17 -fPIC "${SHARED_FLAGS[@]}" \
  "$ROOT/src/mpiwrap/tests/fake-old-kernel.cpp" -o "$TMP/libkernel-v4.so"
"$CXX" -std=c++17 -fPIC "${SHARED_FLAGS[@]}" -I"$ROOT/src/mpiwrap/generated" \
  "$ROOT/src/mpiwrap/tests/fake-provider.cpp" -o "$TMP/libprovider.so"
"$CXX" -std=c++17 -D__PLUMED_HAS_DLOPEN -I"$ROOT/src" \
  "$ROOT/src/mpiwrap/tests/test-kernel-lifecycle.cpp" -ldl -o "$TMP/test-kernel-lifecycle"
"$TMP/test-kernel-lifecycle" "$TMP/libkernel-v5.so" \
  "$TMP/libprovider.so" "$TMP/provider-unloaded" "$TMP/libkernel-v4.so"
echo "kernel open/inject/create/release lifecycle passed"
