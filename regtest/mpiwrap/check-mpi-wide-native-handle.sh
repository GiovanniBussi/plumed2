#!/usr/bin/env bash
# Native MPI remains usable even when its handles exceed wrapper capacity.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TASK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/plumed-wide-native.XXXXXX")
trap 'rm -rf "$TASK_DIR"' EXIT
cat > "$TASK_DIR/mpi.h" <<'HEADER'
#ifndef MPIW_TEST_MPI_H
#define MPIW_TEST_MPI_H
typedef struct { unsigned char bytes[16]; } MPI_Comm;
typedef void *MPI_Datatype;
typedef void *MPI_Errhandler;
typedef void *MPI_Group;
typedef void *MPI_Op;
typedef void *MPI_Request;
#endif
HEADER
cat > "$TASK_DIR/probe.cpp" <<'SOURCE'
#define __PLUMED_HAS_MPI 1
#include "mpiwrap.h"
#include <cassert>
#include <cstring>
namespace PLMD {
int checkWideNativeHandle() {
#if PLUMED_MPIW_HANDLE_CAPACITY == 8
  static_assert(sizeof(MPI_Comm) > sizeof(plumed_mpiw_handle_storage),
                "8-byte probe requires a wider native handle");
#else
  static_assert(sizeof(MPI_Comm) <= sizeof(plumed_mpiw_handle_storage),
                "16-byte probe requires sufficient byte storage");
#endif
  MPI_Comm original;
  std::memset(&original, 0xa5, sizeof(original));
  const MPI_Comm imported = mpiwrap_Comm_from_native(&original);
  assert(std::memcmp(&original, &imported, sizeof(original)) == 0);
  plumed_mpiw_handle_storage storage;
  std::memset(&storage, 0x5a, sizeof(storage));
  unsigned char storage_before[sizeof(storage)];
  std::memcpy(storage_before, &storage, sizeof(storage));
  if (sizeof(MPI_Comm) > sizeof(storage)) {
    assert(!mpiwrap_Comm_export_storage(imported, &storage));
    assert(std::memcmp(storage_before, &storage, sizeof(storage)) == 0);
  } else {
    assert(mpiwrap_Comm_export_storage(imported, &storage));
    assert(std::memcmp(storage.bytes, &original, sizeof(original)) == 0);
    for (std::size_t i = sizeof(original); i < sizeof(storage.bytes); ++i) {
      assert(storage.bytes[i] == 0);
    }
  }
  unsigned char output[sizeof(MPI_Comm)];
  std::memset(output, 0x5a, sizeof(output));
  assert(!mpiwrap_Comm_export_native(imported, output, sizeof(output) - 1));
  for (unsigned char byte : output) assert(byte == 0x5a);
  assert(mpiwrap_Comm_export_native(imported, output, sizeof(output)));
  assert(std::memcmp(&original, output, sizeof(original)) == 0);
  return 0;
}
}
int main() { return PLMD::checkWideNativeHandle(); }
SOURCE
for capacity in 8 16; do
  "${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic \
    -DPLUMED_MPIW_HANDLE_CAPACITY=$capacity \
    -I"$TASK_DIR" -I"$ROOT/src/mpiwrap" "$TASK_DIR/probe.cpp" -o "$TASK_DIR/probe-$capacity"
  "$TASK_DIR/probe-$capacity"
done
echo '16-byte native communicator preserves bytes at capacity 16 and rejects export at capacity 8'
