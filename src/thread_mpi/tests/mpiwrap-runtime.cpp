#include <thread_mpi/tmpi.h>

#include "../../mpiwrap/generated/mpiwrap.h"
#include "../../mpiwrap/runtime/provider-registry.hpp"
#include "../../mpiwrap/generated/provider-abi.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" const plumed_mpiw_provider_descriptor *
plumed_mpiw_thread_mpi_provider(void);

namespace PLMD {
namespace mpiwrap_runtime_test {

std::atomic<int> failures{0};
std::atomic<int> ranks_seen{0};

void rank_work(const void *) {
  const MPI_Comm world = mpiwrap_COMM_WORLD();
  const MPI_Datatype integer = mpiwrap_INT();
  const MPI_Op sum = mpiwrap_SUM();
  const MPI_Datatype float_type = mpiwrap_FLOAT();
  const MPI_Datatype double_type = mpiwrap_DOUBLE();
  const MPI_Datatype char_type = mpiwrap_CHAR();
  const MPI_Datatype unsigned_type = mpiwrap_UNSIGNED();
  const MPI_Datatype unsigned_long_type = mpiwrap_UNSIGNED_LONG();
  const MPI_Datatype unsigned_long_long_type = mpiwrap_UNSIGNED_LONG_LONG();
  const MPI_Datatype long_double_type = mpiwrap_LONG_DOUBLE();
  const MPI_Comm null_comm = mpiwrap_COMM_NULL();
  const MPI_Status *ignore_status = mpiwrap_STATUS_IGNORE();
  const MPI_Op product = mpiwrap_PROD();
  const MPI_Op maximum = mpiwrap_MAX();
  const MPI_Op minimum = mpiwrap_MIN();
  int rank = -1;
  int size = 0;
  if (mpiwrap_Comm_rank(world, &rank) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Comm_size(world, &size) != thread_mpi::TMPI_SUCCESS ||
      size < 1 || rank < 0 || rank >= size) {
    failures.fetch_add(1);
    return;
  }
  if (!float_type.storage || !double_type.storage || !char_type.storage ||
      !unsigned_type.storage || !unsigned_long_type.storage ||
      !unsigned_long_long_type.storage || !long_double_type.storage ||
      null_comm.storage != 0 || ignore_status != nullptr || product == sum ||
      maximum == minimum || mpiwrap_ANY_SOURCE() != -1 ||
      mpiwrap_ANY_TAG() != -1) {
    std::cerr << "rank " << rank << " provider constants failed: null="
              << null_comm.storage << " status-ignore=" << ignore_status
              << " type-handles=" << float_type.storage << ',' << double_type.storage
              << ',' << char_type.storage << ',' << unsigned_type.storage << ','
              << unsigned_long_type.storage << ',' << unsigned_long_long_type.storage
              << ',' << long_double_type.storage << " op-handles=" << product.storage
              << ',' << maximum.storage << ',' << minimum.storage
              << " any-source=" << mpiwrap_ANY_SOURCE()
              << " any-tag=" << mpiwrap_ANY_TAG() << '\n';
    failures.fetch_add(1);
  }
  int initialized = 0;
  if (mpiwrap_Initialized(&initialized) != thread_mpi::TMPI_SUCCESS || !initialized) {
    failures.fetch_add(1);
  }

  int input = rank + 1;
  int output = -123;
  if (!mpiwrap_has_Allreduce() ||
      mpiwrap_Allreduce(&input, &output, 1, integer, sum, world) !=
      thread_mpi::TMPI_SUCCESS || output != size * (size + 1) / 2) {
    failures.fetch_add(1);
  }

  int in_place = rank + 1;
  if (mpiwrap_Allreduce(MPI_IN_PLACE, &in_place, 1, integer, sum, world) !=
      thread_mpi::TMPI_SUCCESS || in_place != size * (size + 1) / 2) {
    failures.fetch_add(1);
  }

  auto check_sum = [&](auto value, MPI_Datatype datatype, const char *label) {
    using Value = decltype(value);
    Value reduced{};
    if (mpiwrap_Allreduce(&value, &reduced, 1, datatype, sum, world) !=
        thread_mpi::TMPI_SUCCESS || reduced != static_cast<Value>(size * (size + 1) / 2)) {
      std::cerr << "rank " << rank << " reduction failed: " << label
                << " result=" << reduced << '\n';
      failures.fetch_add(1);
    }
  };
  check_sum(static_cast<float>(rank + 1), float_type, "float");
  check_sum(static_cast<double>(rank + 1), double_type, "double");
  check_sum(static_cast<unsigned>(rank + 1), unsigned_type, "unsigned");
  check_sum(static_cast<unsigned long>(rank + 1), unsigned_long_type, "unsigned long");
  check_sum(static_cast<unsigned long long>(rank + 1), unsigned_long_long_type, "unsigned long long");
  check_sum(static_cast<long double>(rank + 1), long_double_type, "long double");

  int broadcast_value = rank == 0 ? 91 : -1;
  if (mpiwrap_Bcast(&broadcast_value, 1, integer, 0, world) !=
      thread_mpi::TMPI_SUCCESS || broadcast_value != 91 ||
      mpiwrap_Barrier(world) != thread_mpi::TMPI_SUCCESS) {
    failures.fetch_add(1);
  }

  MPI_Comm duplicate{};
  int duplicate_rank = -1;
  int duplicate_size = 0;
  if (mpiwrap_Comm_dup(world, &duplicate) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Comm_rank(duplicate, &duplicate_rank) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Comm_size(duplicate, &duplicate_size) != thread_mpi::TMPI_SUCCESS ||
      duplicate_rank != rank || duplicate_size != size ||
      mpiwrap_Comm_free(&duplicate) != thread_mpi::TMPI_SUCCESS) {
    failures.fetch_add(1);
  }

  MPI_Comm split{};
  int split_rank = -1;
  int split_size = 0;
  if (mpiwrap_Comm_split(world, rank % 2, rank, &split) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Comm_rank(split, &split_rank) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Comm_size(split, &split_size) != thread_mpi::TMPI_SUCCESS ||
      split_size != (size + 1 - (rank % 2)) / 2 || split_rank < 0 ||
      mpiwrap_Comm_free(&split) != thread_mpi::TMPI_SUCCESS) {
    failures.fetch_add(1);
  }

  int gathered_value = rank + 11;
  std::vector<int> gathered(static_cast<std::size_t>(size), -1);
  if (mpiwrap_Gather(&gathered_value, 1, integer,
                     rank == 0 ? gathered.data() : nullptr, 1, integer, 0,
                     world) != thread_mpi::TMPI_SUCCESS) {
    failures.fetch_add(1);
  } else if (rank == 0) {
    for (int i = 0; i < size; ++i) {
      if (gathered[static_cast<std::size_t>(i)] != i + 11) {
        failures.fetch_add(1);
      }
    }
  }

  std::vector<int> scattered(static_cast<std::size_t>(size));
  for (int i = 0; i < size; ++i) {
    scattered[static_cast<std::size_t>(i)] = i + 101;
  }
  int scatter_result = -1;
  if (mpiwrap_Scatter(rank == 0 ? scattered.data() : nullptr, 1, integer,
                      &scatter_result, 1, integer, 0, world) !=
      thread_mpi::TMPI_SUCCESS || scatter_result != rank + 101) {
    failures.fetch_add(1);
  }

  int allgather_input = rank + 21;
  std::vector<int> allgather_output(static_cast<std::size_t>(size), -1);
  if (mpiwrap_Allgather(&allgather_input, 1, integer, allgather_output.data(),
                        1, integer, world) != thread_mpi::TMPI_SUCCESS) {
    failures.fetch_add(1);
  } else {
    for (int i = 0; i < size; ++i) {
      if (allgather_output[static_cast<std::size_t>(i)] != i + 21) {
        failures.fetch_add(1);
      }
    }
  }

  std::vector<int> recvcounts(static_cast<std::size_t>(size));
  std::vector<int> displacements(static_cast<std::size_t>(size));
  int total_variable = size + size - 1;
  for (int i = 0; i < size; ++i) {
    recvcounts[static_cast<std::size_t>(i)] = i + 1;
    displacements[static_cast<std::size_t>(i)] = i * (i + 1) / 2 + i;
    total_variable += i;
  }
  std::vector<int> variable_input(static_cast<std::size_t>(rank + 1), rank + 31);
  std::vector<int> variable_output(static_cast<std::size_t>(total_variable), -99);
  if (mpiwrap_Allgatherv(variable_input.data(), rank + 1, integer,
                         variable_output.data(), recvcounts.data(),
                         displacements.data(), integer, world) !=
      thread_mpi::TMPI_SUCCESS) {
    failures.fetch_add(1);
  } else {
    for (int i = 0; i < size; ++i) {
      for (int j = 0; j < i + 1; ++j) {
        if (variable_output[static_cast<std::size_t>(displacements[static_cast<std::size_t>(i)] + j)] != i + 31) {
          failures.fetch_add(1);
        }
      }
      const int next_displacement = i + 1 < size ?
                                    displacements[static_cast<std::size_t>(i + 1)] :
                                    total_variable;
      for (int gap = displacements[static_cast<std::size_t>(i)] + i + 1;
           gap < next_displacement; ++gap) {
        if (variable_output[static_cast<std::size_t>(gap)] != -99) {
          failures.fetch_add(1);
        }
      }
    }
  }

  std::vector<int> inplace_output(static_cast<std::size_t>(total_variable), -99);
  for (int i = 0; i < rank + 1; ++i) {
    inplace_output[static_cast<std::size_t>(displacements[static_cast<std::size_t>(rank)] + i)] = rank + 51;
  }
  if (mpiwrap_Allgatherv(MPI_IN_PLACE, -7, integer, inplace_output.data(),
                         recvcounts.data(), displacements.data(), integer,
                         world) != thread_mpi::TMPI_SUCCESS) {
    failures.fetch_add(1);
  } else {
    for (int i = 0; i < size; ++i) {
      for (int j = 0; j < i + 1; ++j) {
        if (inplace_output[static_cast<std::size_t>(displacements[static_cast<std::size_t>(i)] + j)] != i + 51) {
          failures.fetch_add(1);
        }
      }
      const int next_displacement = i + 1 < size ?
                                    displacements[static_cast<std::size_t>(i + 1)] :
                                    total_variable;
      for (int gap = displacements[static_cast<std::size_t>(i)] + i + 1;
           gap < next_displacement; ++gap) {
        if (inplace_output[static_cast<std::size_t>(gap)] != -99) {
          failures.fetch_add(1);
        }
      }
    }
  }

  const int next = (rank + 1) % size;
  const int previous = (rank + size - 1) % size;
  int ring_value = -1;
  MPI_Request receive_request{};
  MPI_Request send_request{};
  MPI_Status receive_status{};
  if (!mpiwrap_has_Irecv() || !mpiwrap_has_Isend() || !mpiwrap_has_Wait() ||
      !mpiwrap_has_Get_count() ||
      mpiwrap_Irecv(&ring_value, 1, integer, previous, 73, world,
                    &receive_request) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Isend(&input, 1, integer, next, 73, world,
                    &send_request) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Wait(&receive_request, &receive_status) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Wait(&send_request, MPI_STATUS_IGNORE) != thread_mpi::TMPI_SUCCESS) {
    failures.fetch_add(1);
  } else {
    int received_count = -1;
    if (ring_value != previous + 1 || receive_status.MPI_SOURCE != previous ||
        receive_status.MPI_TAG != 73 ||
        mpiwrap_Get_count(&receive_status, integer, &received_count) !=
        thread_mpi::TMPI_SUCCESS || received_count != 1) {
      failures.fetch_add(1);
    }
  }

  int sendrecv_value = input;
  int sendrecv_result = -1;
  MPI_Status sendrecv_status{};
  if (mpiwrap_Sendrecv(&sendrecv_value, 1, integer, next, 74,
                       &sendrecv_result, 1, integer, previous, 74, world,
                       &sendrecv_status) != thread_mpi::TMPI_SUCCESS ||
      sendrecv_result != previous + 1 || sendrecv_status.MPI_SOURCE != previous ||
      sendrecv_status.MPI_TAG != 74) {
    failures.fetch_add(1);
  }

  int blocking_input = rank + 61;
  int blocking_output = -1;
  MPI_Request blocking_send{};
  MPI_Status blocking_status{};
  if (mpiwrap_Isend(&blocking_input, 1, integer, next, 75, world,
                    &blocking_send) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Recv(&blocking_output, 1, integer, previous, 75, world,
                   &blocking_status) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Wait(&blocking_send, MPI_STATUS_IGNORE) != thread_mpi::TMPI_SUCCESS ||
      blocking_output != previous + 61 || blocking_status.MPI_SOURCE != previous ||
      blocking_status.MPI_TAG != 75) {
    failures.fetch_add(1);
  }
  ranks_seen.fetch_add(1);
}

int run(int ranks) {
  std::string error;
  auto &registry = mpiwrap_provider::currentRegistry();
  if (!registry.install(plumed_mpiw_thread_mpi_provider(), error)) {
    std::cerr << "provider install failed: " << error << '\n';
    return 1;
  }
  if (!mpiwrap_has_Comm_rank() || !mpiwrap_has_Comm_size() ||
      !mpiwrap_has_Bcast() || !mpiwrap_has_Allreduce() ||
      !mpiwrap_has_Barrier() || !mpiwrap_has_Irecv() ||
      !mpiwrap_has_Isend() || !mpiwrap_has_Wait() ||
      !mpiwrap_has_Get_count() || !mpiwrap_has_Sendrecv() ||
      !mpiwrap_has_Initialized() || !mpiwrap_has_Comm_dup() ||
      !mpiwrap_has_Comm_free() || !mpiwrap_has_Comm_split() ||
      !mpiwrap_has_Abort() || !mpiwrap_has_Recv() || !mpiwrap_has_Scatter() ||
      !mpiwrap_has_Gather() || !mpiwrap_has_Allgather() ||
      !mpiwrap_has_Allgatherv()) {
    std::cerr << "required tMPI provider entries missing\n";
    return 1;
  }
  if (mpiwrap_has_Init()) {
    std::cerr << "unimplemented tMPI call reported as available\n";
    return 1;
  }
  try {
    (void)mpiwrap_Init(nullptr, nullptr);
    std::cerr << "unimplemented tMPI call did not throw\n";
    return 1;
  } catch (const std::runtime_error &error) {
    if (std::string(error.what()).find("MPI_Init") == std::string::npos ||
        std::string(error.what()).find("PLUMED embedded thread-MPI") ==
        std::string::npos) {
      std::cerr << "unavailable diagnostic omitted function/provider identity: "
                << error.what() << '\n';
      return 1;
    }
  }
  if (thread_mpi::tMPI_Init_fn(0, ranks, thread_mpi::TMPI_AFFINITY_NONE,
                               rank_work, nullptr) != thread_mpi::TMPI_SUCCESS) {
    return 1;
  }
  int finalized = 0;
  if (thread_mpi::tMPI_Finalized(&finalized) != thread_mpi::TMPI_SUCCESS ||
      !finalized || ranks_seen.load() != ranks || failures.load() != 0) {
    std::cerr << "MPI wrapper rank checks failed: seen=" << ranks_seen.load()
              << " expected=" << ranks << " failures=" << failures.load()
              << " finalized=" << finalized << '\n';
    return 1;
  }
  return 0;
}

} // namespace mpiwrap_runtime_test
} // namespace PLMD

int main(int argc, char **argv) {
  if (argc != 2) {
    return 2;
  }
  const int ranks = std::atoi(argv[1]);
  if (ranks < 1 || ranks > 4) {
    return 2;
  }
  return PLMD::mpiwrap_runtime_test::run(ranks);
}
