#include <thread_mpi/tmpi.h>

#include "../../mpiwrap/generated/mpiwrap.h"
#include "../../mpiwrap/runtime/provider-registry.hpp"
#include "../../mpiwrap/generated/provider-abi.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>

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
  int rank = -1;
  int size = 0;
  if (mpiwrap_Comm_rank(world, &rank) != thread_mpi::TMPI_SUCCESS ||
      mpiwrap_Comm_size(world, &size) != thread_mpi::TMPI_SUCCESS ||
      size < 1 || rank < 0 || rank >= size) {
    failures.fetch_add(1);
    return;
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

  int broadcast_value = rank == 0 ? 91 : -1;
  if (mpiwrap_Bcast(&broadcast_value, 1, integer, 0, world) !=
      thread_mpi::TMPI_SUCCESS || broadcast_value != 91 ||
      mpiwrap_Barrier(world) != thread_mpi::TMPI_SUCCESS) {
    failures.fetch_add(1);
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
      !mpiwrap_has_Get_count() || !mpiwrap_has_Sendrecv()) {
    std::cerr << "required tMPI provider entries missing\n";
    return 1;
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
