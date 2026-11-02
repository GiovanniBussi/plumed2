#include <thread_mpi_tmpi.h>
#include "../../mpiwrap/thread-mpi-launcher.hpp"
#include "../../mpiwrap/provider-registry.hpp"
#include "../../mpiwrap/mpiwrap.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace PLMD {
namespace thread_mpi_launcher_test {

int rank_callback(int rank, const void *native_comm, std::size_t native_size,
                  void *opaque) {
  if (opaque && *static_cast<int *>(opaque) == rank) {
    throw std::runtime_error("injected callback failure");
  }
  if (!native_comm || native_size != sizeof(thread_mpi::tMPI_Comm)) {
    return 101;
  }
  int wrapper_rank = -1;
  if (mpiwrap_Comm_rank(mpiwrap_COMM_WORLD(), &wrapper_rank) !=
      thread_mpi::TMPI_SUCCESS || wrapper_rank != rank) {
    return 100;
  }
  return rank + 1;
}

int run(int ranks, int throw_rank) {
  if (throw_rank >= 0) {
    int result = 0;
    (void)mpiwrap_provider::launchInternalThreadMPI(
      ranks, rank_callback, &throw_rank, &result, nullptr, nullptr);
    return 1;
  }
  int result = -1;
  const int status = mpiwrap_provider::launchInternalThreadMPI(
                       ranks, rank_callback, nullptr, &result, nullptr, nullptr);
  if (status != 0 || result != ranks) {
    std::cerr << "internal launch result mismatch: status=" << status
              << " result=" << result << " expected=" << ranks << '\n';
    return 1;
  }
  int finalized = 0;
  if (thread_mpi::tMPI_Finalized(&finalized) != thread_mpi::TMPI_SUCCESS ||
      !finalized) {
    std::cerr << "internal launch did not finalize its worker runtime\n";
    return 1;
  }

  int second_result = 0;
  if (mpiwrap_provider::launchInternalThreadMPI(
        ranks, rank_callback, nullptr, &second_result, nullptr, nullptr) == 0) {
    std::cerr << "repeated internal launch was accepted\n";
    return 1;
  }
  return 0;
}

int runUnlaunchedSerial() {
  auto &registry = mpiwrap_provider::currentRegistry();
  if (registry.installed()) {
    return 1;
  }
  int initialized = 1;
  if (mpiwrap_Initialized(&initialized) != thread_mpi::TMPI_SUCCESS ||
      initialized != 0 || registry.installed()) {
    std::cerr << "MPI_Initialized selected or initialized a provider\n";
    return 1;
  }
  mpiwrap_provider::noteKernelObjectCreated();
  if (!registry.installed() || registry.runtimeKind() !=
      PLUMED_MPIW_RUNTIME_THREAD_MPI) {
    std::cerr << "ordinary object creation did not select the default provider\n";
    return 1;
  }
  const MPI_Comm self = mpiwrap_COMM_SELF();
  if (self.storage != 0) {
    std::cerr << "pre-init MPI_COMM_SELF was not the zero-filled placeholder\n";
    return 1;
  }
  if (mpiwrap_Initialized(&initialized) != thread_mpi::TMPI_SUCCESS ||
      initialized != 0) {
    std::cerr << "provider selection initialized thread-MPI unexpectedly\n";
    return 1;
  }
  return 0;
}

}  // namespace thread_mpi_launcher_test
}  // namespace PLMD

int main(int argc, char **argv) {
  if (argc == 2 && std::strcmp(argv[1], "serial") == 0) {
    return PLMD::thread_mpi_launcher_test::runUnlaunchedSerial();
  }
  if (argc != 2 && argc != 3) {
    return 2;
  }
  const int ranks = std::atoi(argv[1]);
  if (ranks < 1 || ranks > 4) {
    return 2;
  }
  int throw_rank = -1;
  if (argc == 3) {
    if (std::strcmp(argv[2], "throw") != 0) {
      return 2;
    }
    throw_rank = ranks > 1 ? 1 : 0;
  }
  return PLMD::thread_mpi_launcher_test::run(ranks, throw_rank);
}
