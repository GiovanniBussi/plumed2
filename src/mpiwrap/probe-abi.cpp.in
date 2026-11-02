#include "provider-abi-proposal.h"

#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>

static_assert(std::is_standard_layout<plumed_mpiw_function_entry>::value,
              "function entry must remain standard layout");
static_assert(std::is_standard_layout<plumed_mpiw_metadata_entry>::value,
              "metadata entry must remain standard layout");
static_assert(std::is_standard_layout<plumed_mpiw_provider_descriptor>::value,
              "provider descriptor must remain standard layout");

namespace PLMD {
using sample_function = int (*)(int);

struct CommHandle {
  uintptr_t value;
  bool operator==(const CommHandle &other) const {
    return value == other.value;
  }
  bool operator!=(const CommHandle &other) const {
    return value != other.value;
  }
};

int sample_callback(int value) {
  return value + 3;
}

int run_probe() {
  static_assert(sizeof(void *) <= sizeof(uintptr_t), "pointer handles exceed uintptr_t storage");
  const sample_function typed = &sample_callback;
  const plumed_mpiw_generic_fn carrier = reinterpret_cast<plumed_mpiw_generic_fn>(typed);
  const sample_function restored = reinterpret_cast<sample_function>(carrier);
  if (restored(7) != 10) {
    return 1;
  }

  int native_integer_handle = 19;
  uintptr_t integer_storage = 0;
  std::memcpy(&integer_storage, &native_integer_handle, sizeof(native_integer_handle));
  int restored_integer_handle = 0;
  std::memcpy(&restored_integer_handle, &integer_storage, sizeof(restored_integer_handle));
  if (restored_integer_handle != native_integer_handle) {
    return 2;
  }
  if (!(CommHandle{integer_storage} == CommHandle{integer_storage}) ||
      !(CommHandle{integer_storage} != CommHandle{integer_storage + 1})) {
    return 5;
  }

  int pointed_value = 23;
  int *native_pointer_handle = &pointed_value;
  uintptr_t pointer_storage = 0;
  std::memcpy(&pointer_storage, &native_pointer_handle, sizeof(native_pointer_handle));
  int *restored_pointer_handle = nullptr;
  std::memcpy(&restored_pointer_handle, &pointer_storage, sizeof(restored_pointer_handle));
  if (restored_pointer_handle != native_pointer_handle || *restored_pointer_handle != 23) {
    return 3;
  }

  plumed_mpiw_function_entry entries[2] = {};
  if (reinterpret_cast<unsigned char *>(&entries[1]) -
      reinterpret_cast<unsigned char *>(&entries[0]) != sizeof(plumed_mpiw_function_entry)) {
    return 4;
  }

  std::printf("status sizeof=%zu align=%zu source=%zu tag=%zu error=%zu raw_size=%zu raw=%zu\n",
              sizeof(plumed_mpiw_status), alignof(plumed_mpiw_status),
              offsetof(plumed_mpiw_status, source), offsetof(plumed_mpiw_status, tag),
              offsetof(plumed_mpiw_status, error), offsetof(plumed_mpiw_status, raw_size),
              offsetof(plumed_mpiw_status, raw));
  std::printf("descriptor sizeof=%zu align=%zu functions=%zu metadata=%zu identity_rank=%zu token=%zu\n",
              sizeof(plumed_mpiw_provider_descriptor), alignof(plumed_mpiw_provider_descriptor),
              offsetof(plumed_mpiw_provider_descriptor, functions),
              offsetof(plumed_mpiw_provider_descriptor, metadata),
              offsetof(plumed_mpiw_provider_descriptor, identity_comm_rank),
              offsetof(plumed_mpiw_provider_descriptor, host_runtime_token));
  std::printf("function-entry sizeof=%zu align=%zu name=%zu signature=%zu pointer=%zu\n",
              sizeof(plumed_mpiw_function_entry), alignof(plumed_mpiw_function_entry),
              offsetof(plumed_mpiw_function_entry, name),
              offsetof(plumed_mpiw_function_entry, signature_id),
              offsetof(plumed_mpiw_function_entry, function));
  std::printf("metadata-entry sizeof=%zu align=%zu name=%zu value-size=%zu value=%zu\n",
              sizeof(plumed_mpiw_metadata_entry), alignof(plumed_mpiw_metadata_entry),
              offsetof(plumed_mpiw_metadata_entry, name),
              offsetof(plumed_mpiw_metadata_entry, value_size),
              offsetof(plumed_mpiw_metadata_entry, value));
  std::printf("uintptr_t=%zu wrapper-handle=%u wrapper-status-capacity=%u\n",
              sizeof(uintptr_t),
              static_cast<unsigned>(sizeof(uintptr_t)), 128U);
  std::printf("native-C-int sizeof=%zu signed=%d MPI_Fint metadata=provider-reported\n",
              sizeof(int), std::numeric_limits<int>::is_signed ? 1 : 0);
  std::printf("generic-function-pointer roundtrip=pass integer-handle-bytes=pass pointer-handle-bytes=pass entry-stride=pass\n");
  return 0;
}
}

int main() {
  return PLMD::run_probe();
}
