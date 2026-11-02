cmake_minimum_required(VERSION 3.16)
project(plumed_thread_mpi LANGUAGES CXX)

find_package(Threads REQUIRED)
# Imported upstream implementation sources stay in sync with reimport.py.
file(GLOB TMPI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(plumed_thread_mpi STATIC ${TMPI_SOURCES})
target_include_directories(plumed_thread_mpi PUBLIC . PRIVATE .)
target_compile_features(plumed_thread_mpi PUBLIC cxx_std_17)
target_compile_definitions(plumed_thread_mpi PRIVATE
  THREAD_PTHREADS HAVE_UNISTD_H HAVE_SYS_TIME_H HAVE_PTHREAD_H
  SIZEOF_LONG_LONG_INT __PLUMED_HAS_THREAD_MPI)
target_link_libraries(plumed_thread_mpi PUBLIC Threads::Threads)

set(TEST_INPUTS
  launch.cpp
  host-runner.cpp
  internal-runner.cpp
  internal-launcher.cpp
  isolation-main.cpp
  mpiwrap-runtime.cpp)
foreach(name IN LISTS TEST_INPUTS)
  configure_file(${name}.in ${name} COPYONLY)
endforeach()

add_executable(thread_mpi_launch_test ${CMAKE_CURRENT_BINARY_DIR}/launch.cpp)
target_link_libraries(thread_mpi_launch_test PRIVATE plumed_thread_mpi)

add_executable(thread_mpi_internal_launcher_test
  ${CMAKE_CURRENT_BINARY_DIR}/internal-launcher.cpp
  ../mpiwrap/mpiwrap.cpp
  ../mpiwrap/mpiwrap-launcher.cpp)
target_include_directories(thread_mpi_internal_launcher_test PRIVATE
  ../mpiwrap .)
target_compile_definitions(thread_mpi_internal_launcher_test PRIVATE
  SIZEOF_LONG_LONG_INT __PLUMED_HAS_THREAD_MPI __PLUMED_MPIWRAP)
target_link_libraries(thread_mpi_internal_launcher_test PRIVATE plumed_thread_mpi)

add_executable(mpiwrap_runtime_test
  ${CMAKE_CURRENT_BINARY_DIR}/mpiwrap-runtime.cpp
  ../mpiwrap/mpiwrap.cpp
  ../mpiwrap/mpiwrap-launcher.cpp)
target_include_directories(mpiwrap_runtime_test PRIVATE
  ../mpiwrap .)
target_compile_definitions(mpiwrap_runtime_test PRIVATE
  SIZEOF_LONG_LONG_INT __PLUMED_HAS_THREAD_MPI __PLUMED_MPIWRAP)
target_link_libraries(mpiwrap_runtime_test PRIVATE plumed_thread_mpi)

enable_testing()
foreach(ranks IN ITEMS 1 2 4)
  add_test(NAME launch_${ranks} COMMAND thread_mpi_launch_test ${ranks})
  set_tests_properties(launch_${ranks} PROPERTIES TIMEOUT 20)
  add_test(NAME internal_launcher_${ranks}
    COMMAND thread_mpi_internal_launcher_test ${ranks})
  set_tests_properties(internal_launcher_${ranks} PROPERTIES TIMEOUT 20)
endforeach()
add_test(NAME internal_launcher_callback_exception
  COMMAND ${CMAKE_COMMAND}
  -DLAUNCHER=$<TARGET_FILE:thread_mpi_internal_launcher_test>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/check-launcher-fatal.cmake)
set_tests_properties(internal_launcher_callback_exception PROPERTIES
  TIMEOUT 20)
add_test(NAME internal_unlaunched_serial
  COMMAND thread_mpi_internal_launcher_test serial)
set_tests_properties(internal_unlaunched_serial PROPERTIES TIMEOUT 20)
foreach(ranks IN ITEMS 1 2 4)
  add_test(NAME mpiwrap_runtime_${ranks} COMMAND mpiwrap_runtime_test ${ranks})
  set_tests_properties(mpiwrap_runtime_${ranks} PROPERTIES TIMEOUT 20)
endforeach()
