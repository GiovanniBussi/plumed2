cmake_minimum_required(VERSION 3.16)
project(plumed_long_long_mpi LANGUAGES CXX)
get_filename_component(PLUMED_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(SOURCE_ROOT "${PLUMED_ROOT}/src")
file(GLOB THREAD_MPI_SOURCES CONFIGURE_DEPENDS "${SOURCE_ROOT}/thread_mpi/*.cpp")
add_executable(long_long_mpi_test
  runtime-check.cpp
  ${SOURCE_ROOT}/mpiwrap/mpiwrap.cpp
  ${SOURCE_ROOT}/mpiwrap/loader.cpp
  ${SOURCE_ROOT}/mpiwrap/fallbacks.cpp
  ${SOURCE_ROOT}/mpiwrap/launcher.cpp
  ${SOURCE_ROOT}/mpiwrap/tmpi.cpp
  ${SOURCE_ROOT}/tools/Exception.cpp
  ${THREAD_MPI_SOURCES})
target_compile_features(long_long_mpi_test PRIVATE cxx_std_17)
target_compile_definitions(long_long_mpi_test PRIVATE
  __PLUMED_MPIWRAP __PLUMED_HAS_THREAD_MPI SIZEOF_LONG_LONG_INT)
target_include_directories(long_long_mpi_test PRIVATE
  ${SOURCE_ROOT} ${SOURCE_ROOT}/thread_mpi)
if(WIN32)
  target_compile_definitions(long_long_mpi_test PRIVATE
    THREAD_WINDOWS NOMINMAX WIN32_LEAN_AND_MEAN)
else()
  find_package(Threads REQUIRED)
  target_compile_definitions(long_long_mpi_test PRIVATE
    THREAD_PTHREADS HAVE_UNISTD_H HAVE_SYS_TIME_H HAVE_PTHREAD_H)
  target_link_libraries(long_long_mpi_test PRIVATE Threads::Threads)
endif()
enable_testing()
foreach(ranks IN ITEMS 1 2 4)
  add_test(NAME long_long_mpi_${ranks} COMMAND long_long_mpi_test ${ranks})
  set_tests_properties(long_long_mpi_${ranks} PROPERTIES TIMEOUT 60)
endforeach()
