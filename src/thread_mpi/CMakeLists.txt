cmake_minimum_required(VERSION 3.16)
project(plumed_thread_mpi LANGUAGES CXX)

find_package(Threads REQUIRED)
set(TMPI_SOURCES
  src/alltoall.cpp src/atomic.cpp src/barrier.cpp src/bcast.cpp
  src/collective.cpp src/comm.cpp src/errhandler.cpp src/event.cpp
  src/gather.cpp src/group.cpp src/list.cpp src/lock.cpp src/numa_malloc.cpp
  src/once.cpp src/p2p_protocol.cpp src/p2p_send_recv.cpp src/p2p_wait.cpp
  src/profile.cpp src/pthreads.cpp src/reduce.cpp src/reduce_fast.cpp
  src/scan.cpp src/scatter.cpp src/tmpi_init.cpp src/tmpi_malloc.cpp
  src/topology.cpp src/type.cpp)
add_library(plumed_thread_mpi STATIC ${TMPI_SOURCES})
target_include_directories(plumed_thread_mpi PUBLIC include PRIVATE src)
target_compile_features(plumed_thread_mpi PUBLIC cxx_std_17)
target_compile_definitions(plumed_thread_mpi PRIVATE
  THREAD_PTHREADS HAVE_UNISTD_H HAVE_SYS_TIME_H HAVE_PTHREAD_H)
target_link_libraries(plumed_thread_mpi PUBLIC Threads::Threads)

add_executable(thread_mpi_launch_test tests/launch.cpp)
target_link_libraries(thread_mpi_launch_test PRIVATE plumed_thread_mpi)

add_executable(mpiwrap_runtime_test
  tests/mpiwrap-runtime.cpp
  ../mpiwrap/runtime/provider-registry.cpp
  ../mpiwrap/runtime/provider-frontend.cpp
  ../mpiwrap/runtime/thread-mpi-adapter.cpp)
target_include_directories(mpiwrap_runtime_test PRIVATE
  ../mpiwrap/generated ../mpiwrap/runtime)
target_link_libraries(mpiwrap_runtime_test PRIVATE plumed_thread_mpi)

enable_testing()
foreach(ranks IN ITEMS 1 2 4)
  add_test(NAME launch_${ranks} COMMAND thread_mpi_launch_test ${ranks})
  set_tests_properties(launch_${ranks} PROPERTIES TIMEOUT 20)
endforeach()
foreach(ranks IN ITEMS 1 2 4)
  add_test(NAME mpiwrap_runtime_${ranks} COMMAND mpiwrap_runtime_test ${ranks})
  set_tests_properties(mpiwrap_runtime_${ranks} PROPERTIES TIMEOUT 20)
endforeach()
