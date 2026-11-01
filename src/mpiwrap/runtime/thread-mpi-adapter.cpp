// Stable tMPI provider entries for the first runtime communication slice.
#include "../generated/provider-abi.h"
#include "../generated/provider-callbacks.h"

#include <thread_mpi/tmpi.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

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
};
static const plumed_mpiw_metadata_entry metadata[] = {
  {
    sizeof(plumed_mpiw_metadata_entry), PLUMED_MPIW_VALUE_UTF8,
    PLUMED_MPIW_METADATA_REQUIRED, 0,
    PLUMED_MPIW_METADATA_MPI_USER_FUNCTION_ABI,
    sizeof(callback_unavailable), callback_unavailable
  },
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
