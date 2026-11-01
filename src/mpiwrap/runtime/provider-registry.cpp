// Runtime validation and typed lookup for one installed MPI provider.
#include "provider-registry.hpp"

#include <cstddef>
#include <cstring>
#include <unordered_set>
#include <vector>

namespace PLMD {
namespace mpiwrap_provider {
namespace {

struct Signature {
  const char *name;
  std::uint32_t id;
  std::uint32_t version;
};

#define PLUMED_MPIW_NATIVE_FUNCTION(name, id, version) {#name, id, version},
const Signature signatures[] = {
#include "../generated/native-adapter-entries.inc"
};
#undef PLUMED_MPIW_NATIVE_FUNCTION

const Signature *findSignature(const char *name) {
  for (const auto &signature : signatures) {
    if (std::strcmp(signature.name, name) == 0) {
      return &signature;
    }
  }
  return nullptr;
}

bool validString(const char *text) {
  return text && text[0] != '\0' && std::memchr(text, '\0', 4096) != nullptr;
}

bool powerOfTwo(std::uint32_t value) {
  return value != 0 && (value & (value - 1)) == 0;
}

std::uint64_t compatibilityFingerprint(
  const plumed_mpiw_provider_descriptor &descriptor) {
  std::uint64_t hash = UINT64_C(14695981039346656037);
  const std::uint64_t values[] = {
    descriptor.abi_major, descriptor.abi_minor, descriptor.runtime_kind,
    descriptor.flags, descriptor.native_comm_size, descriptor.native_group_size,
    descriptor.native_datatype_size, descriptor.native_request_size,
    descriptor.native_op_size, descriptor.native_errhandler_size,
    descriptor.native_status_size, descriptor.native_status_align,
    descriptor.native_c_int_size, descriptor.native_c_int_signed,
    descriptor.native_fint_size, descriptor.native_fint_signed,
    descriptor.wrapper_handle_size, descriptor.wrapper_status_capacity,
    descriptor.capabilities[0], descriptor.capabilities[1],
    descriptor.capabilities[2], descriptor.capabilities[3]
  };
  for (std::uint64_t value : values) {
    for (unsigned int byte = 0; byte < sizeof(value); ++byte) {
      hash ^= static_cast<unsigned char>(value >> (byte * 8));
      hash *= UINT64_C(1099511628211);
    }
  }
  return hash;
}

bool sameFunctions(
  const std::unordered_map<std::string, Registry::FunctionEntry> &left,
  const std::unordered_map<std::string, Registry::FunctionEntry> &right) {
  if (left.size() != right.size()) {
    return false;
  }
  for (const auto &entry : left) {
    const auto match = right.find(entry.first);
    if (match == right.end() ||
        entry.second.signature_id != match->second.signature_id ||
        entry.second.signature_version != match->second.signature_version ||
        entry.second.flags != match->second.flags ||
        entry.second.function != match->second.function) {
      return false;
    }
  }
  return true;
}

bool setError(std::string &error, const std::string &message) {
  error = message;
  return false;
}

bool validateDescriptor(const plumed_mpiw_provider_descriptor *descriptor,
                        std::unordered_map<std::string, Registry::FunctionEntry> &functions,
                        std::string &provider_name, std::string &provider_version,
                        plumed_mpiw_generic_fn &identity_comm_rank,
                        plumed_mpiw_generic_fn &identity_initialized,
                        const void *&host_runtime_token, std::uint32_t &runtime_kind,
                        std::uint64_t &compatibility_fingerprint,
                        std::string &error) {
  if (!descriptor) {
    return setError(error, "null MPI provider descriptor");
  }
  if (descriptor->struct_size < sizeof(*descriptor)) {
    return setError(error, "MPI provider descriptor is shorter than the required ABI prefix");
  }
  if (descriptor->abi_major != 1) {
    return setError(error, "unsupported MPI provider ABI major");
  }
  if (descriptor->runtime_kind != PLUMED_MPIW_RUNTIME_PROCESS_MPI &&
      descriptor->runtime_kind != PLUMED_MPIW_RUNTIME_THREAD_MPI) {
    return setError(error, "invalid MPI provider runtime kind");
  }
  if (descriptor->flags != 0 || descriptor->capabilities[0] != 0 ||
      descriptor->capabilities[1] != 0 || descriptor->capabilities[2] != 0 ||
      descriptor->capabilities[3] != 0) {
    return setError(error, "provider advertises unsupported required flags or capabilities");
  }
  if (!validString(descriptor->provider_name) || !validString(descriptor->provider_version)) {
    return setError(error, "MPI provider name and version must be nonempty bounded strings");
  }

  const std::uint32_t handle_sizes[] = {
    descriptor->native_comm_size, descriptor->native_group_size,
    descriptor->native_datatype_size, descriptor->native_request_size,
    descriptor->native_op_size, descriptor->native_errhandler_size
  };
  const char *handle_names[] = {"Comm", "Group", "Datatype", "Request", "Op", "Errhandler"};
  for (std::size_t i = 0; i < sizeof(handle_sizes) / sizeof(handle_sizes[0]); ++i) {
    if (handle_sizes[i] == 0 || handle_sizes[i] > sizeof(plumed_mpiw_handle_storage)) {
      return setError(error, std::string("MPI provider native ") + handle_names[i] +
                      " handle exceeds configured integer storage");
    }
  }
  if (descriptor->native_status_size == 0 ||
      descriptor->native_status_size > PLUMED_MPIW_STATUS_CAPACITY ||
      !powerOfTwo(descriptor->native_status_align) ||
      descriptor->native_c_int_size != sizeof(std::int32_t) ||
      descriptor->native_c_int_signed != 1 ||
      descriptor->native_fint_size == 0 ||
      descriptor->native_fint_size > sizeof(std::int32_t) ||
      descriptor->native_fint_signed != 1 ||
      descriptor->wrapper_handle_size != sizeof(plumed_mpiw_handle_storage) ||
      descriptor->wrapper_status_capacity != PLUMED_MPIW_STATUS_CAPACITY) {
    return setError(error, "MPI provider scalar, status, or wrapper representation is incompatible");
  }
  for (std::uint32_t value : descriptor->reserved) {
    if (value != 0) {
      return setError(error, "MPI provider reserved descriptor fields must be zero");
    }
  }
  if (!descriptor->identity_comm_rank || !descriptor->identity_initialized) {
    return setError(error, "MPI provider must supply native identity anchors");
  }
  if ((descriptor->function_count != 0 && !descriptor->functions) ||
      descriptor->function_count > 100000) {
    return setError(error, "invalid MPI provider function table");
  }
  if ((descriptor->metadata_count != 0 && !descriptor->metadata) ||
      descriptor->metadata_count > 100000) {
    return setError(error, "invalid MPI provider metadata table");
  }

  std::unordered_set<std::string> names;
  for (std::uint64_t i = 0; i < descriptor->function_count; ++i) {
    const plumed_mpiw_function_entry &entry = descriptor->functions[i];
    if (entry.struct_size != sizeof(entry) || !validString(entry.name)) {
      return setError(error, "malformed MPI provider function entry");
    }
    if ((entry.flags & ~PLUMED_MPIW_FUNCTION_REQUIRED) != 0) {
      return setError(error, std::string("unknown MPI provider function flags: ") + entry.name);
    }
    if (!names.emplace(entry.name).second) {
      return setError(error, std::string("duplicate MPI provider function entry: ") + entry.name);
    }
    const Signature *signature = findSignature(entry.name);
    if (!signature) {
      if ((entry.flags & PLUMED_MPIW_FUNCTION_REQUIRED) != 0) {
        return setError(error, std::string("unknown required MPI provider function: ") + entry.name);
      }
      continue;
    }
    if (entry.signature_id != signature->id || entry.signature_version != signature->version) {
      return setError(error, std::string("MPI provider signature mismatch: ") + entry.name);
    }
    if (!entry.function) {
      return setError(error, std::string("null MPI provider function: ") + entry.name);
    }
    functions.emplace(entry.name, Registry::FunctionEntry{
      entry.signature_id, entry.signature_version, entry.flags, entry.function});
  }

  names.clear();
  for (std::uint64_t i = 0; i < descriptor->metadata_count; ++i) {
    const plumed_mpiw_metadata_entry &entry = descriptor->metadata[i];
    if (entry.struct_size != sizeof(entry) || !validString(entry.name) ||
        (entry.value_size != 0 && !entry.value)) {
      return setError(error, "malformed MPI provider metadata entry");
    }
    if (!names.emplace(entry.name).second) {
      return setError(error, std::string("duplicate MPI provider metadata: ") + entry.name);
    }
    if ((entry.flags & PLUMED_MPIW_METADATA_REQUIRED) != 0) {
      return setError(error, std::string("unknown required MPI provider metadata: ") + entry.name);
    }
  }

  provider_name = descriptor->provider_name;
  provider_version = descriptor->provider_version;
  identity_comm_rank = descriptor->identity_comm_rank;
  identity_initialized = descriptor->identity_initialized;
  host_runtime_token = descriptor->host_runtime_token;
  runtime_kind = descriptor->runtime_kind;
  compatibility_fingerprint = ::PLMD::mpiwrap_provider::compatibilityFingerprint(*descriptor);
  return true;
}

}  // namespace

Registry::Registry() : identity_comm_rank_(nullptr), identity_initialized_(nullptr),
  host_runtime_token_(nullptr), runtime_kind_(0),
  compatibility_fingerprint_(0), installed_(false) {}

bool Registry::install(const plumed_mpiw_provider_descriptor *descriptor,
                       std::string &error) {
  std::unordered_map<std::string, FunctionEntry> next_functions;
  std::string next_name;
  std::string next_version;
  plumed_mpiw_generic_fn next_comm_rank = nullptr;
  plumed_mpiw_generic_fn next_initialized = nullptr;
  const void *next_token = nullptr;
  std::uint32_t next_runtime_kind = 0;
  std::uint64_t next_fingerprint = 0;
  if (!validateDescriptor(descriptor, next_functions,
                          next_name, next_version, next_comm_rank,
                          next_initialized, next_token, next_runtime_kind,
                          next_fingerprint,
                          error)) {
    return false;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  if (installed_) {
    if (runtime_kind_ == next_runtime_kind &&
        identity_comm_rank_ == next_comm_rank &&
        identity_initialized_ == next_initialized &&
        host_runtime_token_ == next_token &&
        provider_name_ == next_name && provider_version_ == next_version &&
        compatibility_fingerprint_ == next_fingerprint &&
        sameFunctions(functions_, next_functions)) {
      error.clear();
      return true;
    }
    return setError(error, "conflicting MPI provider is already installed");
  }
  functions_.swap(next_functions);
  provider_name_.swap(next_name);
  provider_version_.swap(next_version);
  identity_comm_rank_ = next_comm_rank;
  identity_initialized_ = next_initialized;
  host_runtime_token_ = next_token;
  runtime_kind_ = next_runtime_kind;
  compatibility_fingerprint_ = next_fingerprint;
  installed_ = true;
  error.clear();
  return true;
}

bool Registry::has(const char *name) const {
  if (!name) {
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  return functions_.find(name) != functions_.end();
}

std::string Registry::providerName() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return provider_name_;
}

std::string Registry::providerVersion() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return provider_version_;
}

}  // namespace mpiwrap_provider
}  // namespace PLMD
