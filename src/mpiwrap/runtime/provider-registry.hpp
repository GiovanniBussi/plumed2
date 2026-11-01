// Runtime validation and typed lookup for one installed MPI provider.
#ifndef PLUMED_MPIWRAP_RUNTIME_PROVIDER_REGISTRY_HPP
#define PLUMED_MPIWRAP_RUNTIME_PROVIDER_REGISTRY_HPP

#include "../generated/mpiwrap-pointers.h"

#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

  Registry();
  Registry(const Registry &) = delete;
  Registry &operator=(const Registry &) = delete;

  bool install(const plumed_mpiw_provider_descriptor *descriptor,
               std::string &error);
  bool has(const char *name) const;
  const ProviderBindings &bindings() const noexcept {
    return bindings_;
  }
  std::uint32_t nativeCommSize() const;

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
  ProviderBindings bindings_;
  std::string provider_name_;
  std::string provider_version_;
  plumed_mpiw_generic_fn identity_comm_rank_;
  plumed_mpiw_generic_fn identity_initialized_;
  const void *host_runtime_token_;
  std::uint32_t runtime_kind_;
  std::uint32_t native_comm_size_;
  std::uint64_t compatibility_fingerprint_;
  bool installed_;
};

class RegistryScope {
public:
  explicit RegistryScope(Registry &registry) noexcept;
  ~RegistryScope();
  RegistryScope(const RegistryScope &) = delete;
  RegistryScope &operator=(const RegistryScope &) = delete;

private:
  Registry *previous_;
};

Registry &currentRegistry();

}  // namespace mpiwrap_provider
}  // namespace PLMD

#endif
