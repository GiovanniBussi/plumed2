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
internal runtime is built. __PLUMED_MPIWRAP is always defined by configure:
availability of the wrapper frontend is separate from selecting a provider.
Native MPI still uses its actual handle types and direct calls. Config reports
has mpi when either MPI provider exists, and also reports real_mpi and
thread_mpi separately. The CLI provides --has-mpi for combined capability,
--has-real-mpi for a native build and --has-thread-mpi for the internal runtime.
These distinguish capability from MPI initialization. The regtest runner uses
--has-mpi for its skip decision and probes --has-thread-mpi for internal launch;
older executables without that query retain their native MPI launcher.

Communicator construction evaluates MPI_COMM_SELF, as in the original code.
Its lazy accessor selects the provider before storing the communicator: inject
the host implementation before the first provider-dependent constant or call.
Provider selection and initialization are separate; ordinary serial operations
still use local fallbacks until MPI is initialized. In a fully disabled build,
MPI_COMM_SELF supplies the serial surrogate when no provider was selected.
Capability inquiries themselves do not select a provider. Explicit environment
selection and the troubleshooting override are resolved by provider-dependent
access; no dummy per-object lifecycle field or notification is needed.
The separate `mpiwrap/mpi.h` inquiry reports whether an installed, initialized
provider executes ranks as threads; the PLUMED action uses it for CHDIR safety.

## Handle and ABI boundary

In provider mode, each public MPI handle is a trivial, standard-layout POD
record. Its zero-filled storage starts at offset zero and holds the native
handle representation. This is a deliberate compatibility contract:
existing PLUMED callers can keep passing &Communicator::Get_comm() to nested
kernels without changing their source or creating export buffers. A compatible
receiver copies exactly the selected provider's reported native handle size
from that address into zero-filled wrapper storage. The storage capacity
is validated before use. Matching bytes do not make a handle valid across
unrelated MPI runtimes.

The wrapper and native adapter communicate through a versioned C-compatible
provider descriptor. Kernel-table version 5 adds the injection/launch entries
after the unchanged version-4 prefix; the provider descriptor has its own ABI
version. Fixed entry layouts are frozen within a provider ABI major; new optional
capabilities use named metadata entries. Each injected implementation takes
one pointer per argument plus a result-storage pointer, following the draft;
there is no array of argument pointers. Function pointers use a generic
function-pointer carrier and are cast back to the exact generated per-function
transport type before invocation. IDs are hashes of function names used for
validation, never addresses or table offsets. Entries append contiguously in
array order, including conditional entries. PLUMED handles native storage,
array strides and status conversion; injected implementations call native MPI.
The nullable description is copied at installation for diagnostics only.
`ranks_are_threads` describes execution; absence of a function means unavailable.

PLUMED_MPIW_HANDLE_CAPACITY defaults to max(8, sizeof(void *)); configure with
--with-mpiwrap-handle-capacity=BYTES to choose a positive byte count. Use
--with-mpiwrap-status-capacity=BYTES to change the status default of 128 bytes.
Configured sizes reach all kernel translation units and installed public
headers. Rebuild clients using wrapper types when changing these sizes;
explicit compiler overrides must agree with the kernel. Native MPI's direct
types are independent of these capacities, as is the thin native provider;
provider installation checks that its native values fit the receiving kernel. Handle storage is a zero-filled
byte-array POD. Provider ABI major 3 uses fixed per-function pointer transport
and rejects older incompatible provider layouts before reading their entries. PLUMED_MPIW_STATUS_CAPACITY
defaults to 128 bytes. Providers must fit those configured representations,
and installation validates the capacities and native scalar metadata before
publishing function pointers. The frontend signatures
follow MPI-5 const correctness. The native adapter keeps any compatibility
cast for older native headers local to an input-only MPI call; callback types
and writable arguments retain their exact types.

Internal launch is explicit and one-shot, and must precede the first
provider-dependent constant use (including construction of a communicator). The launcher owns worker lifetime and finalization. A callback
exception is fatal to the process rather than unwound through the C/tMPI
boundary.

plumed_kernel_open_static, plumed_kernel_open_dlopen, and
plumed_kernel_open_selected create a kernel handle. A caller can install a
provider with plumed_kernel_inject_mpi, load the provider library with
plumed_kernel_load_mpi_wrapper, create objects with plumed_kernel_create, and
launch internal ranks with plumed_kernel_launch_internal_threads when needed.
Inject before the first provider-dependent constant or function is used. Objects retain their kernel/provider lifetime; release
the object with plumed_finalize and release the handle with the matching
plumed_kernel_release. Releasing a handle does not finalize an object.
C++ users can use the PLMD::Kernel RAII convenience API.

## Inject the host MPI from Plumed.h

When the host needs to inject the MPI implementation already used by its MD
code, define `PLUMED_MPIW_HOST_ADAPTER` and
`__PLUMED_WRAPPER_IMPLEMENTATION` in exactly one translation unit before
including `Plumed.h`. Include the host MPI bindings first. For GROMACS
thread-MPI, include `thread_mpi/mpi_bindings.h` first and ensure
`GMX_THREAD_MPI` has its numeric build value. Include `gromacs/version.h` so
`GMX_VERSION` is available, or explicitly define `PLUMED_MPIW_GROMACS_VERSION`
using the same numeric encoding (2026.0 is 20260000). Unknown versions and
versions earlier than 2026.0 are rejected when building the injected implementation. This opt-in header embeds the
same generated adapter body, so it does not need a separate provider source or
provider library.

Open the specific kernel, call `plumed_kernel_inject_native_mpi` before
creating its first object, then create the object from that kernel handle.
The helper takes its descriptor from the MPI bindings included in that
translation unit. Existing `setMPIComm` callers can continue passing the
address of their native communicator: PLUMED imports the provider-sized bytes
into zero-filled wrapper storage, whose native byte prefix has the same
layout. This byte-copy contract is why the wrapper representation preserves
the original communicator pointer path without caller-side export buffers.
The generated adapter omits thread-MPI functions and constants unavailable in
the selected GROMACS headers; PLUMED reports calls to omitted functions as a
clear runtime error.

## Build a native provider

Use `plumed mkmpi --compiler /absolute/path/to/mpicc --out provider.so`
(or an MPI C++ compiler driver) to build the shared native adapter. The command
reuses configure-tested compiler/shared-library flags, relies on the `.cpp`
suffix for language selection, and adds no language-standard flag. The adapter
is C++98-compatible and does not link a PLUMED kernel.
Use a `.dylib` output name on macOS. Pass each additional compiler or linker
argument separately with repeated `--cflag ARG` or `--ldflag ARG` options;
arguments are never shell-evaluated. The selected compiler must support C++.
The adapter sources are installed with PLUMED, so this command does not need
the original source checkout. Failed builds preserve an existing output.

Load the result into an opened kernel with `plumed_kernel_load_mpi_wrapper`
or `PLMD::Kernel::loadMPIWrapper` before creating objects. Building a provider
does not initialize MPI: the host remains responsible for its MPI lifecycle.
Alternatively, set `PLUMED_MPI_WRAPPER` to an absolute provider-library path
or an absolute executable MPI compiler path. Explicit pre-creation injection
takes precedence; otherwise the environment selection precedes the internal
provider. A loadable library must expose the provider factory. A non-loadable
executable compiler invokes the same `mkmpi` command in a private temporary
directory, with compiler diagnostics inherited by the process. Files with
shared-library suffixes are never executed as compiler commands. The kernel
retains the loaded provider library; temporary build files are removed after
loading. This selection does not initialize the host MPI runtime.

For troubleshooting, set `PLUMED_MPI_WRAPPER_FORCE=1` together with
`PLUMED_MPI_WRAPPER` before any provider is installed. This makes that
environment-selected provider take precedence over an incoming injection.
Without the force flag, injection retains priority. An absent provider path,
an invalid force value, or a late attempt to replace an installed provider
raises an error. A bypassed injected library reference is released after the
forced provider is installed successfully; failure leaves ownership with the
caller. Already published callbacks are never replaced.

## Generated interface and runtime files

All module files live directly in src/mpiwrap. The machine-readable inputs
are support-matrix.tsv, argument-metadata.json, constant-metadata.json,
type-metadata.json, signature-versions.json, availability-metadata.json, and
thread-mpi-api.tsv. Signature revisions are independent of MPI standard versions.
Build the repository formatter once with make -C astyle. generate.py validates
these inputs and generates the wrapper declarations, provider ABI, constant
accessors, route tables, signature checks, and coverage reports in the same
directory. The checked-in generated files are reproducible
with:

    python3 src/mpiwrap/generate.py --check

The .cpp.in and .c.in files are compile-only tests and probes; the module's
wildcard source build does not compile them. The implementation unity source
mpiwrap.cpp supplies the registry and frontend in wrapper mode, including when
the embedded runtime is disabled. Its internal adapter is compiled only when
thread-MPI is configured. mpiwrap-launcher.cpp
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

The native adapter can publish 113 of the 118 retained function entries;
actual publication also depends on the host MPI version and supported constants. The omitted entries are MPI_Init/Finalize
(lifecycle-owned), MPI_Pcontrol (variadic), MPI_Comm_create_errhandler and
MPI_Op_create (callback bridges).
Their frontend declarations remain available and their inquiries report the
missing provider capability. The mock-native probes compile against real
provider headers and exercise call-through without initializing MPI; they are
not real MPI runtime communication tests. Native MPI startup and multi-process
communication were blocked in the development sandbox by process-manager and
socket permissions, so those semantics still need verification outside that
environment. The pinned thread-MPI source has a one-rank
out-of-place Allreduce defect; the wrapper handles its size-one identity case
without relying on that defective operation.

The internal adapter currently advertises 26 entries, including fixes or
emulation for Allreduce, Allgather, Allgatherv, Wait, Waitsome, Testsome, Rsend,
and Irsend. Ready sends delegate to the corresponding standard sends; callers
must have posted the matching receive before starting either ready send.
Ssend/Issend remain unavailable because tMPI's standard sends can complete
through eager buffering before a receive starts. Bsend remains unavailable
because it needs the separate attached-buffer completion contract.
Successful Wait clears the wrapper request even though upstream leaves the
released pointer unchanged. Upstream Waitsome/Testsome index and status packing is bypassed
through Waitany/Testany. Testall remains unavailable because the pinned upstream
implementation modifies requests when its completion flag is false.

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
      src/thread_mpi

Internal launch semantics are exercised by
src/mpiwrap/check-thread-mpi-cli.sh src/lib/plumed-static after a configured
thread-MPI build. The tests cover 1, 2, and 4 ranks, callback error handling,
provider lifecycle, raw communicator byte-prefix handling, native/legacy
runner selection, and CHDIR rejection while thread-MPI is active.

## Maintaining GROMACS compatibility

Injected GROMACS thread-MPI support starts at GROMACS 2026. The embedded
runtime remains pinned to stable 2026.4 in the separately licensed thread_mpi
module. Earlier versions lacked Type_free, retaining derived datatype storage
until finalization; 2025 also corrected Error_string's length-output type.

Audit historical headers from a local full GROMACS clone without network access:

    python3 src/mpiwrap/check-gromacs-history.py /path/to/gromacs --check

To review a future release, rerun with --through vYYYY.N without --check,
inspect the resulting gromacs-history.json diff, and update availability rules
only after reviewing the corresponding implementations. The inventory records
47 stable releases from 2020 through 2026.4; declarations alone do not establish
standard-compatible semantics. Updating the embedded importer does not replace
this review.

The audited development-main snapshot 73d6be5d778e adds UINT64_T and makes Wait
clear completed requests, relative to 2026.4. It introduces no further public
functions or signature changes. This is a recorded snapshot, not a promise
about a future 2027 release. Keep the 2026.4 request-clearing workaround and
omit newer constants from hosts that do not provide them.

Native MPI availability uses MPI_VERSION/MPI_SUBVERSION rather than testing
whether a function name is a macro. The MPI-2 functions Comm_f2c, Finalized,
Comm_get_errhandler and Comm_set_errhandler are omitted for older implementations;
MPI_IN_PLACE also requires MPI-2. Optional long-long predefined types are
published conservatively from MPI-2.1, when required by the standard. The same
guards protect implementations and table rows. Synthetic MPI-1.1 probes verify
that older headers need not declare omitted functions or constants.

A configured-capacity installation check uses a separate configured checkout:

    ./configure --disable-mpi --with-mpiwrap-handle-capacity=24 \
      --with-mpiwrap-status-capacity=192
    MPICH_CXX=/usr/bin/clang++ OMPI_CXX=/usr/bin/clang++ \
      bash src/mpiwrap/check-capacity-install.sh /path/to/mpich/mpicxx \
      /path/to/openmpi/mpicxx

It runs the public install-headers recipe and compiles installed headers
without capacity defines. This checks propagation; the build/runtime matrix
above remains separate validation.
