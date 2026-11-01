USE=core lapack lepton small_vector xdrfile molfile

# Native MPI keeps its direct call path. Other builds compile the communicator
# against the generated provider wrappers, which remain dormant until install.
ifeq ($(findstring -D__PLUMED_HAS_MPI,$(CPPFLAGS)),)
ADDCPPFLAGS += -D__PLUMED_MPIWRAP
endif

# generic makefile
include ../maketools/make.module

# -Wshadows helps to find slips in the code, 
# -Werror makes fixing warnings mandatory
CXXFLAGS:= $(CXXFLAGS) -Wshadow -Werror

