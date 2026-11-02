# MPI provider wrapper

This module lets one PLUMED kernel use MPI through one selected runtime while
preserving the existing Communicator source interface. It has three mutually
exclusive configure modes: native MPI, internal thread-MPI, and fully serial.
The native build keeps PLUMED's existing direct MPI path. The thread-MPI build
uses the pinned GROMACS thread-MPI implementation. A fully serial build has
neither provider and retains the existing serial behavior.

## Configuration and capability reporting

Native MPI is discovered through --enable-mpi (enabled by default). Internal
thread-MPI defaults on when native MPI is unavailable; use
--disable-thread-mpi to turn it off. Explicitly requesting both available
providers is an error. --disable-mpi --disable-thread-mpi configures a fully
serial build.

__PLUMED_HAS_MPI means native MPI only. __PLUMED_HAS_THREAD_MPI means the
internal runtime is built, and __PLUMED_MPIWRAP keeps one consistent wrapper
handle layout in every translation unit of such a build. Config reports
has mpi when either MPI provider exists, and also reports real_mpi and
thread_mpi separately. The regtest runner uses real_mpi when available and
falls back to the legacy native-only has mpi query for older executables.
Use --no-mpi --has-thread-mpi to query the capability without triggering
native MPI initialization in a native-MPI executable.

## Handle and ABI boundary

In provider mode, each public MPI handle is a trivial, standard-layout POD
record. Its zero-filled storage starts at offset zero and holds the native
handle representation. This is a deliberate compatibility contract:
existing PLUMED callers can keep passing &Communicator::Get_comm() to nested
kernels without changing their source or creating export buffers. A compatible
receiver copies exactly the selected provider's reported native handle size
from that address into an aligned native handle object. The storage capacity
is validated before use. Matching bytes do not make a handle valid across
unrelated MPI runtimes.

The wrapper and native adapter communicate through a versioned C-compatible
provider descriptor. Kernel-table version 5 adds the injection/launch entries
after the unchanged version-4 prefix; the provider descriptor has its own ABI
version. Fixed entry layouts are frozen within a provider ABI major; new optional
capabilities use named metadata entries. Function pointers travel through a
generic carrier via byte copying and are restored to their exact typed
signature before invocation. Integer values use explicit fixed-width and native
conversion.

PLUMED_MPIW_HANDLE_TYPE defaults to uintptr_t, and
PLUMED_MPIW_STATUS_CAPACITY defaults to 128 bytes. Providers must fit those
configured representations, and installation validates the capacities and
native scalar metadata before publishing callbacks. The frontend signatures
follow MPI-5 const correctness. The native adapter keeps any compatibility
cast for older native headers local to an input-only MPI call; callback types
and writable arguments retain their exact types.

Internal launch is explicit and one-shot, and must happen before a PLUMED object
is created. The launcher owns worker lifetime and finalization. A callback
exception is fatal to the process rather than unwound through the C/tMPI
boundary.

plumed_kernel_open_static, plumed_kernel_open_dlopen, and
plumed_kernel_open_selected create a kernel handle. A caller can install a
provider with plumed_kernel_inject_mpi, load the provider library with
plumed_kernel_load_mpi_wrapper, create objects with plumed_kernel_create, and
launch internal ranks with plumed_kernel_launch_internal_threads when needed. Provider selection must happen before the first wrapper use or
kernel object creation. Objects retain their kernel/provider lifetime; release
the object with plumed_finalize and release the handle with the matching
plumed_kernel_release. Releasing a handle does not finalize an object.
C++ users can use the PLMD::Kernel RAII convenience API.

## Generated interface and runtime files

All module files live directly in src/mpiwrap. The machine-readable inputs
are support-matrix.tsv, argument-metadata.json, constant-metadata.json,
type-metadata.json, signature-versions.json, and thread-mpi-api.tsv.
Build the repository formatter once with make -C astyle. generate.py validates
these inputs and generates the wrapper declarations, provider ABI, constant
accessors, route tables, signature checks, and coverage reports in the same
directory. The checked-in generated files are reproducible
with:

    python3 src/mpiwrap/generate.py --check

The .cpp.in and .c.in files are compile-only tests and probes; the module's
wildcard source build does not compile them. The implementation unity source
mpiwrap.cpp is built only when thread-MPI is configured and includes provider
registry, frontend, and internal adapter fragments. mpiwrap-launcher.cpp
includes the internal rank launcher. mpiwrap_implementation.cpp is an opt-in
translation unit for a provider DSO; it is not compiled into ordinary PLUMED
kernels. Imported thread-MPI sources remain a separate module at
src/thread_mpi.

## Current implementation and limits

The internal provider has a tested launch path and a usable initial MPI subset,
including communicator rank/size, collectives, point-to-point operations,
status/count handling, and selected handle/datatype operations. Functions with
no safe route remain declared and their availability inquiry reports false;
calling them reports the missing capability. The standard variadic
MPI_Pcontrol remains direct in native mode and unavailable through provider
forwarding. Custom tMPI error handlers remain excluded as specified by the
stable provider surface.

The native adapter currently publishes 101 of the 118 retained function
entries and all 80 constants. The omitted entries are MPI_Init/Finalize
(lifecycle-owned), MPI_Pcontrol (variadic), MPI_Comm_create_errhandler and
MPI_Op_create (callback bridges), and MPI_Gatherv, MPI_Scatterv,
MPI_Alltoallv, MPI_Reduce_scatter, MPI_Graph_create, MPI_Graph_get,
MPI_Cart_get, MPI_Cart_rank, MPI_Cart_coords, MPI_Graph_neighbors,
MPI_Cart_sub, and MPI_Graph_map (array extents or output lengths need explicit
conversion metadata).
Their frontend declarations remain available and their inquiries report the
missing provider capability. The mock-native probes compile against real
provider headers and exercise call-through without initializing MPI; they are
not real MPI runtime communication tests. Native MPI startup and multi-process
communication were blocked in the development sandbox by process-manager and
socket permissions, so those semantics still need verification outside that
environment. The pinned thread-MPI source has a one-rank
out-of-place Allreduce defect; the wrapper handles its size-one identity case
without relying on that defective operation.

## Verification

The focused checks can be run from the repository root:

    python3 src/mpiwrap/validate-manifest.py
    python3 src/mpiwrap/generate.py --check
    sh src/mpiwrap/check-provider-registry.sh
    bash src/mpiwrap/check-kernel-lifecycle.sh
    bash src/mpiwrap/check-regtest-runner.sh
    bash src/mpiwrap/check-native-adapter.sh

Header and generated signature checks for local MPICH, Open MPI, and pinned
tMPI use:

    MPICH_CXX=/usr/bin/clang++ OMPI_CXX=/usr/bin/clang++ \
      sh src/mpiwrap/check-generated.sh \
      /Users/bussi/miniconda3/envs/mpich/bin/mpicxx \
      /Users/bussi/miniconda3/envs/openmpi/bin/mpicxx \
      src/thread_mpi/include

Internal launch semantics are exercised by
src/mpiwrap/check-thread-mpi-cli.sh src/lib/plumed-static after a configured
thread-MPI build. The tests cover 1, 2, and 4 ranks, callback error handling,
provider lifecycle, raw communicator byte-prefix handling, native/legacy
runner selection, and CHDIR rejection while thread-MPI is active.
