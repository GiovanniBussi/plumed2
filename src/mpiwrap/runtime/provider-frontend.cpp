// Provider-mode call-through for the first communication slice.
#include "../generated/mpiwrap.h"
#include "provider-registry.hpp"

namespace PLMD {
namespace {

template<class Function>
Function requireFunction(Function function, const char *name) {
  if (!function) {
    throw std::runtime_error(std::string("MPI function unavailable: ") + name +
                             " (provider " +
                             mpiwrap_provider::currentRegistry().providerName() + ")");
  }
  return function;
}

}

#define PLUMED_MPIW_PROVIDER_CONSTANT_ERROR(name) \
  throw std::runtime_error(std::string("MPI constant unavailable: ") + #name + \
                           " (provider " + \
                           mpiwrap_provider::currentRegistry().providerName() + ")")
#define PLUMED_MPIW_PROVIDER_HANDLE(name, suffix, type) \
  type mpiwrap_##suffix() { \
    type value{}; \
    if (!mpiwrap_provider::currentRegistry().copyValue( \
          #name, &value.storage, sizeof(value.storage))) { \
      PLUMED_MPIW_PROVIDER_CONSTANT_ERROR(name); \
    } \
    return value; \
  }
#define PLUMED_MPIW_PROVIDER_INTEGER(name, suffix, type) \
  type mpiwrap_##suffix() { \
    type value{}; \
    if (!mpiwrap_provider::currentRegistry().copyValue(#name, &value, sizeof(value))) { \
      PLUMED_MPIW_PROVIDER_CONSTANT_ERROR(name); \
    } \
    return value; \
  }
#define PLUMED_MPIW_PROVIDER_POINTER(name, suffix, type) \
  type mpiwrap_##suffix() { \
    type value{}; \
    if (!mpiwrap_provider::currentRegistry().copyValue(#name, &value, sizeof(value))) { \
      PLUMED_MPIW_PROVIDER_CONSTANT_ERROR(name); \
    } \
    return value; \
  }
#include "../generated/provider-constant-functions.inc"
#undef PLUMED_MPIW_PROVIDER_POINTER
#undef PLUMED_MPIW_PROVIDER_INTEGER
#undef PLUMED_MPIW_PROVIDER_HANDLE
#undef PLUMED_MPIW_PROVIDER_CONSTANT_ERROR

int mpiwrap_Comm_rank(MPI_Comm comm, int *rank) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Comm_rank_pointer,
                    "MPI_Comm_rank");
  return function(comm.storage, reinterpret_cast<std::int32_t *>(rank));
}

bool mpiwrap_has_Comm_rank() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Comm_rank_pointer != nullptr;
}

int mpiwrap_Comm_size(MPI_Comm comm, int *size) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Comm_size_pointer,
                    "MPI_Comm_size");
  return function(comm.storage, reinterpret_cast<std::int32_t *>(size));
}

bool mpiwrap_has_Comm_size() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Comm_size_pointer != nullptr;
}

int mpiwrap_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
                  MPI_Comm comm) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Bcast_pointer,
                    "MPI_Bcast");
  return function(buffer, count, datatype.storage, root, comm.storage);
}

bool mpiwrap_has_Bcast() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Bcast_pointer != nullptr;
}

int mpiwrap_Allreduce(const void *sendbuf, void *recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Allreduce_pointer,
                    "MPI_Allreduce");
  return function(sendbuf, recvbuf, count, datatype.storage, op.storage,
                  comm.storage);
}

bool mpiwrap_has_Allreduce() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Allreduce_pointer != nullptr;
}

int mpiwrap_Barrier(MPI_Comm comm) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Barrier_pointer,
                    "MPI_Barrier");
  return function(comm.storage);
}

bool mpiwrap_has_Barrier() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Barrier_pointer != nullptr;
}

} // namespace PLMD
