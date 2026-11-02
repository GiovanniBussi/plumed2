// Runtime validation and typed lookup for one installed MPI provider.
#ifndef PLUMED_MPIWRAP_RUNTIME_PROVIDER_REGISTRY_HPP
#define PLUMED_MPIWRAP_RUNTIME_PROVIDER_REGISTRY_HPP

#include "../generated/mpiwrap-pointers.h"
#include "../generated/mpiwrap.h"

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace PLMD {
namespace mpiwrap_provider {

class Registry {
public:
  struct FunctionEntry {
    std::uint32_t signature_id;
    std::uint32_t signature_version;
    std::uint32_t flags;
    plumed_mpiw_generic_fn function;
  };
  struct ValueEntry {
    std::uint32_t value_type;
    plumed_mpiw_constant_accessor_fn accessor;
    bool operator==(const ValueEntry &other) const {
      return value_type == other.value_type;
    }
  };

  Registry();
  ~Registry();
  Registry(const Registry &) = delete;
  Registry &operator=(const Registry &) = delete;

  bool install(const plumed_mpiw_provider_descriptor *descriptor,
               std::string &error,void *provider_library=nullptr);
  bool has(const char *name) const;
  bool installed() const noexcept {
    return ready_.load(std::memory_order_acquire);
  }
  void markObjectCreated();
  bool beginInternalLaunch(const plumed_mpiw_provider_descriptor *descriptor,
                           std::string &error);
  void finishInternalLaunch() noexcept;
  std::uint32_t runtimeKind() const;
  const ProviderBindings &bindings() const {
    if (!ready_.load(std::memory_order_acquire)) {
      throw std::runtime_error("MPI provider is not installed");
    }
    return bindings_;
  }
  std::uint32_t nativeCommSize() const;
  bool copyValue(const char *name, void *output, std::size_t size) const;

  template<class Function>
  Function function(const char *name, std::uint32_t signature_id,
                    std::uint32_t signature_version) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = functions_.find(name);
    if (found == functions_.end() || found->second.signature_id != signature_id ||
        found->second.signature_version != signature_version) {
      return nullptr;
    }
    // The provider's generic function pointer is converted back to the exact
    // generated function type only after descriptor validation checked its ID.
    return reinterpret_cast<Function>(found->second.function);
  }

  template<class Function>
  Function require(const char *name, std::uint32_t signature_id,
                   std::uint32_t signature_version) const {
    Function result = function<Function>(name, signature_id, signature_version);
    if (!result) {
      throw std::runtime_error(std::string("MPI function unavailable: ") + name +
                               " (provider " + providerName() + ")");
    }
    return result;
  }

  std::string providerName() const;
  std::string providerVersion() const;

private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, FunctionEntry> functions_;
  std::unordered_map<std::string, std::string> metadata_;
  std::unordered_map<std::string, ValueEntry> values_;
  ProviderBindings bindings_;
  std::string provider_name_;
  std::string provider_version_;
  plumed_mpiw_generic_fn identity_comm_rank_;
  plumed_mpiw_generic_fn identity_initialized_;
  const void *host_runtime_token_;
  std::uint32_t runtime_kind_;
  std::uint32_t native_comm_size_;
  std::uint64_t compatibility_fingerprint_;
  std::vector<void *> provider_libraries_;
  bool installed_;
  bool object_created_;
  bool internal_launch_started_;
  bool internal_launch_active_;
  std::atomic<bool> ready_;
};

// The loaded kernel owns one immutable provider registry shared by all its
// rank threads. Injection mutates it only before object construction.
Registry &currentRegistry();
// Selects the embedded thread-MPI provider on first provider-dependent use.
// Explicit provider installation before that point takes precedence.
void ensureProviderSelected();
void noteKernelObjectCreated();
MPI_Comm importComm(Registry &registry, const void *native);
bool exportComm(Registry &registry, MPI_Comm value,
                plumed_mpiw_handle_storage *storage);
bool exportNativeComm(Registry &registry, MPI_Comm value,
                      void *output,std::size_t capacity);

}  // namespace mpiwrap_provider
}  // namespace PLMD

#endif
