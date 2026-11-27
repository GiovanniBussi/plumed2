// Driverless Linux unload check: compile without linking libplumed or a GPU runtime.
#define __PLUMED_WRAPPER_LINK_RUNTIME 1
#define __PLUMED_WRAPPER_IMPLEMENTATION 1
#define __PLUMED_HAS_DLOPEN 1
#include "plumed/wrapper/Plumed.h"
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition,const char* message) {
  if(!condition) {
    throw std::runtime_error(message);
  }
}
bool resident(const char* path) {
  void* handle=dlopen(path,RTLD_NOW|RTLD_LOCAL|RTLD_NOLOAD);
  if(handle) {
    dlclose(handle);
  }
  return handle!=nullptr;
}
}

int main(int argc,char** argv) try {
  require(argc>=4,"Expected kernel, runtime symbol, mode [module library ...]");
  const char* kernel=argv[1];
  const char* runtimeSymbol=argv[2];
  const std::string mode=argv[3];
  require(mode=="native" || mode=="autoload" || mode=="explicit","Unknown load mode");
  require((argc-4)%2==0,"Expected module/library pairs");
  setenv("PLUMED_AUTOLOAD",mode=="autoload" ? "1" : "0",1);
  for(int cycle=0; cycle<3; ++cycle) {
    plumed p=plumed_create_dlopen(kernel);
    require(plumed_valid(p),"Cannot create PLUMED");
    if(mode=="explicit") {
      int atoms=0;
      plumed_cmd(p,"setNatoms",&atoms);
      plumed_cmd(p,"setLogFile","/dev/null");
      plumed_cmd(p,"init",nullptr);
      for(int index=4; index<argc; index+=2) {
        const std::string load="LOAD MODULE="+std::string(argv[index]);
        plumed_cmd(p,"readInputLine",load.c_str());
      }
    }
    std::vector<void*> runtimes;
    auto probe=[&](const char* path) {
      // NOLOAD: a missing autoload must fail, rather than loading it for the test.
      void* handle=dlopen(path,RTLD_NOW|RTLD_LOCAL|RTLD_NOLOAD);
      require(handle,"Expected library was not loaded");
      void* runtime=dlsym(handle,runtimeSymbol);
      dlclose(handle);
      require(runtime,"Cannot resolve the shared GPU runtime");
      runtimes.push_back(runtime);
    };
    if(mode=="native") {
      probe(kernel);
    }
    for(int index=4; index<argc; index+=2) {
      probe(argv[index+1]);
    }
    plumed_finalize(p);
    for(int index=4; index<argc; index+=2) {
      require(!resident(argv[index+1]),"GPU extension remains mapped");
    }
    // Strict: these CI builds use -fno-gnu-unique. Do not silently skip a
    // failure caused by a new GNU-unique symbol or a runtime-to-kernel binding.
    require(!resident(kernel),"GPU runtime retention kept the PLUMED kernel mapped");
    for(void* runtime : runtimes) {
      Dl_info info{};
      require(dladdr(runtime,&info),"Shared GPU runtime was unloaded");
      std::printf("Retained runtime: %s\n",info.dli_fname);
    }
  }
  return 0;
} catch(const std::exception& error) {
  std::fprintf(stderr,"Runtime lifetime check failed: %s\n",error.what());
  return 1;
}
