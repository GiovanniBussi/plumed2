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
};
extern "C" void *plumed_old_create() {
  return nullptr;
}
extern "C" void plumed_old_cmd(void *,const char *,const void *) {}
extern "C" void plumed_old_finalize(void *) {}
extern "C" SymbolTable plumed_symbol_table= {
  4,{plumed_old_create,plumed_old_cmd,plumed_old_finalize},
  nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
};
