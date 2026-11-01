#include <thread_mpi/tmpi.h>

#include "../../mpiwrap/generated/mpiwrap.h"
#include "../../mpiwrap/runtime/provider-registry.hpp"
#include "../../mpiwrap/generated/provider-abi.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>

extern "C" const plumed_mpiw_provider_descriptor *
plumed_mpiw_thread_mpi_provider(void);

namespace PLMD {
namespace {

plumed_mpiw_handle_storage pointerStorage(const void *pointer) {
  plumed_mpiw_handle_storage storage = 0;
  static_assert(sizeof(pointer) <= sizeof(storage), "pointer exceeds wrapper storage");
  std::memcpy(&storage, &pointer, sizeof(pointer));
  return storage;
}

}

MPI_Comm mpiwrap_COMM_WORLD() {
  MPI_Comm value{};
  value.storage = pointerStorage(thread_mpi::TMPI_COMM_WORLD);
  return value;
}

MPI_Datatype mpiwrap_INT() {
  MPI_Datatype value{};
  value.storage = pointerStorage(thread_mpi::TMPI_INT);
  return value;
}

MPI_Op mpiwrap_SUM() {
  MPI_Op value{};
  static_assert(sizeof(thread_mpi::tMPI_Op) <= sizeof(value.storage),
                "tMPI operation exceeds wrapper storage");
  const thread_mpi::tMPI_Op native = thread_mpi::TMPI_SUM;
  std::memcpy(&value.storage, &native, sizeof(native));
  return value;
}

void *mpiwrap_IN_PLACE() {
  return nullptr;
}

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
      !mpiwrap_has_Barrier()) {
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
