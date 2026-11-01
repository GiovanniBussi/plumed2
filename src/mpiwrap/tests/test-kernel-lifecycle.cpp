#define __PLUMED_WRAPPER_IMPLEMENTATION 1
#define __PLUMED_WRAPPER_EXTERN 0
#define __PLUMED_HAS_DLOPEN 1
#include "../../wrapper/Plumed.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <type_traits>

extern "C" plumed_symbol_table_type *plumed_symbol_table_reexport() {
  return nullptr;
}

static_assert(std::is_move_constructible<PLMD::Kernel>::value,
              "PLMD::Kernel must be movable");

namespace {
int reportError(void *,int,const char *,const char *message) {
  std::fprintf(stderr,"kernel API error: %s\n",message);
  return 0;
}

bool exists(const char *path) {
  std::ifstream input(path);
  return input.good();
}
}

int main(int argc,char **argv) {
  if(argc!=5) {
    return 2;
  }
  const char *marker=argv[3];
  std::remove(marker);
  setenv("MPIW_TEST_PROVIDER_MARKER",marker,1);

  plumed_kernel_handle kernel=nullptr;
  if(plumed_kernel_open_dlopen(argv[1],RTLD_NOW|RTLD_LOCAL,&kernel,
                               &reportError,nullptr)!=PLUMED_KERNEL_OK) {
    return 3;
  }
  const int caller_owned_descriptor=1;
  if(plumed_kernel_inject_mpi(kernel,&caller_owned_descriptor,
                              &reportError,nullptr)!=PLUMED_KERNEL_OK) {
    plumed_kernel_release(kernel);
    return 4;
  }
  plumed object=plumed_create_invalid();
  if(plumed_kernel_create(kernel,&object,&reportError,nullptr)!=PLUMED_KERNEL_OK ||
      !plumed_valid(object)) {
    plumed_kernel_release(kernel);
    return 5;
  }
  plumed_kernel_release(kernel);
  plumed_finalize(object);

  kernel=nullptr;
  if(plumed_kernel_open_dlopen(argv[1],RTLD_NOW|RTLD_LOCAL,&kernel,
                               &reportError,nullptr)!=PLUMED_KERNEL_OK) {
    return 6;
  }
  if(plumed_kernel_load_mpi_wrapper(kernel,argv[2],&reportError,nullptr)!=
      PLUMED_KERNEL_OK) {
    plumed_kernel_release(kernel);
    return 7;
  }
  object=plumed_create_invalid();
  if(plumed_kernel_create(kernel,&object,&reportError,nullptr)!=PLUMED_KERNEL_OK ||
      !plumed_valid(object)) {
    plumed_kernel_release(kernel);
    return 8;
  }
  plumed_kernel_release(kernel);
  if(exists(marker)) {
    plumed_finalize(object);
    return 9;
  }
  plumed_finalize(object);
  if(!exists(marker)) {
    return 10;
  }

  kernel=nullptr;
  if(plumed_kernel_open_dlopen(argv[1],RTLD_NOW|RTLD_LOCAL,&kernel,
                               &reportError,nullptr)!=PLUMED_KERNEL_OK) {
    return 11;
  }
  plumed_kernel_release(kernel);
  kernel=nullptr;
  if(plumed_kernel_open_dlopen(argv[4],RTLD_NOW|RTLD_LOCAL,&kernel,
                               &reportError,nullptr)!=PLUMED_KERNEL_UNSUPPORTED || kernel) {
    if(kernel) {
      plumed_kernel_release(kernel);
    }
    return 12;
  }
  return 0;
}
