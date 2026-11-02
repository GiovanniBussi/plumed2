// Stable tMPI provider entries for the first runtime communication slice.
#include "provider-abi.h"
#include "provider-callbacks.h"

#include <thread_mpi_tmpi.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>
#include <vector>

namespace PLMD {
namespace thread_mpi_adapter {

static_assert(sizeof(plumed_mpiw_handle_storage) >= sizeof(thread_mpi::tMPI_Comm),
              "wrapper storage cannot hold tMPI communicator pointers");
static_assert(sizeof(plumed_mpiw_handle_storage) >= sizeof(thread_mpi::tMPI_Datatype),
              "wrapper storage cannot hold tMPI datatype pointers");
static_assert(sizeof(plumed_mpiw_handle_storage) >= sizeof(thread_mpi::tMPI_Request),
              "wrapper storage cannot hold tMPI request pointers");
static_assert(std::is_same<int, std::int32_t>::value,
              "tMPI C int outputs must match the provider ABI");

thread_mpi::tMPI_Comm decode_comm(plumed_mpiw_handle_storage storage) {
  thread_mpi::tMPI_Comm native{};
  std::memcpy(&native, &storage, sizeof(native));
  return native;
}

thread_mpi::tMPI_Datatype decode_datatype(plumed_mpiw_handle_storage storage) {
  thread_mpi::tMPI_Datatype native{};
  std::memcpy(&native, &storage, sizeof(native));
  return native;
}

thread_mpi::tMPI_Op decode_op(plumed_mpiw_handle_storage storage) {
  thread_mpi::tMPI_Op native{};
  std::memcpy(&native, &storage, sizeof(native));
  return native;
}

thread_mpi::tMPI_Request decode_request(plumed_mpiw_handle_storage storage) {
  thread_mpi::tMPI_Request native{};
  std::memcpy(&native, &storage, sizeof(native));
  return native;
}

plumed_mpiw_handle_storage encode_request(thread_mpi::tMPI_Request request) {
  plumed_mpiw_handle_storage storage = 0;
  std::memcpy(&storage, &request, sizeof(request));
  return storage;
}

void export_status(const thread_mpi::tMPI_Status &native,
                   plumed_mpiw_status *status) {
  status->source = native.TMPI_SOURCE;
  status->tag = native.TMPI_TAG;
  status->error = native.TMPI_ERROR;
  status->raw_size = sizeof(native);
  std::memset(status->raw, 0, sizeof(status->raw));
  std::memcpy(status->raw, &native, sizeof(native));
}

bool import_status(const plumed_mpiw_status *status,
                   thread_mpi::tMPI_Status *native) {
  if (!status || !native || status->raw_size != sizeof(*native)) {
    return false;
  }
  std::memcpy(native, status->raw, sizeof(*native));
  return true;
}

std::int32_t comm_rank(plumed_mpiw_handle_storage comm, std::int32_t *rank) {
  return thread_mpi::tMPI_Comm_rank(decode_comm(comm), rank);
}

std::int32_t comm_size(plumed_mpiw_handle_storage comm, std::int32_t *size) {
  return thread_mpi::tMPI_Comm_size(decode_comm(comm), size);
}

std::int32_t bcast(void *buffer, std::int32_t count,
                   plumed_mpiw_handle_storage datatype, std::int32_t root,
                   plumed_mpiw_handle_storage comm) {
  return thread_mpi::tMPI_Bcast(buffer, count, decode_datatype(datatype), root,
                                decode_comm(comm));
}

std::size_t builtin_type_size(thread_mpi::tMPI_Datatype datatype) {
  using namespace thread_mpi;
  if (datatype == TMPI_CHAR) {
    return sizeof(char);
  }
  if (datatype == TMPI_SIGNED_CHAR) {
    return sizeof(signed char);
  }
  if (datatype == TMPI_UNSIGNED_CHAR) {
    return sizeof(unsigned char);
  }
  if (datatype == TMPI_SHORT) {
    return sizeof(short);
  }
  if (datatype == TMPI_UNSIGNED_SHORT) {
    return sizeof(unsigned short);
  }
  if (datatype == TMPI_INT) {
    return sizeof(int);
  }
  if (datatype == TMPI_UNSIGNED) {
    return sizeof(unsigned int);
  }
  if (datatype == TMPI_LONG) {
    return sizeof(long);
  }
  if (datatype == TMPI_UNSIGNED_LONG) {
    return sizeof(unsigned long);
  }
#ifdef SIZEOF_LONG_LONG_INT
  if (datatype == TMPI_LONG_LONG || datatype == TMPI_LONG_LONG_INT) {
    return sizeof(long long);
  }
  if (datatype == TMPI_UNSIGNED_LONG_LONG) {
    return sizeof(unsigned long long);
  }
#endif
  if (datatype == TMPI_C_BOOL) {
    return sizeof(bool);
  }
  if (datatype == TMPI_INT64_T) {
    return sizeof(std::int64_t);
  }
  if (datatype == TMPI_FLOAT) {
    return sizeof(float);
  }
  if (datatype == TMPI_DOUBLE) {
    return sizeof(double);
  }
  if (datatype == TMPI_LONG_DOUBLE) {
    return sizeof(long double);
  }
  if (datatype == TMPI_BYTE) {
    return 1;
  }
  return 0;
}

std::int32_t allreduce(const void *sendbuf, void *recvbuf, std::int32_t count,
                       plumed_mpiw_handle_storage datatype,
                       plumed_mpiw_handle_storage op,
                       plumed_mpiw_handle_storage comm) {
  auto native_type = decode_datatype(datatype);
  auto native_op = decode_op(op);
  auto native_comm = decode_comm(comm);
  int size = 0;
  const int size_result = thread_mpi::tMPI_Comm_size(native_comm, &size);
  if (size_result != thread_mpi::TMPI_SUCCESS) {
    return size_result;
  }
  if (count < 0) {
    return thread_mpi::TMPI_ERR_TYPE;
  }
  if (count > 0 && !recvbuf) {
    return thread_mpi::TMPI_ERR_BUF;
  }
  if (size == 1 && sendbuf != nullptr) {
    if (count == 0) {
      return thread_mpi::TMPI_SUCCESS;
    }
    const std::size_t element_size = builtin_type_size(native_type);
    if (element_size == 0 || static_cast<std::size_t>(count) >
        static_cast<std::size_t>(-1) / element_size) {
      return thread_mpi::TMPI_ERR_TYPE;
    }
    if (!sendbuf) {
      return thread_mpi::TMPI_ERR_BUF;
    }
    std::memcpy(recvbuf, sendbuf, element_size * static_cast<std::size_t>(count));
    return thread_mpi::TMPI_SUCCESS;
  }
  return thread_mpi::tMPI_Allreduce(sendbuf, recvbuf, count, native_type,
                                    native_op, native_comm);
}

std::int32_t barrier(plumed_mpiw_handle_storage comm) {
  return thread_mpi::tMPI_Barrier(decode_comm(comm));
}

std::int32_t initialized(std::int32_t *flag) {
  if (!flag) {
    return thread_mpi::TMPI_ERR_BUF;
  }
  return thread_mpi::tMPI_Initialized(flag);
}

std::int32_t comm_dup(plumed_mpiw_handle_storage comm,
                      plumed_mpiw_handle_storage *newcomm) {
  if (!newcomm) {
    return thread_mpi::TMPI_ERR_COMM;
  }
  thread_mpi::tMPI_Comm native_newcomm{};
  const int result = thread_mpi::tMPI_Comm_dup(decode_comm(comm), &native_newcomm);
  if (result == thread_mpi::TMPI_SUCCESS) {
    *newcomm = 0;
    std::memcpy(newcomm, &native_newcomm, sizeof(native_newcomm));
  }
  return result;
}

std::int32_t comm_free(plumed_mpiw_handle_storage *comm) {
  if (!comm) {
    return thread_mpi::TMPI_ERR_COMM;
  }
  thread_mpi::tMPI_Comm native_comm = decode_comm(*comm);
  const int result = thread_mpi::tMPI_Comm_free(&native_comm);
  if (result == thread_mpi::TMPI_SUCCESS) {
    *comm = 0;
    std::memcpy(comm, &native_comm, sizeof(native_comm));
  }
  return result;
}

std::int32_t comm_split(plumed_mpiw_handle_storage comm, std::int32_t color,
                        std::int32_t key,
                        plumed_mpiw_handle_storage *newcomm) {
  if (!newcomm) {
    return thread_mpi::TMPI_ERR_COMM;
  }
  thread_mpi::tMPI_Comm native_newcomm{};
  const int result = thread_mpi::tMPI_Comm_split(
                       decode_comm(comm), color, key, &native_newcomm);
  if (result == thread_mpi::TMPI_SUCCESS) {
    *newcomm = 0;
    std::memcpy(newcomm, &native_newcomm, sizeof(native_newcomm));
  }
  return result;
}

std::int32_t abort_comm(plumed_mpiw_handle_storage comm, std::int32_t code) {
  return thread_mpi::tMPI_Abort(decode_comm(comm), code);
}

std::int32_t recv(void *buffer, std::int32_t count,
                  plumed_mpiw_handle_storage datatype, std::int32_t source,
                  std::int32_t tag, plumed_mpiw_handle_storage comm,
                  plumed_mpiw_status *status) {
  thread_mpi::tMPI_Status native_status{};
  const int result = thread_mpi::tMPI_Recv(
                       buffer, count, decode_datatype(datatype), source, tag,
                       decode_comm(comm), status ? &native_status : nullptr);
  if (result == thread_mpi::TMPI_SUCCESS && status) {
    export_status(native_status, status);
  }
  return result;
}

std::int32_t scatter(const void *sendbuf, std::int32_t sendcount,
                     plumed_mpiw_handle_storage sendtype, void *recvbuf,
                     std::int32_t recvcount,
                     plumed_mpiw_handle_storage recvtype, std::int32_t root,
                     plumed_mpiw_handle_storage comm) {
  return thread_mpi::tMPI_Scatter(
           sendbuf, sendcount, decode_datatype(sendtype), recvbuf, recvcount,
           decode_datatype(recvtype), root, decode_comm(comm));
}

std::int32_t gather(const void *sendbuf, std::int32_t sendcount,
                    plumed_mpiw_handle_storage sendtype, void *recvbuf,
                    std::int32_t recvcount,
                    plumed_mpiw_handle_storage recvtype, std::int32_t root,
                    plumed_mpiw_handle_storage comm) {
  return thread_mpi::tMPI_Gather(
           sendbuf, sendcount, decode_datatype(sendtype), recvbuf, recvcount,
           decode_datatype(recvtype), root, decode_comm(comm));
}

bool checked_extent(std::int32_t count, std::size_t element_size,
                    std::size_t *bytes) {
  if (count < 0 || !bytes || (element_size != 0 &&
                              static_cast<std::size_t>(count) >
                              static_cast<std::size_t>(-1) / element_size)) {
    return false;
  }
  *bytes = static_cast<std::size_t>(count) * element_size;
  return true;
}

std::int32_t allgather(const void *sendbuf, std::int32_t sendcount,
                       plumed_mpiw_handle_storage sendtype, void *recvbuf,
                       std::int32_t recvcount,
                       plumed_mpiw_handle_storage recvtype,
                       plumed_mpiw_handle_storage comm) {
  const auto native_sendtype = decode_datatype(sendtype);
  const auto native_recvtype = decode_datatype(recvtype);
  if (sendbuf != nullptr &&
      (sendcount != recvcount || native_sendtype != native_recvtype)) {
    return thread_mpi::TMPI_ERR_TYPE;
  }
  std::size_t bytes = 0;
  const std::size_t element_size = builtin_type_size(native_recvtype);
  if (element_size == 0 || !checked_extent(recvcount, element_size, &bytes)) {
    return thread_mpi::TMPI_ERR_TYPE;
  }
  int rank = 0;
  int size = 0;
  const auto native_comm = decode_comm(comm);
  int result = thread_mpi::tMPI_Comm_rank(native_comm, &rank);
  if (result != thread_mpi::TMPI_SUCCESS) {
    return result;
  }
  result = thread_mpi::tMPI_Comm_size(native_comm, &size);
  if (result != thread_mpi::TMPI_SUCCESS) {
    return result;
  }
  if (bytes != 0 && !recvbuf) {
    return thread_mpi::TMPI_ERR_BUF;
  }
  unsigned char empty_segment = 0;
  for (int root = 0; root < size; ++root) {
    if (recvcount != 0 && static_cast<std::size_t>(root) >
        static_cast<std::size_t>(-1) / bytes) {
      return thread_mpi::TMPI_ERR_BUF;
    }
    auto *segment = recvbuf ? static_cast<unsigned char *>(recvbuf) +
                    static_cast<std::size_t>(root) * bytes :
                    &empty_segment;
    if (root == rank && sendbuf != nullptr && bytes != 0) {
      std::memcpy(segment, sendbuf, bytes);
    }
    result = thread_mpi::tMPI_Bcast(segment, recvcount, native_recvtype,
                                    root, native_comm);
    if (result != thread_mpi::TMPI_SUCCESS) {
      return result;
    }
  }
  return thread_mpi::TMPI_SUCCESS;
}

std::int32_t allgatherv(const void *sendbuf, std::int32_t sendcount,
                        plumed_mpiw_handle_storage sendtype, void *recvbuf,
                        const std::int32_t recvcounts[],
                        const std::int32_t displacements[],
                        plumed_mpiw_handle_storage recvtype,
                        plumed_mpiw_handle_storage comm) {
  const auto native_sendtype = decode_datatype(sendtype);
  const auto native_recvtype = decode_datatype(recvtype);
  if ((sendbuf != nullptr && native_sendtype != native_recvtype) ||
      !recvcounts || !displacements) {
    return thread_mpi::TMPI_ERR_TYPE;
  }
  const std::size_t element_size = builtin_type_size(native_recvtype);
  if (element_size == 0) {
    return thread_mpi::TMPI_ERR_TYPE;
  }
  int rank = 0;
  int size = 0;
  const auto native_comm = decode_comm(comm);
  int result = thread_mpi::tMPI_Comm_rank(native_comm, &rank);
  if (result != thread_mpi::TMPI_SUCCESS) {
    return result;
  }
  result = thread_mpi::tMPI_Comm_size(native_comm, &size);
  if (result != thread_mpi::TMPI_SUCCESS) {
    return result;
  }
  if (sendbuf != nullptr && sendcount != recvcounts[rank]) {
    return thread_mpi::TMPI_ERR_TYPE;
  }
  if (sendbuf != nullptr && !recvbuf && sendcount != 0) {
    return thread_mpi::TMPI_ERR_BUF;
  }
  unsigned char empty_segment = 0;
  for (int root = 0; root < size; ++root) {
    std::size_t bytes = 0;
    if (displacements[root] < 0 ||
        !checked_extent(recvcounts[root], element_size, &bytes) ||
        static_cast<std::size_t>(displacements[root]) >
        static_cast<std::size_t>(-1) / element_size) {
      return thread_mpi::TMPI_ERR_TYPE;
    }
    if (bytes != 0 && !recvbuf) {
      return thread_mpi::TMPI_ERR_BUF;
    }
    auto *segment = recvbuf ? static_cast<unsigned char *>(recvbuf) +
                    static_cast<std::size_t>(displacements[root]) * element_size :
                    &empty_segment;
    if (root == rank && sendbuf != nullptr && bytes != 0) {
      std::memcpy(segment, sendbuf, bytes);
    }
    result = thread_mpi::tMPI_Bcast(segment, recvcounts[root], native_recvtype,
                                    root, native_comm);
    if (result != thread_mpi::TMPI_SUCCESS) {
      return result;
    }
  }
  return thread_mpi::TMPI_SUCCESS;
}

std::int32_t irecv(void *buffer, std::int32_t count,
                   plumed_mpiw_handle_storage datatype, std::int32_t source,
                   std::int32_t tag, plumed_mpiw_handle_storage comm,
                   plumed_mpiw_handle_storage *request) {
  if (!request) {
    return thread_mpi::TMPI_ERR_REQUESTS;
  }
  thread_mpi::tMPI_Request native_request{};
  const int result = thread_mpi::tMPI_Irecv(
                       buffer, count, decode_datatype(datatype), source, tag,
                       decode_comm(comm), &native_request);
  if (result == thread_mpi::TMPI_SUCCESS) {
    *request = encode_request(native_request);
  }
  return result;
}

std::int32_t isend(const void *buffer, std::int32_t count,
                   plumed_mpiw_handle_storage datatype, std::int32_t dest,
                   std::int32_t tag, plumed_mpiw_handle_storage comm,
                   plumed_mpiw_handle_storage *request) {
  if (!request) {
    return thread_mpi::TMPI_ERR_REQUESTS;
  }
  thread_mpi::tMPI_Request native_request{};
  const int result = thread_mpi::tMPI_Isend(
                       buffer, count, decode_datatype(datatype), dest, tag,
                       decode_comm(comm), &native_request);
  if (result == thread_mpi::TMPI_SUCCESS) {
    *request = encode_request(native_request);
  }
  return result;
}

std::int32_t wait(plumed_mpiw_handle_storage *request,
                  plumed_mpiw_status *status) {
  if (!request) {
    return thread_mpi::TMPI_ERR_REQUESTS;
  }
  thread_mpi::tMPI_Request native_request = decode_request(*request);
  thread_mpi::tMPI_Status native_status{};
  const int result = thread_mpi::tMPI_Wait(
                       &native_request, status ? &native_status : nullptr);
  *request = encode_request(native_request);
  if (result == thread_mpi::TMPI_SUCCESS && status) {
    export_status(native_status, status);
  }
  return result;
}

std::int32_t waitall(std::int32_t count,
                     plumed_mpiw_handle_storage array_of_requests[],
                     plumed_mpiw_status array_of_statuses[]) {
  if (count < 0 || (count > 0 && !array_of_requests)) {
    return thread_mpi::TMPI_ERR_REQUESTS;
  }
  try {
    std::vector<thread_mpi::tMPI_Request> requests(static_cast<std::size_t>(count));
    std::vector<thread_mpi::tMPI_Status> statuses;
    if (array_of_statuses) {
      statuses.resize(static_cast<std::size_t>(count));
    }
    for (int i = 0; i < count; ++i) {
      requests[static_cast<std::size_t>(i)] = decode_request(array_of_requests[i]);
    }
    const int result = thread_mpi::tMPI_Waitall(
                         count, count ? requests.data() : nullptr,
                         array_of_statuses && count ? statuses.data() : nullptr);
    for (int i = 0; i < count; ++i) {
      array_of_requests[i] = encode_request(requests[static_cast<std::size_t>(i)]);
    }
    if (array_of_statuses &&
        (result == thread_mpi::TMPI_SUCCESS || result == thread_mpi::TMPI_ERR_IN_STATUS)) {
      for (int i = 0; i < count; ++i) {
        export_status(statuses[static_cast<std::size_t>(i)], &array_of_statuses[i]);
      }
    }
    return result;
  } catch (const std::bad_alloc &) {
    return thread_mpi::TMPI_ERR_NO_MEM;
  }
}

std::int32_t waitsome(std::int32_t incount,
                      plumed_mpiw_handle_storage array_of_requests[],
                      std::int32_t *outcount, std::int32_t array_of_indices[],
                      plumed_mpiw_status array_of_statuses[]) {
  if (incount < 0 || (incount > 0 && !array_of_requests) || !outcount ||
      (incount > 0 && !array_of_indices)) {
    return thread_mpi::TMPI_ERR_REQUESTS;
  }
  try {
    std::vector<thread_mpi::tMPI_Request> requests(static_cast<std::size_t>(incount));
    std::vector<thread_mpi::tMPI_Status> statuses;
    if (array_of_statuses) {
      statuses.resize(static_cast<std::size_t>(incount));
    }
    for (int i = 0; i < incount; ++i) {
      requests[static_cast<std::size_t>(i)] = decode_request(array_of_requests[i]);
    }
    int completed = 0;
    int result = thread_mpi::TMPI_SUCCESS;
    if (incount > 0) {
      int index = TMPI_UNDEFINED;
      thread_mpi::tMPI_Status status{};
      result = thread_mpi::tMPI_Waitany(incount, requests.data(), &index, &status);
      if (index != TMPI_UNDEFINED) {
        array_of_indices[completed] = index;
        if (array_of_statuses) {
          statuses[static_cast<std::size_t>(completed)] = status;
        }
        ++completed;
        for (;;) {
          int next_index = TMPI_UNDEFINED;
          int flag = 0;
          thread_mpi::tMPI_Status next_status{};
          const int next_result = thread_mpi::tMPI_Testany(
                                    incount, requests.data(), &next_index, &flag,
                                    &next_status);
          if (next_result != thread_mpi::TMPI_SUCCESS) {
            result = next_result;
          }
          if (!flag || next_index == TMPI_UNDEFINED) {
            break;
          }
          array_of_indices[completed] = next_index;
          if (array_of_statuses) {
            statuses[static_cast<std::size_t>(completed)] = next_status;
          }
          ++completed;
        }
      }
    }
    *outcount = completed ? completed : TMPI_UNDEFINED;
    for (int i = 0; i < incount; ++i) {
      array_of_requests[i] = encode_request(requests[static_cast<std::size_t>(i)]);
    }
    if (array_of_statuses && completed > 0) {
      for (int i = 0; i < completed; ++i) {
        export_status(statuses[static_cast<std::size_t>(i)], &array_of_statuses[i]);
      }
    }
    return result;
  } catch (const std::bad_alloc &) {
    return thread_mpi::TMPI_ERR_NO_MEM;
  }
}

std::int32_t testsome(std::int32_t incount,
                      plumed_mpiw_handle_storage array_of_requests[],
                      std::int32_t *outcount, std::int32_t array_of_indices[],
                      plumed_mpiw_status array_of_statuses[]) {
  if (incount < 0 || (incount > 0 && !array_of_requests) || !outcount ||
      (incount > 0 && !array_of_indices)) {
    return thread_mpi::TMPI_ERR_REQUESTS;
  }
  try {
    std::vector<thread_mpi::tMPI_Request> requests(static_cast<std::size_t>(incount));
    std::vector<thread_mpi::tMPI_Status> statuses;
    if (array_of_statuses) {
      statuses.resize(static_cast<std::size_t>(incount));
    }
    bool any_active = false;
    for (int i = 0; i < incount; ++i) {
      requests[static_cast<std::size_t>(i)] = decode_request(array_of_requests[i]);
      any_active = any_active || requests[static_cast<std::size_t>(i)] != nullptr;
    }
    int completed = 0;
    int result = thread_mpi::TMPI_SUCCESS;
    if (any_active) {
      for (;;) {
        int index = TMPI_UNDEFINED;
        int flag = 0;
        thread_mpi::tMPI_Status status{};
        const int next_result = thread_mpi::tMPI_Testany(
                                  incount, requests.data(), &index, &flag, &status);
        if (next_result != thread_mpi::TMPI_SUCCESS) {
          result = next_result;
        }
        if (!flag || index == TMPI_UNDEFINED) {
          break;
        }
        array_of_indices[completed] = index;
        if (array_of_statuses) {
          statuses[static_cast<std::size_t>(completed)] = status;
        }
        ++completed;
      }
    }
    *outcount = any_active ? completed : TMPI_UNDEFINED;
    for (int i = 0; i < incount; ++i) {
      array_of_requests[i] = encode_request(requests[static_cast<std::size_t>(i)]);
    }
    if (array_of_statuses && completed > 0) {
      for (int i = 0; i < completed; ++i) {
        export_status(statuses[static_cast<std::size_t>(i)], &array_of_statuses[i]);
      }
    }
    return result;
  } catch (const std::bad_alloc &) {
    return thread_mpi::TMPI_ERR_NO_MEM;
  }
}

std::int32_t get_count(const plumed_mpiw_status *status,
                       plumed_mpiw_handle_storage datatype,
                       std::int32_t *count) {
  thread_mpi::tMPI_Status native_status{};
  if (!import_status(status, &native_status)) {
    return thread_mpi::TMPI_ERR_STATUS;
  }
  return thread_mpi::tMPI_Get_count(&native_status, decode_datatype(datatype), count);
}

std::int32_t sendrecv(const void *sendbuf, std::int32_t sendcount,
                      plumed_mpiw_handle_storage sendtype, std::int32_t dest,
                      std::int32_t sendtag, void *recvbuf,
                      std::int32_t recvcount,
                      plumed_mpiw_handle_storage recvtype, std::int32_t source,
                      std::int32_t recvtag, plumed_mpiw_handle_storage comm,
                      plumed_mpiw_status *status) {
  thread_mpi::tMPI_Status native_status{};
  const int result = thread_mpi::tMPI_Sendrecv(
                       sendbuf, sendcount, decode_datatype(sendtype), dest,
                       sendtag, recvbuf, recvcount, decode_datatype(recvtype),
                       source, recvtag, decode_comm(comm),
                       status ? &native_status : nullptr);
  if (result == thread_mpi::TMPI_SUCCESS && status) {
    export_status(native_status, status);
  }
  return result;
}

int32_t get_self(void *output, uint32_t capacity) {
  if (!output || capacity != sizeof(plumed_mpiw_handle_storage)) {
    return -1;
  }
  plumed_mpiw_handle_storage storage = 0;
  int initialized = 0;
  if (thread_mpi::tMPI_Initialized(&initialized) != thread_mpi::TMPI_SUCCESS ||
      !initialized) {
    std::memcpy(output, &storage, sizeof(storage));
    return 0;
  }
  const thread_mpi::tMPI_Comm value = thread_mpi::tMPI_Get_comm_self();
  std::memcpy(&storage, &value, sizeof(value));
  std::memcpy(output, &storage, sizeof(storage));
  return 0;
}

template<typename Handle>
int32_t exportHandle(void *output, uint32_t capacity, Handle value) {
  static_assert(std::is_trivially_copyable<Handle>::value,
                "tMPI constant handle must be byte-copyable");
  if (!output || capacity != sizeof(plumed_mpiw_handle_storage) ||
      sizeof(Handle) > sizeof(plumed_mpiw_handle_storage)) {
    return -1;
  }
  plumed_mpiw_handle_storage storage = 0;
  if constexpr (!std::is_same<Handle, std::nullptr_t>::value) {
    std::memcpy(&storage, &value, sizeof(value));
  }
  std::memcpy(output, &storage, sizeof(storage));
  return 0;
}

template<typename Integer>
int32_t exportInteger(void *output, uint32_t capacity, Integer value) {
  static_assert(std::is_integral<Integer>::value || std::is_enum<Integer>::value,
                "tMPI integer constant must be integral or enum");
  if (!output || capacity != sizeof(int32_t) || sizeof(Integer) != sizeof(int32_t)) {
    return -1;
  }
  const int32_t storage = static_cast<int32_t>(value);
  std::memcpy(output, &storage, sizeof(storage));
  return 0;
}

template<typename Pointer>
int32_t exportPointer(void *output, uint32_t capacity, Pointer value) {
  if (!output || capacity != sizeof(void *) || sizeof(Pointer) != sizeof(void *)) {
    return -1;
  }
  void *storage = nullptr;
  if constexpr (std::is_pointer<Pointer>::value) {
    storage = value;
  } else if constexpr (std::is_same<Pointer, std::nullptr_t>::value) {
    storage = nullptr;
  } else if constexpr (std::is_integral<Pointer>::value) {
    if (value != 0) {
      return -1;
    }
  } else {
    static_assert(std::is_pointer<Pointer>::value,
                  "unsupported tMPI sentinel representation");
  }
  std::memcpy(output, &storage, sizeof(storage));
  return 0;
}

using namespace thread_mpi;
#include "thread-mpi-constant-accessors.inc"

static const char callback_unavailable[] = PLUMED_MPIW_CALLBACK_ABI_UNAVAILABLE;
static const plumed_mpiw_function_entry functions[] = {
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Comm_rank", 0x4e7672aa,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&comm_rank)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Comm_size", 0xda2369ec,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&comm_size)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Bcast", 0xe809e1ca,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&bcast)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Allreduce", 0x788900db,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&allreduce)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Barrier", 0xe7a052f4,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&barrier)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Irecv", 0xb69a0083,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&irecv)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Isend", 0xf5e9d915,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&isend)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Wait", 0x4b15813d,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&wait)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Waitall", 0x7a57ae23,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&waitall)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Waitsome", 0xb6562e1a,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&waitsome)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Testsome", 0x5a87d17b,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&testsome)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Get_count", 0x71967a66,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&get_count)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Sendrecv", 0xb09a6fc1,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&sendrecv)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Initialized", 0xd4498369,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&initialized)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Comm_dup", 0xf0dc6608,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&comm_dup)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Comm_free", 0x4da4e1b2,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&comm_free)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Comm_split", 0x97f1adf0,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&comm_split)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Abort", 0x84f458d6,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&abort_comm)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Recv", 0xdd56a4bc,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&recv)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Scatter", 0xb2fa0f58,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&scatter)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Gather", 0xb1f903d9,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&gather)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Allgather", 0xab8b26c9,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&allgather)
  },
  {
    sizeof(plumed_mpiw_function_entry), 0, "MPI_Allgatherv", 0x0f1b22f3,
    1, reinterpret_cast<plumed_mpiw_generic_fn>(&allgatherv)
  },
};
static const plumed_mpiw_metadata_entry metadata[] = {
  {
    sizeof(plumed_mpiw_metadata_entry), PLUMED_MPIW_VALUE_UTF8,
    PLUMED_MPIW_METADATA_REQUIRED, 0,
    PLUMED_MPIW_METADATA_MPI_USER_FUNCTION_ABI,
    sizeof(callback_unavailable), callback_unavailable
  },
#include "thread-mpi-constant-metadata.inc"
  {
    sizeof(plumed_mpiw_metadata_entry), PLUMED_MPIW_VALUE_UTF8,
    PLUMED_MPIW_METADATA_REQUIRED, 0,
    PLUMED_MPIW_METADATA_MPI_ERRHANDLER_FUNCTION_ABI,
    sizeof(callback_unavailable), callback_unavailable
  },
};

static const plumed_mpiw_provider_descriptor descriptor = {
  sizeof(plumed_mpiw_provider_descriptor), 1, 0,
  PLUMED_MPIW_RUNTIME_THREAD_MPI,
  0, {0, 0, 0, 0},
  sizeof(thread_mpi::tMPI_Comm), sizeof(thread_mpi::tMPI_Group),
  sizeof(thread_mpi::tMPI_Datatype), sizeof(thread_mpi::tMPI_Request),
  sizeof(thread_mpi::tMPI_Op), sizeof(thread_mpi::tMPI_Errhandler),
  sizeof(thread_mpi::tMPI_Status), alignof(thread_mpi::tMPI_Status),
  sizeof(int), 1, 0, 0,
  sizeof(plumed_mpiw_handle_storage), PLUMED_MPIW_STATUS_CAPACITY,
  {0, 0, 0, 0, 0, 0},
  "PLUMED embedded thread-MPI", "GROMACS v2026.4",
  sizeof(functions) / sizeof(functions[0]), functions,
  sizeof(metadata) / sizeof(metadata[0]), metadata,
  reinterpret_cast<plumed_mpiw_generic_fn>(&thread_mpi::tMPI_Comm_rank),
  reinterpret_cast<plumed_mpiw_generic_fn>(&thread_mpi::tMPI_Initialized),
  &thread_mpi::TMPI_COMM_WORLD
};

} // namespace thread_mpi_adapter

extern "C" const plumed_mpiw_provider_descriptor *
plumed_mpiw_thread_mpi_provider(void) {
  return &thread_mpi_adapter::descriptor;
}
} // namespace PLMD
