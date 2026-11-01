// Focused tests for atomic provider validation and exact typed pointer restore.
#include "../runtime/provider-registry.hpp"
#include "../generated/provider-callbacks.h"

#include <cassert>
#include <cstdint>
#include <string>

namespace {

std::int32_t fakeRank(plumed_mpiw_handle_storage communicator,
                      std::int32_t *rank) {
  *rank = static_cast<std::int32_t>(communicator + 42);
  return 0;
}

void fakeInitialized() {}

std::int32_t fakeRankAlternative(plumed_mpiw_handle_storage,
                                 std::int32_t *rank) {
  *rank = 0;
  return 0;
}

plumed_mpiw_provider_descriptor makeDescriptor(
  plumed_mpiw_function_entry *functions, std::uint64_t function_count,
  plumed_mpiw_generic_fn rank_anchor) {
  plumed_mpiw_provider_descriptor descriptor{};
  descriptor.struct_size = sizeof(descriptor);
  descriptor.abi_major = 1;
  descriptor.abi_minor = 0;
  descriptor.runtime_kind = PLUMED_MPIW_RUNTIME_PROCESS_MPI;
  descriptor.native_comm_size = sizeof(plumed_mpiw_handle_storage);
  descriptor.native_group_size = sizeof(plumed_mpiw_handle_storage);
  descriptor.native_datatype_size = sizeof(plumed_mpiw_handle_storage);
  descriptor.native_request_size = sizeof(plumed_mpiw_handle_storage);
  descriptor.native_op_size = sizeof(plumed_mpiw_handle_storage);
  descriptor.native_errhandler_size = sizeof(plumed_mpiw_handle_storage);
  descriptor.native_status_size = 20;
  descriptor.native_status_align = 4;
  descriptor.native_c_int_size = sizeof(std::int32_t);
  descriptor.native_c_int_signed = 1;
  descriptor.native_fint_size = sizeof(std::int32_t);
  descriptor.native_fint_signed = 1;
  descriptor.wrapper_handle_size = sizeof(plumed_mpiw_handle_storage);
  descriptor.wrapper_status_capacity = PLUMED_MPIW_STATUS_CAPACITY;
  descriptor.provider_name = "test-provider";
  descriptor.provider_version = "1";
  descriptor.function_count = function_count;
  descriptor.functions = functions;
  descriptor.identity_comm_rank = rank_anchor;
  descriptor.identity_initialized = reinterpret_cast<plumed_mpiw_generic_fn>(
                                      &fakeInitialized);
  return descriptor;
}

plumed_mpiw_function_entry rankEntry() {
  plumed_mpiw_function_entry entry{};
  entry.struct_size = sizeof(entry);
  entry.name = "MPI_Comm_rank";
  entry.signature_id = UINT32_C(0x4e7672aa);
  entry.signature_version = 1;
  entry.function = reinterpret_cast<plumed_mpiw_generic_fn>(&fakeRank);
  return entry;
}

}  // namespace

int main() {
  using PLMD::mpiwrap_provider::Registry;
  plumed_mpiw_function_entry valid_entry = rankEntry();
  const auto anchor = reinterpret_cast<plumed_mpiw_generic_fn>(&fakeRank);
  plumed_mpiw_provider_descriptor descriptor = makeDescriptor(&valid_entry, 1, anchor);

  Registry registry;
  std::string error;
  descriptor.native_comm_size = sizeof(plumed_mpiw_handle_storage) + 1;
  assert(!registry.install(&descriptor, error));
  assert(!error.empty());
  assert(!registry.has("MPI_Comm_rank"));

  descriptor.native_comm_size = sizeof(plumed_mpiw_handle_storage);
  assert(registry.install(&descriptor, error));
  assert(error.empty());
  assert(registry.has("MPI_Comm_rank"));
  assert(!registry.has("MPI_Pcontrol"));

  const auto callback = registry.function<plumed_mpiw_MPI_Comm_rank_fn>(
                          "MPI_Comm_rank", UINT32_C(0x4e7672aa), 1);
  assert(callback);
  std::int32_t rank = -1;
  assert(callback(7, &rank) == 0);
  assert(rank == 49);
  assert(!registry.function<plumed_mpiw_MPI_Comm_rank_fn>(
           "MPI_Comm_rank", UINT32_C(0x4e7672aa), 2));

  bool missing_reported = false;
  try {
    registry.require<plumed_mpiw_MPI_Finalize_fn>(
      "MPI_Finalize", UINT32_C(0xf02bd24d), 1);
  } catch (const std::runtime_error &exception) {
    missing_reported = std::string(exception.what()).find("MPI_Finalize") !=
                       std::string::npos;
  }
  assert(missing_reported);

  assert(registry.install(&descriptor, error));
  plumed_mpiw_provider_descriptor conflict = descriptor;
  conflict.identity_comm_rank = reinterpret_cast<plumed_mpiw_generic_fn>(
                                  &fakeInitialized);
  assert(!registry.install(&conflict, error));
  assert(error.find("conflicting") != std::string::npos);
  conflict = descriptor;
  conflict.native_status_size = 24;
  assert(!registry.install(&conflict, error));
  conflict = descriptor;
  valid_entry.function = reinterpret_cast<plumed_mpiw_generic_fn>(
                           &fakeRankAlternative);
  assert(!registry.install(&conflict, error));

  plumed_mpiw_function_entry mismatched_entry = valid_entry;
  mismatched_entry.signature_version = 2;
  plumed_mpiw_provider_descriptor malformed = makeDescriptor(
        &mismatched_entry, 1, anchor);
  Registry retryable;
  assert(!retryable.install(&malformed, error));
  assert(!retryable.has("MPI_Comm_rank"));
  assert(retryable.install(&descriptor, error));

  plumed_mpiw_function_entry duplicate_entries[] = {valid_entry, valid_entry};
  malformed = makeDescriptor(duplicate_entries, 2, anchor);
  Registry duplicates;
  assert(!duplicates.install(&malformed, error));
  assert(error.find("duplicate") != std::string::npos);

  plumed_mpiw_function_entry unknown_required{};
  unknown_required.struct_size = sizeof(unknown_required);
  unknown_required.flags = PLUMED_MPIW_FUNCTION_REQUIRED;
  unknown_required.name = "MPI_Unknown_future";
  unknown_required.function = reinterpret_cast<plumed_mpiw_generic_fn>(&fakeRank);
  malformed = makeDescriptor(&unknown_required, 1, anchor);
  Registry unknown;
  assert(!unknown.install(&malformed, error));
  assert(error.find("unknown required") != std::string::npos);

  malformed = descriptor;
  malformed.struct_size = sizeof(malformed) - 1;
  Registry truncated;
  assert(!truncated.install(&malformed, error));
  assert(error.find("shorter") != std::string::npos);
  return 0;
}
