// Provider-mode call-through for the first communication slice.
#include "../generated/mpiwrap.h"
#include "provider-registry.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

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

plumed_mpiw_status copyStatus(const MPI_Status &status) {
  plumed_mpiw_status result{};
  result.source = status.MPI_SOURCE;
  result.tag = status.MPI_TAG;
  result.error = status.MPI_ERROR;
  result.raw_size = status.raw_size;
  std::memcpy(result.raw, status.raw, sizeof(result.raw));
  return result;
}

void copyStatus(const plumed_mpiw_status &status, MPI_Status *result) {
  result->MPI_SOURCE = status.source;
  result->MPI_TAG = status.tag;
  result->MPI_ERROR = status.error;
  result->raw_size = status.raw_size;
  std::memcpy(result->raw, status.raw, sizeof(result->raw));
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
  std::int32_t native_rank = 0;
  if (!rank) {
    throw std::runtime_error("MPI_Comm_rank requires rank output");
  }
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Comm_rank_pointer,
                    "MPI_Comm_rank");
  const int result = function(comm.storage, &native_rank);
  if (result == 0) {
    *rank = native_rank;
  }
  return result;
}

bool mpiwrap_has_Comm_rank() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Comm_rank_pointer != nullptr;
}

int mpiwrap_Comm_size(MPI_Comm comm, int *size) {
  std::int32_t native_size = 0;
  if (!size) {
    throw std::runtime_error("MPI_Comm_size requires size output");
  }
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Comm_size_pointer,
                    "MPI_Comm_size");
  const int result = function(comm.storage, &native_size);
  if (result == 0) {
    *size = native_size;
  }
  return result;
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

int mpiwrap_Irecv(void *buffer, int count, MPI_Datatype datatype, int source,
                  int tag, MPI_Comm comm, MPI_Request *request) {
  if (!request) {
    throw std::runtime_error("MPI_Irecv requires a request output");
  }
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Irecv_pointer,
                    "MPI_Irecv");
  plumed_mpiw_handle_storage request_storage = 0;
  const int result = function(buffer, count, datatype.storage, source, tag,
                              comm.storage, &request_storage);
  if (result == 0)
    std::memcpy(&request->storage, &request_storage,
                sizeof(request_storage));
  return result;
}

bool mpiwrap_has_Irecv() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Irecv_pointer != nullptr;
}

int mpiwrap_Isend(const void *buffer, int count, MPI_Datatype datatype, int dest,
                  int tag, MPI_Comm comm, MPI_Request *request) {
  if (!request) {
    throw std::runtime_error("MPI_Isend requires a request output");
  }
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Isend_pointer,
                    "MPI_Isend");
  plumed_mpiw_handle_storage request_storage = 0;
  const int result = function(buffer, count, datatype.storage, dest, tag,
                              comm.storage, &request_storage);
  if (result == 0)
    std::memcpy(&request->storage, &request_storage,
                sizeof(request_storage));
  return result;
}

bool mpiwrap_has_Isend() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Isend_pointer != nullptr;
}

int mpiwrap_Wait(MPI_Request *request, MPI_Status *status) {
  if (!request) {
    throw std::runtime_error("MPI_Wait requires a request");
  }
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Wait_pointer,
                    "MPI_Wait");
  plumed_mpiw_handle_storage request_storage = 0;
  std::memcpy(&request_storage, &request->storage, sizeof(request_storage));
  plumed_mpiw_status transport_status{};
  const int result = function(&request_storage, status ? &transport_status : nullptr);
  std::memcpy(&request->storage, &request_storage, sizeof(request_storage));
  if (result == 0 && status) {
    copyStatus(transport_status, status);
  }
  return result;
}

bool mpiwrap_has_Wait() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Wait_pointer != nullptr;
}

int mpiwrap_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count) {
  if (!status || !count) {
    throw std::runtime_error("MPI_Get_count requires status and output");
  }
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Get_count_pointer,
                    "MPI_Get_count");
  const plumed_mpiw_status transport_status = copyStatus(*status);
  std::int32_t native_count = 0;
  const int result = function(&transport_status, datatype.storage, &native_count);
  if (result == 0) {
    *count = native_count;
  }
  return result;
}

bool mpiwrap_has_Get_count() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Get_count_pointer != nullptr;
}

int mpiwrap_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                     int dest, int sendtag, void *recvbuf, int recvcount,
                     MPI_Datatype recvtype, int source, int recvtag,
                     MPI_Comm comm, MPI_Status *status) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Sendrecv_pointer,
                    "MPI_Sendrecv");
  plumed_mpiw_status transport_status{};
  const int result = function(sendbuf, sendcount, sendtype.storage, dest, sendtag,
                              recvbuf, recvcount, recvtype.storage, source,
                              recvtag, comm.storage,
                              status ? &transport_status : nullptr);
  if (result == 0 && status) {
    copyStatus(transport_status, status);
  }
  return result;
}

bool mpiwrap_has_Sendrecv() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Sendrecv_pointer != nullptr;
}

#include "../generated/provider-unavailable-functions.inc"

} // namespace PLMD
