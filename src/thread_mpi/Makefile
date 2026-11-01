USE=core

# The imported implementation is namespaced and retains its original source
# files under src/. Keep one translation unit per upstream file for isolation.
ADDCPPFLAGS += -Iinclude -Isrc -DTHREAD_PTHREADS -DHAVE_UNISTD_H \
  -DHAVE_SYS_TIME_H -DHAVE_PTHREAD_H -DSIZEOF_LONG_LONG_INT

include ../maketools/make.module
