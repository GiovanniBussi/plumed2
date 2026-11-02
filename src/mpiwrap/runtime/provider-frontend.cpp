// Provider-mode call-through for the first communication slice.
#include "../generated/mpiwrap.h"
#include "provider-registry.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

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
    mpiwrap_provider::ensureProviderSelected(); \
    type value{}; \
    if (!mpiwrap_provider::currentRegistry().copyValue( \
          #name, &value.storage, sizeof(value.storage))) { \
      PLUMED_MPIW_PROVIDER_CONSTANT_ERROR(name); \
    } \
    return value; \
  }
#define PLUMED_MPIW_PROVIDER_INTEGER(name, suffix, type, select_provider) \
  type mpiwrap_##suffix() { \
    if (select_provider) { mpiwrap_provider::ensureProviderSelected(); } \
    type value{}; \
    if (!mpiwrap_provider::currentRegistry().copyValue(#name, &value, sizeof(value))) { \
      if (!select_provider) { return type{}; } \
      PLUMED_MPIW_PROVIDER_CONSTANT_ERROR(name); \
    } \
    return value; \
  }
#define PLUMED_MPIW_PROVIDER_POINTER(name, suffix, type) \
  type mpiwrap_##suffix() { \
    mpiwrap_provider::ensureProviderSelected(); \
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

int mpiwrap_Initialized(int *flag) {
  if (!flag) {
    throw std::runtime_error("MPI_Initialized requires flag output");
  }
  std::int32_t native_flag = 0;
  if (!mpiwrap_provider::currentRegistry().installed()) {
    *flag = 0;
    return 0;
  }
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Initialized_pointer,
                    "MPI_Initialized");
  const int result = function(&native_flag);
  if (result == 0) {
    *flag = native_flag;
  }
  return result;
}

bool mpiwrap_has_Initialized() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Initialized_pointer != nullptr;
}

int mpiwrap_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
  if (!newcomm) {
    throw std::runtime_error("MPI_Comm_dup requires newcomm output");
  }
  plumed_mpiw_handle_storage result_storage = 0;
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Comm_dup_pointer,
                    "MPI_Comm_dup");
  const int result = function(comm.storage, &result_storage);
  if (result == 0) {
    std::memcpy(&newcomm->storage, &result_storage, sizeof(result_storage));
  }
  return result;
}

bool mpiwrap_has_Comm_dup() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Comm_dup_pointer != nullptr;
}

int mpiwrap_Comm_free(MPI_Comm *comm) {
  if (!comm) {
    throw std::runtime_error("MPI_Comm_free requires comm input/output");
  }
  plumed_mpiw_handle_storage comm_storage = 0;
  std::memcpy(&comm_storage, &comm->storage, sizeof(comm_storage));
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Comm_free_pointer,
                    "MPI_Comm_free");
  const int result = function(&comm_storage);
  if (result == 0) {
    std::memcpy(&comm->storage, &comm_storage, sizeof(comm_storage));
  }
  return result;
}

bool mpiwrap_has_Comm_free() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Comm_free_pointer != nullptr;
}

int mpiwrap_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm) {
  if (!newcomm) {
    throw std::runtime_error("MPI_Comm_split requires newcomm output");
  }
  plumed_mpiw_handle_storage result_storage = 0;
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Comm_split_pointer,
                    "MPI_Comm_split");
  const int result = function(comm.storage, color, key, &result_storage);
  if (result == 0) {
    std::memcpy(&newcomm->storage, &result_storage, sizeof(result_storage));
  }
  return result;
}

bool mpiwrap_has_Comm_split() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Comm_split_pointer != nullptr;
}

int mpiwrap_Abort(MPI_Comm comm, int errorcode) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Abort_pointer,
                    "MPI_Abort");
  return function(comm.storage, errorcode);
}

bool mpiwrap_has_Abort() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Abort_pointer != nullptr;
}

int mpiwrap_Recv(void *buffer, int count, MPI_Datatype datatype, int source,
                 int tag, MPI_Comm comm, MPI_Status *status) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Recv_pointer,
                    "MPI_Recv");
  plumed_mpiw_status transport_status{};
  const int result = function(buffer, count, datatype.storage, source, tag,
                              comm.storage,
                              status ? &transport_status : nullptr);
  if (result == 0 && status) {
    copyStatus(transport_status, status);
  }
  return result;
}

bool mpiwrap_has_Recv() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Recv_pointer != nullptr;
}

int mpiwrap_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                    int root, MPI_Comm comm) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Scatter_pointer,
                    "MPI_Scatter");
  return function(sendbuf, sendcount, sendtype.storage, recvbuf, recvcount,
                  recvtype.storage, root, comm.storage);
}

bool mpiwrap_has_Scatter() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Scatter_pointer != nullptr;
}

int mpiwrap_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   int root, MPI_Comm comm) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Gather_pointer,
                    "MPI_Gather");
  return function(sendbuf, sendcount, sendtype.storage, recvbuf, recvcount,
                  recvtype.storage, root, comm.storage);
}

bool mpiwrap_has_Gather() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Gather_pointer != nullptr;
}

int mpiwrap_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                      MPI_Comm comm) {
  auto function = requireFunction(
                    mpiwrap_provider::currentRegistry().bindings().MPI_Allgather_pointer,
                    "MPI_Allgather");
  return function(sendbuf, sendcount, sendtype.storage, recvbuf, recvcount,
                  recvtype.storage, comm.storage);
}

bool mpiwrap_has_Allgather() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Allgather_pointer != nullptr;
}

int mpiwrap_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, const int recvcounts[], const int displs[],
                       MPI_Datatype recvtype, MPI_Comm comm) {
  if (!recvcounts || !displs) {
    throw std::runtime_error("MPI_Allgatherv requires count and displacement arrays");
  }
  auto &bindings = mpiwrap_provider::currentRegistry().bindings();
  auto get_size = requireFunction(bindings.MPI_Comm_size_pointer, "MPI_Comm_size");
  std::int32_t native_comm_size = 0;
  const int size_result = get_size(comm.storage, &native_comm_size);
  if (size_result != 0) {
    return size_result;
  }
  const int comm_size = native_comm_size;
  if (comm_size < 0) {
    throw std::runtime_error("MPI_Comm_size returned a negative size");
  }
  std::vector<std::int32_t> count_storage(static_cast<std::size_t>(comm_size));
  std::vector<std::int32_t> displacement_storage(static_cast<std::size_t>(comm_size));
  for (int i = 0; i < comm_size; ++i) {
    count_storage[static_cast<std::size_t>(i)] = recvcounts[i];
    displacement_storage[static_cast<std::size_t>(i)] = displs[i];
  }
  auto function = requireFunction(bindings.MPI_Allgatherv_pointer, "MPI_Allgatherv");
  return function(sendbuf, sendcount, sendtype.storage, recvbuf,
                  count_storage.data(), displacement_storage.data(),
                  recvtype.storage, comm.storage);
}

bool mpiwrap_has_Allgatherv() {
  return mpiwrap_provider::currentRegistry().bindings().MPI_Allgatherv_pointer != nullptr;
}

#include "../generated/provider-unavailable-functions.inc"

} // namespace PLMD
