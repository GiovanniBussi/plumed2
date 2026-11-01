#include <dlfcn.h>
#include <cstdlib>

struct Functions {
  void *(*create)();
  void (*cmd)(void *,const char *,const void *);
  void (*finalize)(void *);
};
struct SymbolTable {
  int version;
  Functions functions;
  void (*cmd_nothrow)(void *,const char *,const void *,void *);
  void (*cmd_safe)(void *,const char *,void *);
  void (*cmd_safe_nothrow)(void *,const char *,void *,void *);
  unsigned (*create_reference)(void *);
  unsigned (*delete_reference)(void *);
  unsigned (*use_count)(void *);
  int (*install_mpi_provider)(const void *,int (*)(void *,int,const char *,const char *),void *,void *);
};

namespace {
void *provider_library=nullptr;
}

extern "C" void *plumed_test_create() {
  return std::malloc(1);
}
extern "C" void plumed_test_cmd(void *,const char *,const void *) {}
extern "C" void plumed_test_finalize(void *object) {
  std::free(object);
}
extern "C" unsigned plumed_test_create_reference(void *) {
  return 2;
}
extern "C" unsigned plumed_test_delete_reference(void *) {
  return 0;
}
extern "C" unsigned plumed_test_use_count(void *) {
  return 1;
}
extern "C" int plumed_test_install(const void *descriptor,
                                   int (*)(void *,int,const char *,const char *),
                                   void *,void *library) {
  if(!descriptor) {
    return 1;
  }
  provider_library=library;
  return 0;
}
extern "C" SymbolTable plumed_symbol_table= {
  5,
  {plumed_test_create,plumed_test_cmd,plumed_test_finalize},
  nullptr,nullptr,nullptr,plumed_test_create_reference,
  plumed_test_delete_reference,plumed_test_use_count,
  plumed_test_install
};
__attribute__((destructor)) static void release_provider() {
  if(provider_library) {
    dlclose(provider_library);
  }
}
