// Runtime validation and typed lookup for one installed MPI provider.
#include "provider-registry.hpp"
#include "mpiwrap.h"

#include <cstddef>
#include <cstring>
#include <unordered_set>
#include <vector>
#if defined(__PLUMED_HAS_DLOPEN)
#include <dlfcn.h>
#endif

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
#include "native-adapter-entries.inc"
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
  if (!text) {
    return false;
  }
  for (std::size_t i = 0; i < 4096; ++i) {
    if (text[i] == '\0') {
      return i != 0;
    }
  }
  return false;
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
        entry.second.flags != match->second.flags) {
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
                        std::unordered_map<std::string, std::string> &metadata,
                        std::unordered_map<std::string, Registry::ValueEntry> &values,
                        ProviderBindings &bindings,
                        std::string &provider_name, std::string &provider_version,
                        plumed_mpiw_generic_fn &identity_comm_rank,
                        plumed_mpiw_generic_fn &identity_initialized,
                        const void *&host_runtime_token, std::uint32_t &runtime_kind,
                        std::uint32_t &native_comm_size,
                        std::uint64_t &compatibility_fingerprint,
                        std::string &error) {
  if (!descriptor) {
    return setError(error, "null MPI provider descriptor");
  }
  if (descriptor->struct_size < sizeof(*descriptor)) {
    return setError(error, "MPI provider descriptor is shorter than the required ABI prefix");
  }
  if (descriptor->abi_major != PLUMED_MPIW_PROVIDER_ABI_MAJOR) {
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
                      " handle exceeds configured byte storage");
    }
  }
  if (descriptor->native_status_size == 0 ||
      descriptor->native_status_size > PLUMED_MPIW_STATUS_CAPACITY ||
      !powerOfTwo(descriptor->native_status_align) ||
      descriptor->native_c_int_size != sizeof(std::int32_t) ||
      descriptor->native_c_int_signed != 1 ||
      descriptor->wrapper_handle_size != sizeof(plumed_mpiw_handle_storage) ||
      descriptor->wrapper_status_capacity != PLUMED_MPIW_STATUS_CAPACITY) {
    return setError(error, "MPI provider scalar, status, or wrapper representation is incompatible");
  }
  if (descriptor->runtime_kind == PLUMED_MPIW_RUNTIME_PROCESS_MPI) {
    if (descriptor->native_fint_size == 0 || descriptor->native_fint_signed != 1) {
      return setError(error, "process-MPI provider must report signed MPI_Fint metadata");
    }
  } else if (!((descriptor->native_fint_size == 0 && descriptor->native_fint_signed == 0) ||
               (descriptor->native_fint_size > 0 && descriptor->native_fint_signed == 1))) {
    return setError(error, "thread-MPI provider must report unavailable or signed MPI_Fint metadata");
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
#define PLUMED_MPIW_NATIVE_FUNCTION(func, id, version) \
  if (std::strcmp(entry.name, #func) == 0) { \
    bindings.func##_pointer = reinterpret_cast<ProviderBindings::func##_fn>(entry.function); \
    }
#include "native-adapter-entries.inc"
#undef PLUMED_MPIW_NATIVE_FUNCTION
  }

  names.clear();
  for (std::uint64_t i = 0; i < descriptor->metadata_count; ++i) {
    const plumed_mpiw_metadata_entry &entry = descriptor->metadata[i];
    if (entry.struct_size != sizeof(entry) || !validString(entry.name) ||
        (entry.value_size != 0 && !entry.value) ||
        (entry.flags & ~PLUMED_MPIW_METADATA_REQUIRED) != 0) {
      return setError(error, "malformed MPI provider metadata entry");
    }
    if (!names.emplace(entry.name).second) {
      return setError(error, std::string("duplicate MPI provider metadata: ") + entry.name);
    }
    if (std::strncmp(entry.name, "value:", 6) == 0) {
      const char *name = entry.name + 6;
      bool known = false;
#define PLUMED_MPIW_PROVIDER_CONSTANT(constant_name) \
      if (std::strcmp(name, #constant_name) == 0) known = true;
#include "provider-constant-names.inc"
#undef PLUMED_MPIW_PROVIDER_CONSTANT
      if (!known) {
        if ((entry.flags & PLUMED_MPIW_METADATA_REQUIRED) != 0) {
          return setError(error, std::string("unknown required MPI constant: ") + name);
        }
        continue;
      }
      if (entry.value_type != PLUMED_MPIW_VALUE_ACCESSOR ||
          entry.value_size != sizeof(plumed_mpiw_constant_accessor_fn) ||
          !entry.value || entry.reserved != 0) {
        return setError(error, std::string("invalid provider constant metadata: ") + name);
      }
      Registry::ValueEntry value{};
      value.value_type = entry.value_type;
      std::memcpy(&value.accessor, entry.value, sizeof(value.accessor));
      if (!value.accessor) {
        return setError(error, std::string("null provider constant accessor: ") + name);
      }
      values.emplace(name, std::move(value));
      continue;
    }
    const bool user_callback = std::strcmp(
                                 entry.name, PLUMED_MPIW_METADATA_MPI_USER_FUNCTION_ABI) == 0;
    const bool error_callback = std::strcmp(
                                  entry.name, PLUMED_MPIW_METADATA_MPI_ERRHANDLER_FUNCTION_ABI) == 0;
    if (!user_callback && !error_callback) {
      if ((entry.flags & PLUMED_MPIW_METADATA_REQUIRED) != 0) {
        return setError(error, std::string("unknown required MPI provider metadata: ") + entry.name);
      }
      continue;
    }
    if (entry.value_type != PLUMED_MPIW_VALUE_UTF8 ||
        entry.value_size < 2 || entry.value_size > 64 || !entry.value) {
      return setError(error, std::string("invalid callback ABI metadata: ") + entry.name);
    }
    const char *value = static_cast<const char *>(entry.value);
    if (value[entry.value_size - 1] != '\0') {
      return setError(error, std::string("unterminated callback ABI metadata: ") + entry.name);
    }
    for (std::uint64_t byte = 0; byte + 1 < entry.value_size; ++byte) {
      if (value[byte] == '\0') {
        return setError(error, std::string("embedded NUL in callback ABI metadata: ") + entry.name);
      }
    }
    const std::string value_string(value, static_cast<std::size_t>(entry.value_size - 1));
    const bool valid_variant = value_string == PLUMED_MPIW_CALLBACK_ABI_MPI5_C_V1 ||
                               value_string == PLUMED_MPIW_CALLBACK_ABI_UNAVAILABLE;
    if (!valid_variant) {
      return setError(error, std::string("unsupported callback ABI variant: ") + entry.name);
    }
    metadata.emplace(entry.name, value_string);
  }

  const auto has_function = [&functions](const char *name) {
    return functions.find(name) != functions.end();
  };
  const auto user_abi = metadata.find(PLUMED_MPIW_METADATA_MPI_USER_FUNCTION_ABI);
  if (has_function("MPI_Op_create") &&
      (user_abi == metadata.end() ||
       user_abi->second != PLUMED_MPIW_CALLBACK_ABI_MPI5_C_V1)) {
    return setError(error, "MPI_Op_create requires the supported MPI_User_function ABI metadata");
  }
  if (user_abi != metadata.end() &&
      user_abi->second == PLUMED_MPIW_CALLBACK_ABI_UNAVAILABLE &&
      has_function("MPI_Op_create")) {
    return setError(error, "provider advertises an unavailable MPI_User_function callback");
  }
  const auto error_abi = metadata.find(PLUMED_MPIW_METADATA_MPI_ERRHANDLER_FUNCTION_ABI);
  if (has_function("MPI_Comm_create_errhandler") &&
      (error_abi == metadata.end() ||
       error_abi->second != PLUMED_MPIW_CALLBACK_ABI_MPI5_C_V1)) {
    return setError(error, "MPI_Comm_create_errhandler requires the supported callback ABI metadata");
  }
  if (error_abi != metadata.end() &&
      error_abi->second == PLUMED_MPIW_CALLBACK_ABI_UNAVAILABLE &&
      has_function("MPI_Comm_create_errhandler")) {
    return setError(error, "provider advertises an unavailable error-handler callback");
  }

  provider_name = descriptor->provider_name;
  provider_version = descriptor->provider_version;
  identity_comm_rank = descriptor->identity_comm_rank;
  identity_initialized = descriptor->identity_initialized;
  host_runtime_token = descriptor->host_runtime_token;
  runtime_kind = descriptor->runtime_kind;
  native_comm_size = descriptor->native_comm_size;
  compatibility_fingerprint = ::PLMD::mpiwrap_provider::compatibilityFingerprint(*descriptor);
  return true;
}

}  // namespace

Registry::Registry() : identity_comm_rank_(nullptr), identity_initialized_(nullptr),
  host_runtime_token_(nullptr), runtime_kind_(0), native_comm_size_(0),
  compatibility_fingerprint_(0), installed_(false), object_created_(false),
  internal_launch_started_(false), internal_launch_active_(false), ready_(false) {}

Registry::~Registry() {
#if defined(__PLUMED_HAS_DLOPEN)
  for (void *library : provider_libraries_) {
    if (library) {
      dlclose(library);
    }
  }
#endif
}

bool Registry::install(const plumed_mpiw_provider_descriptor *descriptor,
                       std::string &error,void *provider_library) {
  std::unordered_map<std::string, FunctionEntry> next_functions;
  std::unordered_map<std::string, std::string> next_metadata;
  std::unordered_map<std::string, ValueEntry> next_values;
  ProviderBindings next_bindings{};
  std::string next_name;
  std::string next_version;
  plumed_mpiw_generic_fn next_comm_rank = nullptr;
  plumed_mpiw_generic_fn next_initialized = nullptr;
  const void *next_token = nullptr;
  std::uint32_t next_runtime_kind = 0;
  std::uint32_t next_native_comm_size = 0;
  std::uint64_t next_fingerprint = 0;
  if (!validateDescriptor(descriptor, next_functions, next_metadata, next_values,
                          next_bindings,
                          next_name, next_version, next_comm_rank,
                          next_initialized, next_token, next_runtime_kind,
                          next_native_comm_size, next_fingerprint,
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
        sameFunctions(functions_, next_functions) && metadata_ == next_metadata &&
        values_ == next_values) {
      if (provider_library) {
        provider_libraries_.push_back(provider_library);
      }
      error.clear();
      return true;
    }
    return setError(error, "conflicting MPI provider is already installed");
  }
  if (object_created_) {
    return setError(error, "MPI provider must be installed before the first PLUMED object is created");
  }
  if (provider_library) {
    provider_libraries_.push_back(provider_library);
  }
  functions_.swap(next_functions);
  metadata_.swap(next_metadata);
  values_.swap(next_values);
  bindings_ = next_bindings;
  provider_name_.swap(next_name);
  provider_version_.swap(next_version);
  identity_comm_rank_ = next_comm_rank;
  identity_initialized_ = next_initialized;
  host_runtime_token_ = next_token;
  runtime_kind_ = next_runtime_kind;
  native_comm_size_ = next_native_comm_size;
  compatibility_fingerprint_ = next_fingerprint;
  installed_ = true;
  if (this == &currentRegistry()) {
    // Publish the complete immutable function table before either readiness
    // flag becomes visible to readers.
#define PLUMED_MPIW_PROVIDER_POINTER(field, suffix) \
    pointers::suffix = next_bindings.field##_pointer;
#define PLUMED_MPIW_PROVIDER_VALUE(name, suffix) \
    { \
      const auto value = values_.find(#name); \
      pointers::suffix = value == values_.end() ? nullptr : value->second.accessor; \
    }
#include "provider-pointer-publish.inc"
#undef PLUMED_MPIW_PROVIDER_VALUE
#undef PLUMED_MPIW_PROVIDER_POINTER
    pointers::ready.store(true, std::memory_order_release);
  }
  ready_.store(true, std::memory_order_release);
  error.clear();
  return true;
}

void Registry::markObjectCreated() {
  std::lock_guard<std::mutex> lock(mutex_);
  object_created_ = true;
}

bool Registry::beginInternalLaunch(
  const plumed_mpiw_provider_descriptor *descriptor, std::string &error) {
  if (!descriptor) {
    return setError(error, "missing embedded thread-MPI provider descriptor");
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (!installed_ || runtime_kind_ != PLUMED_MPIW_RUNTIME_THREAD_MPI ||
      identity_comm_rank_ != descriptor->identity_comm_rank ||
      identity_initialized_ != descriptor->identity_initialized ||
      host_runtime_token_ != descriptor->host_runtime_token ||
      compatibility_fingerprint_ != compatibilityFingerprint(*descriptor)) {
    return setError(error, "internal launch requires the embedded thread-MPI provider");
  }
  if (object_created_) {
    return setError(error, "thread-MPI launch must precede the first PLUMED object");
  }
  if (internal_launch_started_) {
    return setError(error, "this kernel's thread-MPI runtime has already been launched");
  }
  internal_launch_started_ = true;
  internal_launch_active_ = true;
  error.clear();
  return true;
}

void Registry::finishInternalLaunch() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  internal_launch_active_ = false;
}

std::uint32_t Registry::runtimeKind() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return installed_ ? runtime_kind_ : 0;
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

std::uint32_t Registry::nativeCommSize() const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!installed_) {
    throw std::runtime_error("MPI provider is not installed");
  }
  return native_comm_size_;
}

bool Registry::copyValue(const char *name, void *output, std::size_t size) const {
  if (!name || !output) {
    return false;
  }
  plumed_mpiw_constant_accessor_fn accessor = nullptr;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!installed_) {
      return false;
    }
    const auto found = values_.find(name);
    if (found == values_.end() || found->second.value_type !=
        PLUMED_MPIW_VALUE_ACCESSOR) {
      return false;
    }
    accessor = found->second.accessor;
  }
  return accessor && size <= UINT32_MAX &&
         accessor(output, static_cast<std::uint32_t>(size)) == 0;
}

namespace {
Registry &kernel_registry() {
  static Registry registry;
  return registry;
}
}

Registry &currentRegistry() {
  return kernel_registry();
}

#if defined(__PLUMED_HAS_MPI)
void ensureProviderSelected() {}
void noteKernelObjectCreated() {
  currentRegistry().markObjectCreated();
}
#endif

MPI_Comm importComm(Registry &registry, const void *native) {
  MPI_Comm result{};
  if (native) {
    const std::uint32_t native_size = registry.nativeCommSize();
    if (native_size > sizeof(result.storage)) {
      throw std::runtime_error("native MPI_Comm exceeds configured wrapper storage");
    }
    std::memcpy(&result.storage, native, native_size);
  }
  return result;
}

bool exportComm(Registry &registry, MPI_Comm value,
                plumed_mpiw_handle_storage *storage) {
  if (!storage) {
    return false;
  }
  const std::uint32_t native_size = registry.nativeCommSize();
  if (native_size > sizeof(value.storage)) {
    return false;
  }
  *storage = value.storage;
  return true;
}

bool exportNativeComm(Registry &registry, MPI_Comm value, void *output,
                      std::size_t capacity) {
  if (!output) {
    return false;
  }
  const std::uint32_t native_size = registry.nativeCommSize();
  if (native_size == 0 || native_size > sizeof(value.storage) ||
      native_size > capacity) {
    return false;
  }
  std::memcpy(output, &value.storage, native_size);
  return true;
}

}  // namespace mpiwrap_provider

#if !defined(__PLUMED_HAS_MPI) && !defined(PLUMED_MPIW_NATIVE_ADAPTER)

MPI_Comm mpiwrap_Comm_from_native(const void *native) {
  return mpiwrap_provider::importComm(mpiwrap_provider::currentRegistry(), native);
}

bool mpiwrap_Comm_export_storage(MPI_Comm value,
                                 plumed_mpiw_handle_storage *storage) {
  return mpiwrap_provider::exportComm(mpiwrap_provider::currentRegistry(), value,
                                      storage);
}

bool mpiwrap_Comm_export_native(MPI_Comm value, void *output,
                                std::size_t capacity) {
  return mpiwrap_provider::exportNativeComm(
           mpiwrap_provider::currentRegistry(), value, output, capacity);
}
#endif

}  // namespace PLMD
