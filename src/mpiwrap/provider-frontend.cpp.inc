// Provider-mode helpers and dynamically evaluated constants.
#include "mpiwrap.h"
#include "provider-registry.hpp"

#include <stdexcept>
#include <string>

namespace PLMD {

bool mpiwrap_provider_installed() {
  return mpiwrap_provider::currentRegistry().installed();
}

bool mpiwrap_is_thread_mpi_active() {
  auto &registry = mpiwrap_provider::currentRegistry();
  if (!registry.installed() ||
      registry.runtimeKind() != PLUMED_MPIW_RUNTIME_THREAD_MPI) {
    return false;
  }
  // Inspect the installed provider directly. Calling the public wrapper here
  // could select a provider, which this inquiry must never do.
  const auto initialized = pointers::Initialized;
  int flag = 0;
  return initialized && initialized(&flag) == 0 && flag != 0;
}

void mpiwrap_ensure_provider_selected() {
  if (pointers::ready.load(std::memory_order_acquire)) {
    return;
  }
  mpiwrap_provider::ensureProviderSelected();
  if (!pointers::ready.load(std::memory_order_acquire)) {
    throw std::runtime_error("MPI provider selection completed without publishing function pointers");
  }
}

[[noreturn]] void mpiwrap_unavailable(const char *name) {
  auto &registry = mpiwrap_provider::currentRegistry();
  throw std::runtime_error(std::string("MPI function unavailable: ") + name +
                           " (provider " + registry.providerName() + ")");
}

#define PLUMED_MPIW_PROVIDER_CONSTANT_ERROR(name) \
  throw std::runtime_error(std::string("MPI constant unavailable: ") + #name + \
                           " (provider " + \
                           mpiwrap_provider::currentRegistry().providerName() + ")")
#define PLUMED_MPIW_PROVIDER_HANDLE(name, suffix, type) \
  type mpiwrap_##suffix() { \
    mpiwrap_provider::ensureProviderSelected(); \
    auto &registry = mpiwrap_provider::currentRegistry(); \
    registry.markProviderValueUsed(); \
    type value{}; \
    if (!registry.installed()) return value; \
    if (!pointers::constant_##suffix || \
        pointers::constant_##suffix(&value.storage, sizeof(value.storage)) != 0) { \
      PLUMED_MPIW_PROVIDER_CONSTANT_ERROR(name); \
    } \
    return value; \
  }
#define PLUMED_MPIW_PROVIDER_INTEGER(name, suffix, type) \
  type mpiwrap_##suffix() { \
    mpiwrap_provider::ensureProviderSelected(); \
    auto &registry = mpiwrap_provider::currentRegistry(); \
    registry.markProviderValueUsed(); \
    type value{}; \
    if (!registry.installed()) return value; \
    if (!pointers::constant_##suffix || pointers::constant_##suffix(&value, sizeof(value)) != 0) { \
      PLUMED_MPIW_PROVIDER_CONSTANT_ERROR(name); \
    } \
    return value; \
  }
#define PLUMED_MPIW_PROVIDER_COMPILE_TIME_INTEGER(name, suffix, type) \
  type mpiwrap_##suffix() { return type{}; }
#define PLUMED_MPIW_PROVIDER_POINTER(name, suffix, type) \
  type mpiwrap_##suffix() { \
    mpiwrap_provider::ensureProviderSelected(); \
    auto &registry = mpiwrap_provider::currentRegistry(); \
    registry.markProviderValueUsed(); \
    type value{}; \
    if (!registry.installed()) return value; \
    if (!pointers::constant_##suffix || pointers::constant_##suffix(&value, sizeof(value)) != 0) { \
      PLUMED_MPIW_PROVIDER_CONSTANT_ERROR(name); \
    } \
    return value; \
  }
#include "provider-constant-functions.inc"
#undef PLUMED_MPIW_PROVIDER_POINTER
#undef PLUMED_MPIW_PROVIDER_COMPILE_TIME_INTEGER
#undef PLUMED_MPIW_PROVIDER_INTEGER
#undef PLUMED_MPIW_PROVIDER_HANDLE
#undef PLUMED_MPIW_PROVIDER_CONSTANT_ERROR

} // namespace PLMD
