// Focused tests for atomic provider validation and exact typed pointer restore.
#include "../runtime/provider-registry.hpp"
#include "../generated/provider-callbacks.h"
#include "../generated/mpiwrap.h"
#include "../generated/callback-bridges.h"

#include <cassert>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>

namespace PLMD {
namespace mpiwrap_provider_test {

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

std::int32_t fakeOpCreate(plumed_mpiw_user_callback *,
                          std::int32_t, plumed_mpiw_handle_storage *operation) {
  *operation = 17;
  return 0;
}

plumed_mpiw_handle_storage observed_datatype = 0;
int observed_callback_length = 0;
void fakeUserFunction(void *, void *, int *length, PLMD::MPI_Datatype *datatype) {
  observed_callback_length = *length;
  observed_datatype = datatype->storage;
}

plumed_mpiw_handle_storage observed_communicator = 0;
int observed_error_code = 0;
void fakeCommErrorHandler(PLMD::MPI_Comm *communicator, int *error_code, ...) {
  observed_communicator = communicator->storage;
  observed_error_code = *error_code;
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

int run() {
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
  assert(registry.bindings().MPI_Comm_rank_pointer == callback);
  const std::uint64_t raw_communicator = UINT64_C(0x12345678);
  {
    PLMD::mpiwrap_provider::RegistryScope scope(registry);
    const PLMD::MPI_Comm imported = PLMD::mpiwrap_Comm_from_native(&raw_communicator);
    assert(imported.storage == raw_communicator);
    plumed_mpiw_handle_storage exported = 0;
    assert(PLMD::mpiwrap_Comm_export_storage(imported, &exported));
    assert(exported == imported.storage);
  }
  plumed_mpiw_provider_descriptor mpich_descriptor = descriptor;
  mpich_descriptor.native_comm_size = sizeof(std::uint32_t);
  Registry mpich_registry;
  assert(mpich_registry.install(&mpich_descriptor, error));
  const std::uint32_t mpich_communicator = UINT32_C(0x1234abcd);
  {
    PLMD::mpiwrap_provider::RegistryScope outer(registry);
    assert(&PLMD::mpiwrap_provider::currentRegistry() == &registry);
    {
      PLMD::mpiwrap_provider::RegistryScope inner(mpich_registry);
      assert(&PLMD::mpiwrap_provider::currentRegistry() == &mpich_registry);
      const PLMD::MPI_Comm imported = PLMD::mpiwrap_Comm_from_native(
                                        &mpich_communicator);
      assert(imported.storage == mpich_communicator);
    }
    assert(&PLMD::mpiwrap_provider::currentRegistry() == &registry);
  }
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
  assert(registry.install(&conflict, error));
  rank = -1;
  assert(callback(7, &rank) == 0);
  assert(rank == 49);  // Equivalent adapter thunk does not replace published code.

  plumed_mpiw_function_entry mismatched_entry = valid_entry;
  mismatched_entry.signature_version = 2;
  plumed_mpiw_provider_descriptor malformed = makeDescriptor(
        &mismatched_entry, 1, anchor);
  Registry retryable;
  assert(!retryable.install(&malformed, error));
  assert(!retryable.has("MPI_Comm_rank"));
  assert(retryable.bindings().MPI_Comm_rank_pointer == nullptr);
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

  std::atomic<bool> concurrent_success{true};
  Registry concurrent;
  auto install_same = [&concurrent, &descriptor, &concurrent_success]() {
    std::string local_error;
    if (!concurrent.install(&descriptor, local_error)) {
      concurrent_success.store(false);
    }
  };
  std::thread first(install_same);
  std::thread second(install_same);
  first.join();
  second.join();
  assert(concurrent_success.load());

  plumed_mpiw_function_entry op_entry{};
  op_entry.struct_size = sizeof(op_entry);
  op_entry.name = "MPI_Op_create";
  op_entry.signature_id = UINT32_C(0x891a52d1);
  op_entry.signature_version = 2;
  op_entry.function = reinterpret_cast<plumed_mpiw_generic_fn>(&fakeOpCreate);
  static const char user_callback_abi[] = PLUMED_MPIW_CALLBACK_ABI_MPI5_C_V1;
  plumed_mpiw_metadata_entry callback_metadata{};
  callback_metadata.struct_size = sizeof(callback_metadata);
  callback_metadata.value_type = PLUMED_MPIW_VALUE_UTF8;
  callback_metadata.flags = PLUMED_MPIW_METADATA_REQUIRED;
  callback_metadata.name = PLUMED_MPIW_METADATA_MPI_USER_FUNCTION_ABI;
  callback_metadata.value_size = sizeof(user_callback_abi);
  callback_metadata.value = user_callback_abi;
  plumed_mpiw_provider_descriptor callback_descriptor = makeDescriptor(
        &op_entry, 1, anchor);
  callback_descriptor.metadata_count = 1;
  callback_descriptor.metadata = &callback_metadata;
  Registry callbacks;
  assert(callbacks.install(&callback_descriptor, error));
  const auto op_create = callbacks.function<plumed_mpiw_MPI_Op_create_fn>(
                           "MPI_Op_create", UINT32_C(0x891a52d1), 2);
  plumed_mpiw_handle_storage operation = 0;
  assert(op_create(nullptr, 1, &operation) == 0 && operation == 17);

  const auto callback_record = PLMD::mpiwrap_provider::make_user_callback(
                                 &fakeUserFunction);
  const std::uint32_t mpich_datatype = UINT32_C(0x1234abcd);
  std::uint64_t imported_mpich_datatype = 0;
  std::memcpy(&imported_mpich_datatype, &mpich_datatype, sizeof(mpich_datatype));
  callback_record.invoke(callback_record.function, nullptr, nullptr, 3,
                         imported_mpich_datatype);
  assert(observed_datatype == imported_mpich_datatype);
  assert(observed_callback_length == 3);
  const std::uint64_t openmpi_datatype = UINT64_C(0x123456789abcdef0);
  callback_record.invoke(callback_record.function, nullptr, nullptr, 5,
                         openmpi_datatype);
  assert(observed_datatype == openmpi_datatype);
  assert(observed_callback_length == 5);

  const auto error_callback_record =
    PLMD::mpiwrap_provider::make_comm_errhandler_callback(&fakeCommErrorHandler);
  error_callback_record.invoke(error_callback_record.function,
                               UINT64_C(0x87654321), 9);
  assert(observed_communicator == UINT64_C(0x87654321));
  assert(observed_error_code == 9);

  callback_metadata.value_type = PLUMED_MPIW_VALUE_I32;
  Registry wrong_metadata_type;
  assert(!wrong_metadata_type.install(&callback_descriptor, error));
  assert(error.find("callback ABI metadata") != std::string::npos);
  callback_metadata.value_type = PLUMED_MPIW_VALUE_UTF8;

  callback_descriptor.metadata_count = 0;
  Registry missing_callback_metadata;
  assert(!missing_callback_metadata.install(&callback_descriptor, error));
  assert(error.find("MPI_User_function") != std::string::npos);

  static const char unavailable_error_callback[] =
    PLUMED_MPIW_CALLBACK_ABI_UNAVAILABLE;
  plumed_mpiw_metadata_entry unavailable_metadata{};
  unavailable_metadata.struct_size = sizeof(unavailable_metadata);
  unavailable_metadata.value_type = PLUMED_MPIW_VALUE_UTF8;
  unavailable_metadata.flags = PLUMED_MPIW_METADATA_REQUIRED;
  unavailable_metadata.name = PLUMED_MPIW_METADATA_MPI_ERRHANDLER_FUNCTION_ABI;
  unavailable_metadata.value_size = sizeof(unavailable_error_callback);
  unavailable_metadata.value = unavailable_error_callback;
  plumed_mpiw_provider_descriptor thread_provider = descriptor;
  thread_provider.runtime_kind = PLUMED_MPIW_RUNTIME_THREAD_MPI;
  thread_provider.native_fint_size = 0;
  thread_provider.native_fint_signed = 0;
  thread_provider.metadata_count = 1;
  thread_provider.metadata = &unavailable_metadata;
  Registry unavailable_error_handler;
  assert(unavailable_error_handler.install(&thread_provider, error));

  thread_provider.metadata_count = 0;
  thread_provider.metadata = nullptr;
  Registry no_fortran;
  assert(no_fortran.install(&thread_provider, error));
  return 0;
}

}  // namespace mpiwrap_provider_test
}  // namespace PLMD

int main() {
  return PLMD::mpiwrap_provider_test::run();
}
