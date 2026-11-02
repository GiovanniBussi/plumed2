# MPI wrapper implementation record

This file preserves the detailed step-by-step evidence from implementation. The current module contract and supported configuration are documented in src/mpiwrap/README.md.

## Latest configuration and layout checkpoint

The old command-line crash was caused by compiling Communicator with different handle types across translation units: a tools-local __PLUMED_MPIWRAP define selected the surrogate class while other modules used the native type. Configure now emits __PLUMED_MPIWRAP globally for thread-MPI builds, and the local Makefile define was removed. A full clean thread-only rebuild then passed the internal CLI tests; this is the evidence for the fix.

Configuration commit c97093f34 passed a clean thread-only build, internal 1/2/4-rank CLI tests, native MPICH configuration/build, a fully serial build, explicit provider-conflict rejection, and capability/config checks. Native MPI process startup remains blocked by sandbox OFI/socket permissions.

The module is now flat: generated inputs, outputs, runtime sources/fragments, and shell/Python checks are all directly in src/mpiwrap. Test/probe translation units use .cpp.in or .c.in so the module wildcard build does not compile them. The README was rewritten as current documentation instead of a concatenated proposal/progress log.

Flat-layout verification passed: python3 src/mpiwrap/generate.py --check; sh src/mpiwrap/check-provider-registry.sh; bash src/mpiwrap/check-kernel-lifecycle.sh; bash src/mpiwrap/check-regtest-runner.sh; the MPICH/Open MPI/pinned-tMPI check-generated.sh matrix; and sh src/mpiwrap/probe-layouts.sh. A clean ./configure --disable-mpi && make clean && make -C src/lib -j4 succeeded. src/lib/plumed-static --no-mpi --has-thread-mpi exited 0, and check-thread-mpi-cli.sh passed at 1/2/4 ranks with CHDIR rejection. The tree remains configured and built for thread-MPI; no further clean/build was run while primary review tests were in progress.

# MPI wrapper boundary

Provider-mode MPI handle wrappers keep their native-handle bytes at offset zero
inside zero-initialized storage. This is a deliberate compatibility contract:
existing PLUMED callers can continue passing `&Communicator::Get_comm()` to a
nested kernel without changing their source or constructing an export buffer.
The receiving compatible kernel copies exactly the provider-reported native
handle size from that address into an aligned native handle object. The wrapper
storage capacity is validated before use; equal bytes alone do not make a
communicator valid across different MPI runtimes.

Native-MPI builds retain native MPI handle types and direct calls. Explicit MPI
provider injection is an opt-in API for callers that need to share an existing
MPI runtime with a nested PLUMED kernel.


---

## ABI design proposal

# MPI provider ABI and injection proposal

This is a stage-one review proposal. It is not installed, generated into the public header, or frozen for implementation. The C-compatible declarations live in [provider-abi-proposal.h](provider-abi-proposal.h); `probe-abi.cpp` checks its layout on the current compiler. The kernel symbol table and provider descriptor are independently versioned protocols.

## Version boundaries and entry formats

The provider protocol starts with `plumed_mpiw_provider_descriptor`. Its fixed header is size-tagged and has an ABI major/minor. A provider with a larger minor may append fields; a consumer reads only fields covered by `struct_size`. A major mismatch, a truncated required prefix, unsupported representation sizes, or an unknown required capability rejects installation. The `reserved` fields must be zero when supplied and are reserved for compatible extension. Capabilities describe advertised provider features; their absence never implies a function is present.

Functions are a counted array keyed by unique NUL-terminated names. Each entry carries `signature_id` and `signature_version`; IDs are assigned by the generated manifest, never by array position. The entry record size is frozen for one provider ABI major: `struct_size` must equal the known size, so a reader may index the counted array using its fixed stride. New optional functions and metadata are added as named entries without changing the descriptor layout; a new fixed descriptor field requires a provider ABI minor/major update. The fixed header layout, including its four capability words and six reserved words, is only a measured proposal and may be reduced after owner review; it is not a requirement. This avoids treating a struct-size field as if it enabled variable-stride traversal. An entry's pointer has the generic C function-pointer carrier type `void (*)(void)`. For a recognized name, the installer checks the exact signature ID/version, converts the carrier back to that exact generated callback type, then stores the typed value. It never invokes the carrier through the generic type or converts it through a data pointer. Duplicate names and recognized signature mismatches reject the whole descriptor before publication. Unknown optional names are ignored. Missing optional functions remain unavailable and are handled by generated fallbacks or a PLUMED exception.

Provider callbacks use generated C transport signatures shared by the kernel and adapter. They do not expose provider-specific `MPI_Comm` typedefs across the descriptor boundary. The review header assigns `PLUMED_MPIW_SIG_COMM_RANK` (`0x00010001`, version 1), `PLUMED_MPIW_SIG_WAIT` (`0x00010002`, version 1), and `PLUMED_MPIW_SIG_HANDLE_VALUE` (`0x00010003`, version 1). A `Comm_rank` entry has the exact callback type `int32_t (*)(plumed_mpiw_handle_storage comm_storage, int32_t *rank)`; the adapter reconstructs a local native `MPI_Comm` with `memcpy`, calls the native routine, and returns scalar output. A `Wait` entry accepts `plumed_mpiw_handle_storage *request_storage` plus a pointer to `plumed_mpiw_status`; the adapter converts the request through an aligned native temporary, calls MPI, copies the updated request and raw status back, and synchronizes source/tag/error fields. Signature IDs distinguish handle kinds even though each uses the same storage integer type. The descriptor reports each native handle size and the configured wrapper handle size so the adapter can reject mismatched capacities before any object crosses the boundary.

Typed callback transport uses a C-compatible record rather than casting between frontend and native callback pointer types. `MPI_Op_create` receives `plumed_mpiw_user_callback { function, invoke }`: `function` is a generic function-pointer carrier for the original frontend callback, and `invoke` is an exact C-callable hook. The hook restores the original callback to the generated frontend type, builds a wrapper `MPI_Datatype` value from fixed transport storage, and invokes it with the typed signature. The native adapter owns a bounded set of typed MPI user-function trampoline slots because MPI provides no callback context argument; each slot retains the record until `MPI_Op_free` and in-flight calls finish. `MPI_Op_create` provider transport signature version is 2. Callback exceptions are fatal at the callback boundary and never unwind through MPI.

`MPI_Comm_create_errhandler` uses the corresponding `plumed_mpiw_comm_errhandler_callback` record and a typed native trampoline slot, with lifetime through handler release and completion of in-flight callbacks. Its frontend hook calls the variadic frontend callback using the standardized communicator and error-code arguments; it does not generically forward implementation-specific trailing varargs. A provider may expose this operation only when those standard arguments are sufficient for its supported callback contract. Its provider transport signature version is 2. The custom thread-MPI error-handler operation remains unavailable under the existing policy.

The validator recognizes required UTF-8 metadata entries `callback:MPI_User_function` and `callback:MPI_Comm_errhandler_function`, with values `mpi5-c-v1` or `unavailable` (the user-function callback may not be marked unavailable when `MPI_Op_create` is present). A callback-taking function entry is accepted only when its matching ABI metadata declares `mpi5-c-v1`; an unavailable variant cannot accompany a present function entry. Unknown optional metadata remains ignorable and unknown required metadata is rejected. Thread-MPI may report `MPI_Fint` as unavailable with size/signedness both zero; `Comm_f2c` remains unavailable in provider mode.

Use a reserved `value:` name prefix for provider-dependent predefined values, such as `value:MPI_COMM_WORLD` and `value:MPI_INT`. Their generated callback type writes the provider's native handle bytes into caller-owned `plumed_mpiw_handle_storage` using `PLUMED_MPIW_SIG_HANDLE_VALUE`; it does not return a provider's native typedef by value. Special pointer sentinels use their own exact accessor signature, such as `const void *(*)(void)`. Entries are called lazily after selection; no provider-dependent global initialization occurs. Capability inquiry must not call a missing entry.

Metadata is separately extensible through the counted `metadata` array. Entries have stable names, a value type, flags, explicit byte length, and a pointer to C-interoperable bytes. Proposed types are fixed-width signed/unsigned integers, UTF-8 strings including the final NUL, and versioned opaque byte records. The installer copies recognized values before returning. Unknown optional entries are ignored; unknown required entries fail installation. Optional runtime-specific identity records and an opaque host token may be named metadata. Do not mirror `runtime_kind`, provider name/version, or the mandatory representation facts that already have fixed descriptor fields.

For metadata ownership, choose one authority per value: fixed descriptor fields are authoritative for the representation facts required before entry parsing (handle sizes, status size/alignment, C `int`, `MPI_Fint`, wrapper capacities, runtime kind); named metadata carries descriptive identity/version strings and optional runtime-specific records only. Do not duplicate any fixed value in metadata. If a later ABI revision permits duplicate aliases, installation must require exact byte-for-byte agreement and reject disagreement. Optional named metadata additions do not change ABI layout or version; a new fixed descriptor field does.

The callback-based error reporter has the shape `int (*)(void *context, int32_t code, const char *function, const char *message)`. It is an optional host-side reporting channel and must not throw or unwind. The normal C++ frontend reports a missing operation as a PLUMED exception containing the MPI function and provider identity. No exception crosses provider callbacks or existing C ABI boundaries.

## Const-correct API signatures and legacy native headers

The frontend declarations, provider callback signatures, and authoritative generated manifest must use MPI-5 const-correct types. The candidate signatures transcribed from the old draft are not authoritative; audit and update their const qualifiers against MPI-5 before generation. Do not weaken the frontend or provider-facing API to satisfy older MPI headers.

Only the native C-compatible adapter that calls the real MPI implementation may explicitly remove `const` from MPI-5 input-only pointer arguments when an older native declaration requires mutable pointers. Generate the cast to the matching native pointer type locally at that call site. Keep the frontend and descriptor signatures const-correct, preserve input-only semantics, and keep output/in-out pointers mutable with their normal copy-back behavior. This exception does not permit unrelated type casts, handle-marshalling bypasses, or incompatible callback/function-pointer calls. If an implementation writes through an MPI-5 input-only argument, diagnose that incompatibility; casting does not make it safe. The shared adapter used by injection, `makempi`, and on-demand compilation must follow the same rule. Later implementation verification should compile against both modern headers and a small legacy-signature mock; no such compatibility adapter or mock test is implemented in this stage-one checkpoint.

## Proposed concrete C declarations

The checked header proposes these fields: `struct_size`, independent provider `abi_major/abi_minor`, runtime kind and flags, fixed-width capability words, native handle sizes and status alignment, native C `int` size/signedness and `MPI_Fint` size/signedness, wrapper handle/status capacities, provider identity strings, counted function and metadata arrays, runtime identity anchors for `Comm_rank` and `Initialized`, and an optional host runtime token. The dimensions of capability/reserved arrays and which identity anchors are fixed versus required named metadata remain design proposals; the owner may choose a smaller fixed header and make nonessential compatibility data named required metadata. Each callback receives or fills wrapper transport storage (`plumed_mpiw_handle_storage` handles and `plumed_mpiw_status`), while only adapter code sees native MPI declarations. The signedness field is fixed-width 0/1 and rejected if inconsistent with supported conversion rules. Native C `int` metadata is mandatory for counts, ranks, and status fields; `MPI_Fint` metadata is descriptive while provider-mode `Comm_f2c` remains unavailable. Its C layout and offsets are compiler dependent in the ordinary platform ABI sense; both sides compile against the same installed declaration and verify layout using `probe-abi.cpp`.

Handle storage is a distinct standard-layout, trivially copyable wrapper type for each MPI handle kind, containing one configured unsigned integer member (default `uintptr_t`). Zero the entire integer, `memcpy` native handle bytes into it, and reconstruct a correctly typed native local by the reverse copy. The stored integer is opaque bytes, not a fabricated pointer or numeric conversion. Generate explicit equality/inequality operators per handle kind with no cross-kind conversion. Before publishing callbacks, reject every native handle larger than the selected integer storage. Native equality must match equality of the copied representation for supported providers; noncanonical padding must be rejected or normalized in a reviewed adapter. Never use `MPI_Comm_compare` as handle identity. Native MPI builds keep their native MPI typedefs and direct calls unchanged.

Storage is configured once per PLUMED build, with `uintptr_t` as the default unsigned handle integer and a separate 128-byte default raw-status capacity. The generated configuration header, public provider header, native adapter, and any runtime-compiled adapter must agree on both values; the descriptor reports actual native sizes independently and cannot override wrapper capacities. The `plumed_mpiw_status` layout has `int32_t source`, `tag`, and `error` at offsets 0, 4, and 8, then `uint32_t raw_size` at offset 12 and `PLUMED_MPIW_STATUS_CAPACITY` provider-native status bytes at offset 16. It preserves native raw status for subsequent status inquiries while exposing portable fields; it never aliases a native `MPI_Status` in the frontend. Native `int` and `MPI_Fint` sizes/signedness are explicit descriptor facts, with checked conversion/range rules required for every use.

Runtime identity is the structured compatibility fingerprint: runtime kind, native representation sizes/alignments, required callback-ABI variant metadata, and a small set of native runtime function-pointer anchors. Compare anchors as function pointers, never as integers or data pointers. These anchors identify the underlying runtime binding and are not called using the generic transport type. Providers whose symbol interposition or isolation makes anchors ambiguous must supply the same explicit host runtime token to adapters for one runtime instance. Names and version strings alone do not establish identity. Re-injection of an identical descriptor is idempotent; a conflicting or ambiguous identity is rejected. The kernel owns or retains each dynamically loaded provider-library reference after successful installation and until the kernel handle and all objects created from that kernel are released.

The descriptor ABI version is not the PLUMED kernel symbol-table version. The kernel table version tells a wrapper which kernel entry points exist. The provider ABI version tells that kernel whether the injected MPI adapter's descriptor and object representations are compatible. The wrapper must check the kernel table version before reading appended injection entries. The planned injection entry points are still names for review, not approved public declarations.

## Ownership and lifetime

| Field or object | During descriptor validation | After successful install | Release condition |
|---|---|---|---|
| Descriptor and counted arrays | Borrowed; must remain readable until install returns | Not retained; recognized fields copied | Caller may release immediately after install returns |
| Entry names and provider identity strings | Borrowed, NUL-terminated | Recognized strings copied into kernel-owned storage | Provider may release after install returns |
| Metadata value bytes | Borrowed for `value_size` bytes | Recognized values copied; unknown optional values discarded | Provider may release after install returns |
| Function pointer values | Borrowed as generic function-pointer carriers | Converted and copied into exact typed kernel callback slots | Provider code/library must remain loaded while installed callbacks can be called |
| Identity anchors | Compared as function pointer values, not invoked | Copied as pointer values into kernel-owned identity record | Their code must remain mapped for as long as reinjection identity checks use them |
| Host runtime token | Opaque identity value; never dereferenced | Copied and compared for equality | Host must keep its identity token stable for provider lifetime |
| Native MPI runtime and communicator objects | Host-owned | Borrowed by PLUMED; PLUMED duplicates communicators only where documented | Host must keep the runtime alive until dependent PLUMED objects/calls finish; PLUMED never finalizes host MPI |
| Internal thread-MPI launch | Not applicable until explicit launch | Launcher owns worker lifetime and finalization | Exactly one launch per kernel/runtime; no use of finalized runtime objects afterward |

Validation is atomic: copy and validate into temporary kernel-owned state, then publish all recognized callbacks and metadata together. On any error, discard the temporary state and expose no partial installation. A selected provider does not imply an initialized MPI runtime. Before internal launch, callers keep the existing local one-rank serial fallback and do not query null native communicators.

## Load, inject, then create examples

The following is the proposed concrete lifecycle surface for primary review. C functions return status codes, fill output handles only on success, and never throw; an optional non-throwing error callback receives diagnostics. The C++ convenience class mirrors these operations with RAII and translates failures to PLUMED exceptions. A kernel handle binds to one exact kernel instance, so injection cannot target a different `PLUMED_KERNEL` selection.

```c
typedef struct plumed_kernel_handle_impl *plumed_kernel_handle;
typedef int32_t plumed_mpiw_status_code;

plumed_mpiw_status_code plumed_kernel_open_static(
    plumed_kernel_handle *out, plumed_mpiw_error_fn report, void *context);
plumed_mpiw_status_code plumed_kernel_open_dlopen(
    const char *path, int mode, plumed_kernel_handle *out,
    plumed_mpiw_error_fn report, void *context);
plumed_mpiw_status_code plumed_kernel_open_selected(
    plumed_kernel_handle *out, plumed_mpiw_error_fn report, void *context);
plumed_mpiw_status_code plumed_kernel_inject_mpi(
    plumed_kernel_handle kernel,
    const plumed_mpiw_provider_descriptor *provider,
    plumed_mpiw_error_fn report, void *context);
plumed_mpiw_status_code plumed_kernel_load_mpi_wrapper(
    plumed_kernel_handle kernel, const char *provider_path,
    plumed_mpiw_error_fn report, void *context);
plumed_mpiw_status_code plumed_kernel_create(
    plumed_kernel_handle kernel, plumed *out,
    plumed_mpiw_error_fn report, void *context);
void plumed_kernel_release(plumed_kernel_handle kernel);
```

`open_static` uses linked/static kernel binding; `open_dlopen` loads one explicit kernel path; `open_selected` follows existing `PLUMED_KERNEL` selection. `inject_mpi` installs a caller-owned descriptor and is valid only before the first object is created. `load_mpi_wrapper` resolves the documented provider factory from the given library and retains that library in the kernel handle only after successful validation. Both operations fail if any object already exists for that kernel. `create` constructs an object from the exact bound kernel. Kernel handles are reference-counted: an object keeps its kernel and provider library alive after the caller releases its handle; library teardown occurs after both handle owners and all dependent objects are gone. The error callback is optional and cannot throw or unwind through C.

The C++ convenience is `PLMD::Kernel`, with static factories `Kernel::openStatic()`, `Kernel::open(path, mode)`, and `Kernel::openSelected()`, methods `injectMPI(descriptor)`, `loadMPIWrapper(path)`, and `create()`, and an RAII destructor. It throws on failure and owns the corresponding C handle. Returned C `plumed` objects still follow the ordinary explicit `plumed_finalize(object)` lifecycle.

The C wrapper now exposes the `plumed_kernel_*` open, inject, load, create, and release functions, and appends `install_mpi_provider` at symbol-table version 5. A provider library exports exactly one factory named `plumed_mpiw_provider_factory` with the `plumed_mpiw_factory_fn` signature. `plumed_kernel_load_mpi_wrapper` transfers its `dlopen` reference into the selected kernel registry only after successful descriptor validation; the kernel registry releases that reference when the kernel library unloads. Objects created through `plumed_kernel_create` reopen the exact selected path and verify that their symbol-table address matches the injected kernel. The old `plumed_implementation` layout remains unchanged. A focused mock-DSO test covers provider loading, exact-kernel object creation, object-owned kernel lifetime after handle release, provider release after object finalization, and rejection of a v4 kernel. Static and selected-path behavior are compile-checked but still need integration tests against built PLUMED kernels.

Static/link-time binding:

```cpp
plumed_kernel_handle kernel = nullptr;
if (plumed_kernel_open_static(&kernel, report_error, context) != PLUMED_MPIW_OK) fail();
plumed_mpiw_provider_descriptor provider = make_mpi_provider_descriptor();
if (plumed_kernel_inject_mpi(kernel, &provider, report_error, context) != PLUMED_MPIW_OK) fail();
plumed object;
if (plumed_kernel_create(kernel, &object, report_error, context) != PLUMED_MPIW_OK) fail();
/* Use object while provider callbacks and its kernel are retained. */
plumed_finalize(object);
plumed_kernel_release(kernel);
```

Dynamic kernel and explicit provider library:

```cpp
plumed_kernel_handle kernel = nullptr;
if (plumed_kernel_open_dlopen("/absolute/path/libplumedKernel.dylib", mode,
                              &kernel, report_error, context) != PLUMED_MPIW_OK) fail();
if (plumed_kernel_load_mpi_wrapper(kernel, "/absolute/path/libplumed-mpi.dylib",
                                   report_error, context) != PLUMED_MPIW_OK) fail();
plumed object;
if (plumed_kernel_create(kernel, &object, report_error, context) != PLUMED_MPIW_OK) fail();
/* Use object while provider callbacks and its kernel are retained. */
plumed_finalize(object);
plumed_kernel_release(kernel);
```

The kernel handle owns its loaded kernel reference; each object keeps the same kernel/provider libraries alive until its finalization. Failed injection leaves the handle retryable only if no object has been created and no partial provider state was published. C++ examples use the same order and RAII releases. Runtime compiler selection and `makempi` are deferred stages; when implemented they use this same descriptor ABI and retained-library lifetime.

## Decisions resolved and remaining review items

The primary agent has resolved routine implementation choices: frozen function/metadata entry layouts per provider ABI major; optional named metadata extensions without fixed-layout changes; no mirrored contradictory facts; `uintptr_t` wrapper storage and 128-byte raw status defaults with one generated configuration used by all sides; transport scalars fixed-width with native sizes and checked conversions; native MPI direct behavior unchanged; kernel-owned/reference-counted provider-library lifetime; C nothrow status/error-callback functions plus throwing C++ convenience methods.

The primary agent has accepted these lifecycle names and signatures as the working implementation contract, subject to consistency with existing wrapper naming and tests. C object finalization is explicit with `plumed_finalize(object)`; it releases that object's references, while `plumed_kernel_release(kernel)` releases only the caller's kernel-handle reference. A live object keeps the exact selected kernel and provider library references alive. Releasing the final kernel-handle reference cannot unload callbacks while an object can still call them.

Injection is a per-kernel-instance state transition and must happen before the first object exists for that loaded kernel, even when the caller holds multiple `plumed_kernel_handle` aliases to it. Handles that identify the same loaded kernel share one object count, provider selection and reference count; no alias may bypass the first-object rule or unload code used through another alias. Atomic validation failure publishes no provider state and leaves the kernel retryable only while no object has been created. Same-provider reinjection compares runtime-native anchors (or the required shared host token) and compatibility facts, never adapter-local slot addresses or descriptor addresses.

Before an internal thread-MPI launch, provider selection does not initialize MPI. The Communicator keeps an explicit local one-rank fallback state and does not query tMPI rank, size, or self-communicator accessors with null native communicator state. Only runtime-active operations materialize a real self communicator. Static `Communicator::StatusIgnore` storage does not select a provider; status-ignore translation occurs at a call boundary after provider selection. These rules preserve early local operations without fabricating a native handle.

The support matrix and companion argument metadata must be the authoritative manifest. Every retained function needs an exact MPI-5 prototype, per-argument direction and type conversion, array lengths and valid-on-return cases, callback ABI/lifetime, provider behavior, fallback prerequisites, and test identifier. Deprecated and removed MPI-1 spellings are retained only in an explicit exclusion inventory. The provider support matrix must distinguish tMPI declarations/macros from linked/semantic verification. Do not feed the earlier draft-transcribed signatures to a generator.


---

## Implementation progress and verification

# MPI wrapper implementation progress

## Stage 1 checkpoint

The stage-1 specification and evidence are ready for primary review in the dedicated local clone `/private/tmp/mpiw-IozTkN/plumed2`, branch `mpiw`. The working branch starts at PLUMED commit `5fe7ea4848dd22f239400c21debfc554d7f1fe28`. Earlier local commits are `f07086fe53c774d007e51e833f743ee61b13d0bf` (initial evidence) and `c8ce61f86006d6b7a853a357518fc35af94d2aee` (const policy). The stage-1 closeout changes are being committed separately. Plan input at the time of this checkpoint has SHA256 `4b08df4e6e96d2c4c84076baec3eff3e1469e3dcd459741c5d18b2e5ac816729`; no plan or policy file was changed.

The first inventory scanned current `src/` and `regtest/` MPI call sites before expanding the candidate union. The support matrix now has 231 records: 133 function rows, 83 constants, 14 types, and one communicator-import helper. There are 118 retained function signatures (22 current PLUMED calls, 91 MPI-1 union candidates, and five additional stable thread-MPI surface entries), 15 explicitly excluded legacy function rows (10 removed and five deprecated), and 80 retained constants. Three Fortran status-array index names (`MPI_SOURCE`, `MPI_TAG`, `MPI_ERROR`) are explicitly excluded from the C API; C uses `MPI_Status` fields. The old draft is an inventory/reference source only and is not treated as authoritative.

Machine-readable marshalling is in `argument-metadata.json` (118 functions and 473 argument records), `constant-metadata.json` (80 constants), and `type-metadata.json` (14 types). `validate-manifest.py` checks coverage and rejects missing pointer extents, output-array validity rules, callback bridge/lifetime rules, provider routes, or value classifications. Per-function metadata records direction, array extent, sentinels, partially-defined results, callback ABI/lifetime, provider route, fallback prerequisites, and semantic test ID. Unsupported provider operations remain explicit frontend entries whose missing callback must throw. The two collective fallback proposals are constrained to intracommunicators and known contiguous datatype extents; they preserve in-place contributions and displacement gaps. Arbitrary derived types, unknown datatype extents, and intercommunicators fail before communication.

The pinned tMPI surface comes from GROMACS `v2026.4`, commit `6c69f720aa7378fddfee67f99d51aff21ccfb0a2`. The declaration/macro inventory is `thread-mpi-api.tsv`. Standard-name macros and namespaced tMPI declarations are distinguished: a missing macro is not mistaken for a missing declaration. Stable tMPI lacks ordinary `MPI_Init` compatibility (the declaration adds a rank-start callback), has no standard `MPI_Allgather`/`MPI_Allgatherv`, and its two-argument error-handler callback cannot be cast to MPI's variadic callback type. Custom error-handler registration is explicitly unavailable under the agreed policy. Runtime symbol exports and communication semantics will be exercised after the stage-2 import; the stage-1 declarations do not claim runtime verification.

The ABI proposal is in `abi-design.md` and the C layout in `provider-abi-proposal.h`. It uses fixed entry layouts per provider ABI major, named metadata extension, configured `uintptr_t` handle storage and a 128-byte raw status default, typed function-pointer restoration, explicit native integer sizes/signedness, native-anchor identity, and kernel-retained provider libraries. It includes the accepted C nothrow load/inject/create/release names plus a `PLMD::Kernel` RAII convenience. `plumed_finalize(object)` drops the object's references; releasing a kernel handle does not unload a provider while any object still depends on it. Injection is per loaded kernel and precedes its first object. Prelaunch local fallback state is explicit: no tMPI rank/self call before launch, and static `Communicator::StatusIgnore` cannot force provider selection.

## Commands and actual results

These commands were run in the clone:

- `python3 src/mpiwrap/validate-manifest.py` — passed: 118 function rows, 473 argument records, 80 constants, 14 types.
- `python3 src/mpiwrap/check-mpi5-prototypes.py /Users/bussi/miniconda3/envs/mpich/bin/mpicxx /Users/bussi/miniconda3/envs/openmpi/bin/mpicxx` — passed: 118 function declarations and 91 retained types/constants compile and execute on both installed headers. `MPI_User_function` and `MPI_Comm_errhandler_function` are checked against their exact typedef signatures. This is header type evidence, not proof that either installation is a standardized MPI-5 runtime.
- `sh src/mpiwrap/probe-layouts.sh` — passed. MPICH 5.0.1 reports MPI_VERSION 5.0, 4-byte integer handles, 20-byte/4-aligned status, `int` 4-byte signed, `MPI_Aint` 8-byte signed, `MPI_Fint` 4-byte signed, status field offsets 8/12/16. Open MPI 5.0.10 reports MPI_VERSION 3.1, 8-byte pointer handles, 24-byte/8-aligned status, `int` 4-byte signed, `MPI_Aint` 8-byte signed, `MPI_Fint` 4-byte signed, status field offsets 0/4/8. Both fit the local 8-byte `uintptr_t` and 128-byte raw-status proposal.
- The same layout probe compiled pinned tMPI declarations from the exact GROMACS tag: pointer handles are 8 bytes except `tMPI_Op` (4-byte enum), `tMPI_Status` is 32 bytes/8-aligned with source/tag/error offsets 0/4/8, and C `int` is 4-byte signed. It verified the incompatible `tMPI_Init`, exact `Error_string`, `Create_errhandler`, and `Errhandler` callback declarations. These are header/layout tests only, not a complete tMPI build or communication test.
- The proposal C11/C++17 layouts match. Descriptor size is 208 bytes, function entry 32, metadata entry 40, wrapper status 144, default handle storage 8, status raw capacity 128. Typed generic-function-carrier round-trip/invocation, integer/pointer handle byte equality, and descriptor entry stride checks passed.

Compiler wrappers used `MPICH_CXX=/usr/bin/clang++` and `OMPI_CXX=/usr/bin/clang++` (set by the probe). No system-wide MPI installation or network fetch was used.

## Reference review and remaining work

Read-only review references used from `mpiw_old_draft` include `src/mpiwi.c` lines 40–135, `devtools/preproc.py`, `devtools/embed.py`, and `test_thread_mpi/main.cpp`. The draft's `Allgather` uses `Gather+Bcast`; its `Allgatherv` broadcasts a dense span through `max(displ+count)`, which can overwrite holes. The new proposal uses per-root segment broadcasts, copies an explicit input into its destination segment, honors `MPI_IN_PLACE`, and leaves displacement gaps unchanged, with checked contiguous-datatype restrictions. The draft `Reduce_scatter` extent whitelist and launcher patterns remain references for later focused tests; its unchecked allocations and unsupported in-place path are not carried forward. Its `MPI_Op_create` and `MPI_Comm_create_errhandler` wrappers transport callbacks as `void (*)(void)` and cast them back to MPI callback types; the native call therefore invokes through incompatible function-pointer types, which is unsafe and needs typed trampolines. For communicator attributes, `preproc.py` creates a typed helper containing the callbacks and caller context, initializes its refcount, and passes it as `extra_state`; `mpiwi.c` interprets the matching helper layout and forwards through typed native shims. This is a real end-to-end draft mechanism, not an omitted helper. It still needs focused repair and tests: allocation failure handling, incrementing references only when the callback requests a copy and that copy succeeds, thread-safe reference tracking if MPI calls can be concurrent, and dropping the keyval's owner reference on keyval free. The manifest requires exact callback-type restoration, scoped native-handle conversion, callback-code retention, and no exception escape. Real-MPI op/error callback support needs the step-4 typed bridge; the separately owner-approved thread-MPI custom-error-handler exclusion remains. Do not import the draft wholesale.

No MPI communication test, full stable tMPI source build/export inspection, generator, frontend implementation, or PLUMED behavior change has happened yet. Stage 2 must test the pinned imported source and symbol isolation. Steps 3–5 remain authorized after the primary reviews this checkpoint; steps 6–8 remain outside the current assignment.

## Stage 2 checkpoint: pinned namespaced thread-MPI import

The standalone module in `src/thread_mpi/` imports exactly the C++ sources and headers listed by GROMACS v2026.4's thread-MPI source set, plus its license. The deterministic `tools/reimport.py` reads the pinned commit using `git archive` (not the source worktree), checks that the `v2026.4` tag resolves to the pinned commit, records SHA256 digests of the untransformed inputs, and wraps imported declarations/definitions in `PLMD::thread_mpi`. `--check` regenerates in a temporary directory and compares the complete imported-file set. Imported third-party files preserve upstream formatting: `src/astyle.sh` scans only the module root and does not cover this nested imported source. The upstream import contains no local function-body edits. The module is standalone and not wired into PLUMED builds yet.

`CMakeLists.txt` builds the imported sources with the required pthread configuration. Its rank callback test runs at 1, 2, and 4 ranks, checks rank/size, ring `Sendrecv`, that every rank ran before `tMPI_Init_fn` returned, and finalized state; CTest applies a 20-second timeout. A separate symbol-isolation script compiles both the unmodified pinned host tMPI sources and the imported namespaced sources into separate libraries/translation units, links both into one executable, and launches each independently. The check passed; `nm` found no global `tMPI_*` exports in the imported library and did find the mangled `PLMD::thread_mpi::tMPI_Comm_rank` symbol.

One upstream semantic defect is confirmed and remains open for the wrapper layer: stable tMPI v2026.4 reports success for a one-rank out-of-place `tMPI_Allreduce` but leaves the receive buffer unchanged (`in=7`, initial `out=-123`, return code `0`, resulting `out=-123`). Its fast reduction path skips the copy when the communicator size is one. The stage-2 direct runtime test therefore exercises Allreduce only at sizes 2 and 4 and records this deliberate gap; stage 4 must add explicit one-rank out-of-place and `MPI_IN_PLACE` wrapper regressions and provide the compatible fallback. Do not treat direct upstream size-one Allreduce as validated.

Stage-2 validation run in the dedicated clone:

- `python3 src/thread_mpi/tools/reimport.py /Users/bussi/plumed-codex/gromacs --check` — passed against `6c69f720aa7378fddfee67f99d51aff21ccfb0a2`.
- `cmake -S src/thread_mpi -B /private/tmp/mpiw-stage2-final -DCMAKE_BUILD_TYPE=Debug && cmake --build /private/tmp/mpiw-stage2-final -j4 && ctest --test-dir /private/tmp/mpiw-stage2-final --output-on-failure` — passed 3/3 tests at 1, 2, and 4 ranks; size-one Allreduce excluded for the defect described above.
- `python3 src/thread_mpi/tools/check-symbol-isolation.py /Users/bussi/plumed-codex/gromacs` — passed; host and namespaced runtime copies both executed in one test executable.
- `nm -gU /tmp/mpiw-stage2-final/libplumed_thread_mpi.a | rg '(^| )_?tMPI_'` — no matches; namespaced symbol check found `PLMD::thread_mpi::tMPI_Comm_rank`.

Current step-2 result is a reproducible, isolated import plus rank-launch and namespace-collision evidence. General PLUMED integration, wrapper dispatch, and the known single-rank collective repair remain later steps.

The primary review identified that astyle behavior depended on tool availability and output location. The importer now preserves upstream formatting and does not invoke astyle on the imported third-party files, which are nested outside the project module-root scan. Reproducibility was checked with `--check` into the tracked output and a second import into `/private/tmp/mpiw-reimport-check-2`; imported upstream files matched byte-for-byte (`diff -qr` only reported expected non-import artifacts: CMake/tests/tools). Project astyle was separately applied to the new test code. This follow-up changes import provenance and stage-2 evidence only.

## Stage 3 checkpoint: generated frontend and provider contract

`generate.py` now renders the selected MPI-5 surface from the reviewed machine-readable manifests. It produces concrete POD wrapper handle structs (not a generic handle template), MPI-compatible public status field names, provider-facing namespaced wrappers and `mpiwrap_has_*` declarations, typed function-pointer slots restored from C-compatible provider callback typedefs, generated native/tMPI adapter entry fragments, route availability, canonical signature fingerprints, and coverage/support tables. The provider frontend alone remaps MPI names; the native branch includes the installed `mpi.h` and identity handle helpers. Native `MPI_Pcontrol` remains checked against its MPI prototype, while its variadic forwarding entry and provider pointer are omitted under the agreed unavailable policy. Native use remains direct. The generated output is reproducible with `python3 src/mpiwrap/generate.py --check`; generation applies the repository astyle binary to generated C/C++ files using the repository options.

The signature ID is a SHA256-derived function-name ID with collision rejection. Each reviewed signature version is paired with a canonical C type fingerprint that ignores parameter names and formatting. A changed ABI type fails generation until both the version and fingerprint are intentionally updated. Function-specific provider route metadata controls entry emission; absent provider pointers are not opportunistically invoked. Array of wrapper handles still require explicit packing loops in the implementation layer.

Validation in this checkpoint:

- `python3 src/mpiwrap/generate.py` — generated 16 files for 118 function records and applied astyle to generated C/C++ outputs.
- `python3 src/mpiwrap/generate.py --check` — passed deterministic output check.
- `MPICH_CXX=/usr/bin/clang++ OMPI_CXX=/usr/bin/clang++ sh src/mpiwrap/check-generated.sh /Users/bussi/miniconda3/envs/mpich/bin/mpicxx /Users/bussi/miniconda3/envs/openmpi/bin/mpicxx src/thread_mpi/include` — passed provider header, C11 callback declarations, pinned namespaced tMPI entry/symbol declarations, exact native MPI prototypes and native adapter entries against MPICH and Open MPI headers. The wrapper emits benign unused linker-flag warnings under `-fsyntax-only`.
- Explicit `rg` check confirmed `MPI_Pcontrol` is absent from generated provider pointer and native forwarding entry files.

This is a generation and compile-contract checkpoint, not runtime dispatch. The generated wrappers are declarations and the provider pointer slots are not yet connected to lifecycle, marshalling, callbacks, or collective fallbacks. Those implementation tasks remain in the subsequent authorized stages.

## Stage 4 implementation checkpoint: atomic registry validation

`provider-registry.{hpp,cpp.inc}` adds a per-kernel provider registry. It validates the fixed descriptor prefix and ABI major, runtime kind, bounded identity strings, configured handle/status/int/Fint representations, zero reserved fields, required identity anchors, table pointer/count consistency, exact function/metadata entry strides, duplicate names, generated function IDs/versions, unknown required entries, and unsupported capability bits before publishing any callback. Unknown optional function/metadata entries are ignored. Recognized generic function pointers are stored only after signature validation and are restored to the exact generated type at lookup; no data-pointer conversion or generic invocation occurs. Function lookup checks the name and signature again; `require()` reports a missing MPI name and provider identity. Installation is atomic, can be retried after validation failure, and permits same-provider reinjection only when anchors, provider identity, compatibility fields, and the complete recognized function slot set match.

`test-provider-registry.cpp.in` exercises malformed handle capacity, truncated descriptors, signature mismatch, duplicate names, unknown required entries, retry after failure, exact typed function-pointer restoration and invocation, wrong-version lookup, missing-function diagnostics, same-provider reinjection, and conflicting anchors/representation/function slots. `check-provider-registry.sh` builds with warnings as errors in a temporary directory and runs the mock test.

Commands/results:

- `./astyle/astyle --options=.astyle.options src/mpiwrap/provider-registry.hpp src/mpiwrap/provider-registry.cpp.inc src/mpiwrap/test-provider-registry.cpp` — all files reported unchanged on final formatting verification.
- `CXX=/usr/bin/clang++ sh src/mpiwrap/check-provider-registry.sh` — passed: provider registry validation and typed restoration.
- `python3 src/mpiwrap/generate.py --check` — passed: 16 generated files for 118 functions.

The registry follow-up now publishes a typed `ProviderBindings` table as part of successful installation. Generated steady-state callers can use the immutable per-registry table without name lookup or locking; descriptor lookup remains validation/test machinery. Reinjection compares recognized slot presence and signature/version/flags, not adapter thunk addresses, and retains the original published binding set. Recognized callback ABI metadata is copied and checked (`mpi5-c-v1` or `unavailable`); unknown optional metadata is ignored and unknown required metadata is rejected. The C-compatible callback records now carry an exact frontend invocation hook; generic callback carriers are restored only to that hook's known frontend type. Provider communicator import/export helpers accept raw native storage and use the validated provider-native size.

Validation after this follow-up: `python3 src/mpiwrap/generate.py --check`, `CXX=/usr/bin/clang++ sh src/mpiwrap/check-provider-registry.sh`, and the MPICH/Open MPI/tMPI header compile script all pass. This remains a descriptor-registry checkpoint, not production MPI dispatch: lazy frontend wrappers, native/tMPI adapter calls, data/status marshalling, usable callback lifecycle, fallback collectives, and real communication tests remain outstanding.

## Stage 4 runtime vertical slice: embedded tMPI

The registry no longer uses thread-local selection. `currentRegistry()` resolves the loaded kernel's single immutable provider registry, shared by all rank threads; low-level communicator import/export helpers accept an explicit registry only for focused ABI tests. The tMPI vertical slice provides a provider descriptor/factory and typed adapter entries for `MPI_Comm_rank`, `MPI_Comm_size`, `MPI_Bcast`, `MPI_Allreduce`, and `MPI_Barrier`, plus provider-mode frontend call-through and `mpiwrap_has_*` inquiries. The standalone test installs once before launching threads and then calls those wrapper functions from each rank. The selected tMPI constants are currently supplied by test-local adapter helpers; general provider constant-value metadata remains open.

The tMPI adapter repairs the confirmed one-rank `Allreduce` copy defect only for built-in contiguous datatypes whose size it knows. Out-of-place input is copied to the receive buffer; `MPI_IN_PLACE` is a no-op at size one. For unsupported/opaque tMPI datatypes the adapter returns `TMPI_ERR_TYPE` rather than guessing an extent. For sizes greater than one it delegates to stable tMPI. `Bcast`, Allreduce in-place/out-of-place, and barrier are exercised through the published registry bindings at 1, 2, and 4 ranks.

Validation after the vertical slice:

- `cmake -S src/thread_mpi -B /private/tmp/mpiw-runtime-build -DCMAKE_BUILD_TYPE=Debug && cmake --build /private/tmp/mpiw-runtime-build -j4 && ctest --test-dir /private/tmp/mpiw-runtime-build --output-on-failure` — passed 6/6: existing pinned tMPI launch at 1/2/4 and wrapper-dispatch semantics at 1/2/4.
- `python3 src/mpiwrap/generate.py --check`, `CXX=/usr/bin/clang++ sh src/mpiwrap/check-provider-registry.sh`, and the MPICH/Open MPI/pinned-tMPI compile-contract script passed at the preceding registry checkpoint.
- Native MPICH/Open MPI header compilation passes, but native MPI runtime communication could not be run in this sandbox: the available launchers fail during process-manager/socket setup or MPI initialization before reaching wrapper code. The primary agent independently confirmed MPICH `mpiexec.gforker` and singleton runs fail in OFI initialization with `Operation not permitted`, and Open MPI PRTE cannot bind. No native runtime correctness claim is made from those attempts; maintained tests must be run in a permitted environment.

This is a first functional vertical slice, not full adapter coverage. Remaining step-4/5 work includes generator-driven marshalling for the rest of the selected API, status/request array conversions, constant transport, collective fallbacks with gap/in-place coverage, callbacks with lifetime tests, production kernel lifecycle/load/injection/create/release APIs, PLUMED object integration, and usable MPICH/Open MPI end-to-end tests outside the sandbox.

The vertical slice has been extended so selected constants come through generated provider-mode accessors and named `value:<MPI_NAME>` metadata. A provider exposes typed accessor function pointers; the registry validates the generated constant name, exact accessor representation, and required/optional status, then publishes them with the rest of the immutable descriptor state. Accessors lazily copy native constant bytes into the configured wrapper representation, which allows `MPI_COMM_WORLD` to be queried after tMPI initialization rather than snapshotting its prelaunch null value. Registry publication now uses a release store and frontend bindings reads an acquire load before any slot use; this publishes the complete binding table to all rank threads.

The runtime CTest now obtains `MPI_COMM_WORLD`, `MPI_INT`, `MPI_SUM`, and `MPI_IN_PLACE` from provider metadata accessors rather than test-local constant helpers. `MPI_COMM_SELF` is also exposed by the tMPI provider. The full selected constant surface is generated, while providers can omit unsupported values and then receive a precise missing-constant exception at use.

The selected request/status communication slice now also dispatches `MPI_Irecv`, `MPI_Isend`, `MPI_Wait`, `MPI_Get_count`, and `MPI_Sendrecv` through typed registry slots. Request and status payloads are copied through wrapper storage; no native/request/status object is accessed by aliasing a different type. The `MPI_STATUS_IGNORE` provider accessor preserves tMPI's null-pointer sentinel. The runtime test performs a nonblocking predecessor/successor ring, waits with both a real status and ignored status, checks source/tag/count, and performs a blocking `Sendrecv` ring. Rank, size, and count outputs use fixed-width temporaries and explicit scalar assignment. The tMPI fallback extent map now includes `C_BOOL` and `INT64_T`; the standalone pinned build defines `SIZEOF_LONG_LONG_INT` consistently so the stable `TMPI_LONG_LONG[_INT]` entries are compiled and available.

Status-ignore sentinel policy: provider-facing `MPI_STATUS_IGNORE` is always canonical null. A native adapter must translate null to that provider's actual `MPI_STATUS_IGNORE` before invoking MPI, including when the native provider sentinel is a non-null address. Status arrays use the same per-element null/ignore semantics. Native constant import must therefore not copy a non-null native status sentinel into a provider-facing pointer. This normalization is recorded in `constant-metadata.json`; a mock-native test using a non-null sentinel remains to be added with the shared native adapter.

Validation for this request/status extension:

- `./astyle/astyle --options=.astyle.options src/mpiwrap/provider-frontend.cpp.inc src/mpiwrap/thread-mpi-adapter.cpp.inc src/thread_mpi/tests/mpiwrap-runtime.cpp` — applied to the two modified adapter sources; runtime test was unchanged.
- `cmake -S src/thread_mpi -B /private/tmp/mpiw-runtime-build -DCMAKE_BUILD_TYPE=Debug && cmake --build /private/tmp/mpiw-runtime-build -j4 && ctest --test-dir /private/tmp/mpiw-runtime-build --output-on-failure` — passed all 6 launch and wrapper tests at 1, 2, and 4 tMPI ranks.
- The generated signature manifest supplies IDs/versions for all five entries; `python3 src/mpiwrap/generate.py --check` and provider/native header checks remain required as expansion continues.

This is still a partial runtime surface. The generated frontend currently declares many more functions than the hand-written frontend defines; generating concrete dispatch or clean unavailable-throw definitions and inquiries for every retained function, and exercising the integration-critical set, remains outstanding. No completion claim is made for stage 4/5.

The provider frontend and stable internal adapter have since closed the complete current-use function slice except for the three manifest-declared unavailable/launcher-owned calls: `MPI_Init`, `MPI_Finalize`, and `MPI_Comm_f2c`. Direct tested calls now include `MPI_Initialized`, communicator dup/free/split, blocking receive, Gather, Scatter, and the previously described nonblocking/status operations. `MPI_Abort` has a direct provider entry and inquiry but is not invoked by tests because it terminates the runtime. The narrow internal Allgather and Allgatherv fallbacks use one Bcast per root segment, preserve gaps, accept in-place contributions, and use only recognized builtin contiguous datatype extents. The Allgatherv regression uses nonuniform counts and deliberate holes; both explicit and in-place variants pass at ranks 1/2/4. Other datatypes/unsupported extent cases return a tMPI type error rather than being guessed.

Stable tMPI value accessors are generated from `constant-metadata.json` for each audited mapped value. This includes the current-use datatype and operation handles and named scalar/pointer constants. `MPI_UNSIGNED_LONG` was corrected from unavailable to mapped after direct verification of `TMPI_UNSIGNED_LONG` in the pinned v2026.4 public declaration and `type.cpp`; absence of a convenience MPI-name macro does not imply absence of a stable provider symbol. The tMPI runtime checks exercise reductions for float, double, unsigned int, unsigned long, unsigned long long, and long double, and check distinct operation values, sentinels, and wildcard constants.

Latest stage-4 current-use runtime validation:

- `python3 src/mpiwrap/generate.py` — regenerated 22 artifacts for 118 functions using the reviewed manifests and project astyle.
- `cmake --build /private/tmp/mpiw-runtime-build -j4 && ctest --test-dir /private/tmp/mpiw-runtime-build --output-on-failure` — passed 6/6: direct thread-MPI launch and wrapper runtime at 1/2/4 ranks. Wrapper semantics include current-use direct calls, size-one Allreduce, communicator dup/split/free, Gather/Scatter, Allgather, Allgatherv with gaps/in-place/nonuniform counts, request/status marshalling, and scalar datatype reductions.

Lifecycle ownership, native adapter dispatch, all other selected function implementations (or clean unavailable definitions), PLUMED object integration, generated inline/declarations-last frontend architecture, callback bridges, and native runtime execution remain outstanding. MPI_Init/Finalize stay launcher-owned by design; MPI_Comm_f2c stays unavailable for the internal tMPI provider. Native MPICH/Open MPI runtime launch remains blocked before wrapper execution by sandbox process-manager initialization restrictions, as recorded above.

## Stage 5 Communicator checkpoint

`Communicator` now uses the generated wrapper types in both native and provider builds. Native MPI remains a direct path. In provider builds, an object constructed before runtime initialization keeps an explicit local-fallback state; it does not call tMPI communicator/self/rank accessors against an uninitialized runtime. The wrapper materializes `MPI_COMM_SELF` lazily when the provider reports an initialized runtime, and tracks whether its communicator is local fallback or an owned duplicate so cleanup does not free a fallback or standard communicator. `Set_comm(const TypesafePtr&)` treats the argument as a pointer to native handle bytes and imports them through the kernel registry helper, rather than reinterpreting the pointer as a wrapper object. Registry readiness is read with acquire semantics. The tMPI runtime test calls the public import/export helper on each rank without a thread-local registry scope.

The independent lifecycle API and provider-injection symbol-table extension are not implemented yet. The current PLUMED kernel table is version 4; the required additive, append-only v5 entry and exact-target open–inject–create/release API remain a step-5 prerequisite. The legacy `plumed_implementation` layout has not been changed. Provider-library retention across future PLUMED object creation also remains to be connected. This checkpoint does not claim that Communicator has been exercised through a full PLUMED kernel build.

Commands/results for this checkpoint:

- `cmake --build /private/tmp/mpiw-runtime-build -j4 && ctest --test-dir /private/tmp/mpiw-runtime-build --output-on-failure` — passed 6/6 internal launch/runtime tests at 1, 2, and 4 ranks, including public raw communicator import/export and rank-thread use.
- Four `Communicator.cpp` compile checks passed: serial and `__PLUMED_MPIWRAP` with Clang C++17; native MPICH 5.0.1 and Open MPI 5.0.10 wrappers with `MPICH_CXX=/usr/bin/clang++` and `OMPI_CXX=/usr/bin/clang++`, respectively.
- Astyle verification with repository `.astyle.options` reported unchanged for `Communicator.h`, `Communicator.cpp`, `provider-registry.hpp`, and the modified runtime test. `git diff --check` passed.
- Commit: `1994c6b4f4832e89b3f0636bb53a6f6ca8b58cf0` (`Support lazy MPI wrapper communicators`).

## Stage 5 append-only provider entry checkpoint

The loaded kernel's versioned function table is now extended from v4 to v5 by appending one C-compatible `install_mpi_provider` function pointer after the entire old prefix. The core exports a nothrow installer that calls the kernel-owned registry, reports validation/exception errors through the supplied callback, and returns a status code. The wrapper-side duplicate table declaration has the same appended function type and documents that provider descriptor ABI versioning remains independent. A small always-built module translation unit links the existing provider registry into the kernel; it has no MPI header or vendor-library dependency.

Native-MPI kernels reject this provider installer explicitly and retain the direct MPI path.

The kernel registry now records the first `PlumedMain` construction. Its marker is the first `PlumedMain` data member so the state transition happens before communicator members are constructed. Registry installation checks that marker under the same mutex used by installation and rejects late provider selection, including when an existing object was constructed through the legacy C wrapper or `PLMD::PlumedHandle`. The focused registry test covers this rejection. This does not yet solve object/provider-library reference-count ownership for the new wrapper kernel handle.

This is an entry-point checkpoint only, not the complete stage-5 lifecycle API. Public open/inject/create/release handles, old/new wrapper compatibility execution, loaded provider-library retention, injection-before-first-object enforcement across legacy and C++ construction paths, selected-kernel resolution, and full configured PLUMED builds remain outstanding. The legacy `plumed_implementation` structure is unchanged. Native and internal runtime behavior were not changed by the table extension.

Checks after the table extension:

- C11 and C++17 syntax checks passed for both ordinary wrapper inclusion and implementation-mode `Plumed.h` inclusion.
- `clang++ -std=c++17 -I src -fsyntax-only src/core/PlumedMainInitializer.cpp` passed.
- `clang++ -std=c++17 -I src -Isrc/mpiwrap -Isrc/mpiwrap/runtime -c src/mpiwrap/mpiwrap-registry.cpp -o /private/tmp/mpiwrap-registry.o` passed.
- `python3 src/mpiwrap/generate.py --check` passed: 22 generated files / 118 MPI functions.
- Internal launch/runtime CTest passed 6/6 at 1, 2, and 4 ranks.
- Repository astyle reported unchanged for `PlumedMainInitializer.h/.cpp`, `Plumed.h`, and `mpiwrap-registry.cpp`; `git diff --check` passed.
- `CXX=/usr/bin/clang++ sh src/mpiwrap/check-provider-registry.sh` passed after adding the late-injection guard. The `PlumedMain.h` header passes a C++17 syntax check; full source compilation still needs the generated `PlumedMainEnum.inc` build artifact.

## Stage 5 C lifecycle API checkpoint

`Plumed.h` now declares and implements `plumed_kernel_open_static`, `plumed_kernel_open_dlopen`, `plumed_kernel_open_selected`, `plumed_kernel_inject_mpi`, `plumed_kernel_load_mpi_wrapper`, `plumed_kernel_create`, and `plumed_kernel_release`, plus a move-only `PLMD::Kernel` C++ convenience. The loader checks the v5 table version before reading the appended field. Explicit and selected dynamic opens retain a kernel loader reference and record the resolved library path; object creation reopens that path, then checks table-address identity against the injected target. Each object owns its regular `plumed` dlopen reference, so releasing the kernel handle cannot unload the code until object finalization. The legacy `plumed_implementation` layout is unchanged.

The documented provider factory symbol is `plumed_mpiw_provider_factory`; its exact return type is represented by a named descriptor struct in generated `provider-abi.h`, avoiding an incompatible function-pointer invocation. After successful install, the kernel registry atomically retains a provider-library `dlopen` reference with validated provider state and releases it at kernel unload. A caller-supplied descriptor keeps caller-owned callback code lifetime requirements. The first `PlumedMain` member marks object construction in the kernel registry; registry install rejects selection after that point, including creation through legacy C and `PLMD::PlumedHandle` paths. Native-MPI kernels return an explicit unsupported status from the appended installer.

`tests/check-kernel-lifecycle.sh` builds mock v5/v4 kernels and a provider DSO. It passes direct descriptor injection, factory loading, same-target object creation, kernel-handle release while an object remains live, provider DSO lifetime through object finalization, and v4-kernel rejection. This is loader/wrapper contract evidence; it does not validate a complete configured PLUMED kernel build or a real MPI provider. Static and `PLUMED_KERNEL` selection compile but still need integration tests against actual PLUMED libraries. Kernel-handle pointer copying is not an alias API; callers must keep one handle and release it once. C++ RAII covers the kernel handle, while returned `plumed` objects still require `plumed_finalize`.

Commands/results after the lifecycle implementation:

- `CXX=clang++ src/mpiwrap/check-kernel-lifecycle.sh` — passed the mock-DSO sequence and old-kernel version gate.
- `CXX=/usr/bin/clang++ sh src/mpiwrap/check-provider-registry.sh` — passed descriptor validation, late-injection rejection, and exact typed pointer restoration.
- C11 and C++17 syntax checks passed for `Plumed.h` implementation mode with and without `__PLUMED_HAS_DLOPEN`; `src/wrapper/Plumed.c` and `src/wrapper/PlumedStatic.cpp` passed syntax checks with dlopen enabled.
- `src/core/PlumedMainInitializer.cpp` passed syntax checks in serial, MPICH, and Open MPI modes. MPI wrapper compilers emitted only their known unused-link-flag warnings in syntax-only mode.
- `python3 src/mpiwrap/generate.py --check` passed for 22 generated files / 118 function signatures.
- `MPICH_CXX=/usr/bin/clang++ OMPI_CXX=/usr/bin/clang++ sh src/mpiwrap/check-generated.sh /Users/bussi/miniconda3/envs/mpich/bin/mpicxx /Users/bussi/miniconda3/envs/openmpi/bin/mpicxx src/thread_mpi/include` — passed generated provider/native/tMPI declaration and signature compilation for MPICH, Open MPI, and pinned thread-MPI.
- `sh src/mpiwrap/probe-layouts.sh` — passed the C11/C++17 provider descriptor layout probe and reported the installed MPICH/Open MPI layouts already recorded above.
- `cmake --build /private/tmp/mpiw-runtime-build -j4 && ctest --test-dir /private/tmp/mpiw-runtime-build --output-on-failure` — passed all 6 internal launcher/runtime tests at 1, 2, and 4 ranks after registry changes.
- C11/C++17 wrapper implementation syntax checks passed with dlopen enabled; `src/core/PlumedMainInitializer.cpp` passed serial and native MPICH/Open MPI syntax checks. The MPI wrapper drivers report only unused linker flags in syntax-only mode.

Full PLUMED configure/build and symbol-table old/new integration tests remain required. Provider injection selection via `PLUMED_MPI_WRAPPER`, automatic internal tMPI fallback, native provider adapters, build/config reporting, launcher, `makempi`, and broad regression integration remain later work.

## Stage 5 configured-kernel integration validation

A full serial PLUMED configure/build now confirms the new always-built `mpiwrap` registry module links into the real kernel, the v5 table definition agrees with wrapper declarations, and the modified `PlumedMainInitializer.cpp`, `Communicator.cpp`, `PlumedHandle.cpp`, `Plumed.c`, and `PlumedStatic.cpp` compile together. The built kernel exports `plumed_symbol_table` and `plumed_plumedmain_install_mpi_provider`. These checks use the serial configuration; they do not establish MPI provider dispatch or runtime communication.

Commands/results:

- `./configure --disable-mpi` — passed with Clang C++17 on macOS; PLUMED configured as serial with dlopen support.
- `make -C src/lib -j4` — passed and produced `src/lib/libplumedKernel.dylib`, `src/lib/libplumed.dylib`, and `src/lib/install/libplumed.a`. Existing warnings from unrelated Lepton, molfile, and xdrfile sources were emitted.
- `nm -gU src/lib/libplumedKernel.dylib | rg '_plumed_symbol_table$|_plumed_plumedmain_install_mpi_provider$|_plumed_plumedmain_create$'` — confirmed the table, v5 install entry, and legacy create symbol are exported.
- An ephemeral C++17 host using `PLMD::Kernel::open(path, mode)` loaded the built kernel, created a `plumed` object from the same table, finalized it, and exited successfully.
- A static-link host using `PLMD::Kernel::openStatic()` linked against `src/lib/install/libplumed.a`, created/finalized an object, and exited successfully.
- A runtime-selected host using `PLMD::Kernel::openSelected()` with `PLUMED_KERNEL=/private/tmp/mpiw-IozTkN/plumed2/src/lib/libplumedKernel.dylib` created/finalized an object successfully.
- These actual-kernel tests supplement the checked-in mock lifecycle test; they are temporary probes, not committed integration tests.

## Stage 5 nested-kernel communicator boundary

Existing nested PLUMED C API callers pass `&Communicator::Get_comm()` unchanged. In provider mode, `MPI_Comm` is a trivial standard-layout wrapper whose first member is its zero-filled native-handle storage; a receiving compatible kernel copies exactly its provider-reported native `MPI_Comm` byte count from that address. This preserves existing callers and avoids introducing export buffers at each call site. Same-runtime identity remains required; a matching byte representation alone does not make a communicator valid across distinct MPI runtimes.

The internal tMPI runtime test now imports a native `TMPI_COMM_WORLD`, exports its bytes, reimports them, and compares the resulting wrapper storage on each launch size. This exercises the provider-side native-size copy and wrapper import together; it does not emulate a host/guest pair built with different real MPI vendors. The actual native MPI process-manager tests remain blocked by sandbox startup restrictions noted earlier.

Checks after this change:

- `python3 src/mpiwrap/generate.py --check` — passed all 22 generated files and 118 MPI functions.
- `cmake --build /private/tmp/mpiw-runtime-build -j4 && ctest --test-dir /private/tmp/mpiw-runtime-build --output-on-failure` — passed 6/6 launcher/runtime tests at 1, 2, and 4 ranks, including the native-byte communicator round trip.
- `make -C src/lib -j4` — passed the configured serial PLUMED build, including `Communicator.cpp` and all modified nested C API callers. The first attempt caught use of the two-argument `plumed_massert` macro; these checks now use `plumed_assert` with an export failure message.
- Repository astyle was applied with `.astyle.options` to every modified C/C++ source/header; `git diff --check` passed.

## Steps 6 and 8 internal-launch integration checkpoint

The pinned GROMACS v2026.4 thread-MPI import is now a normal always-built PLUMED module. Its translation units forward to the preserved, namespaced imported source tree; the module sets the required pthread/platform and `SIZEOF_LONG_LONG_INT` definitions. `mpiwrap` compiles the common provider frontend, native-provider registry, internal tMPI adapter, and launcher into the kernel. Ordinary serial `Communicator` paths use wrapper types while the native-MPI compile path remains direct.

The launcher exposes one coordinated `plumed --tmpi-threads N ...` run. The final versioned kernel table is v5, with both provider installation and internal launch callbacks appended after the unchanged v4 prefix; the callback exports the native `MPI_COMM_WORLD` representation for the lifetime of the rank call, and the CLI installs it on each driver command before construction. Rank callback exceptions print the rank and terminate the process rather than unwinding through tMPI. Repeated/overlapping launches are rejected. `--has-thread-mpi` is a stable capability query; native-MPI builds report false. The guard for `PLUMED ... CHDIR=...` consults provider runtime-kind metadata and rejects an active internal or injected thread-MPI runtime before changing the process directory; a selected but unlaunched provider remains unaffected.

The regtest runner keeps existing `mpiprocs` settings and native MPI behavior. For `type=driver`, a serial executable that answers `--has-thread-mpi` is launched with `--tmpi-threads` and the requested rank count. An explicit `PLUMED_MPIRUN` override and per-test `plumed_thread_mpi=no` disable that fallback; non-driver tests keep the existing unavailable-MPI result. The README documents the selection rule. No existing MPI test input/config was modified.

Validation after this integration:

- `make -C src/lib -j4` — passed the configured serial PLUMED kernel and CLI build with imported thread-MPI linked.
- `src/lib/plumed-static --has-thread-mpi` — exited 0; the same query on the serial dynamic CLI exited 0. `src/lib/plumed-static help` still lists the existing commands/options.
- From a temporary directory with a one-line `UNITS` input, `plumed-static --tmpi-threads N driver --noatoms --multi N ...` — passed for N=1, 2, and 4. A deliberate mismatch (one launched rank with `--multi 2`) failed with the expected “invalid number of processes” diagnostic.
- `cmake --build /private/tmp/mpiw-runtime-build -j4 && ctest --test-dir /private/tmp/mpiw-runtime-build --output-on-failure` — passed 10/10 internal launch/runtime tests, including 1/2/4-rank launcher callbacks and fatal callback-exception handling.
- `regtest/funnel/rt-fps/../../scripts/run` with a static serial CLI — selected internal thread-MPI with the configured one-rank count, then explicitly skipped because this configured build lacks the `funnel` module. Repeating with `PLUMED_MPIRUN=/bin/false` preserved the override and used the existing “MPI NOT INSTALLED” skip path.
- `src/mpiwrap/check-thread-mpi-cli.sh src/lib/plumed-static` — passed end-to-end CLI launches at 1/2/4 ranks, driver rank-count mismatch handling, and the two-rank CHDIR rejection/no-directory-change check.
- `src/mpiwrap/check-regtest-runner.sh` — passed mocked runner selection for internal launch with exact `mpiprocs=3`, native MPI precedence, an old executable without capability, explicit `PLUMED_MPIRUN` override, and the per-test CHDIR skip reason.
- Ordinary single-process `driver --noatoms` was exercised during CLI bring-up. No native MPI runtime claim is made: installed MPICH/Open MPI launchers remain blocked before wrapper execution by sandbox process-manager restrictions.

The imported upstream one-rank out-of-place tMPI Allreduce defect is still present in the pinned source; the wrapper/runtime path handles its identity result without calling the defective operation at size one. Real MPI multi-process communication, GROMACS-host injection smoke tests, broad native/thread-MPI regtest suites, full per-manifest unified C++ semantic coverage, and steps 7 (makempi and on-demand compiler-path loading) remain incomplete. Step 8 configuration reporting and the fully disabled mode have since been implemented and verified; see the current configuration checkpoint below. The broad step-7 loader path and broader regression integration remain pending.


## Owner review correction: Communicator boundary and lazy provider selection

`Communicator.{h,cpp}` is back to the original MPI operation flow. The class has no provider registry, active-communicator accessor, or serial fallback state. Its only boundary-specific conversion is `Set_comm(const TypesafePtr&)`, which uses the wrapper's native-pointer import helper. Existing nested calls again pass `&Get_comm()` directly. Generated wrapper handle records assert trivial/standard layout, exact storage size, and offset-zero storage. This is a deliberate compatibility contract: existing callers keep passing the address unchanged, and a compatible receiver copies exactly its provider-reported native handle size; it is not an incidental optimization. `src/mpiwrap/README.md` records this rationale.

Provider-dependent constant accessors lazily select the default embedded thread-MPI provider; explicit installation before first wrapper use takes precedence. `MPI_Initialized` does not select a provider and returns false before selection. Once thread-MPI is selected but not initialized, `MPI_COMM_SELF` is a zero-filled placeholder, and the Communicator's original initialized guard prevents MPI calls with it. Default selection occurs before the first PLUMED object is marked created; internal launch remains a separate explicit action and its existing object-before-launch rejection remains in force. `Communicator::Split` retains the original MPI call semantics; it has no wrapper-specific pre-initialization branch.

Checks at this checkpoint:

- `python3 src/mpiwrap/generate.py --check` — passed all 22 generated files.
- `sh src/mpiwrap/check-provider-registry.sh` — passed, including copying only the reported native `MPI_Comm` byte prefix from wrapper storage.
- `CXX=clang++ sh src/mpiwrap/check-kernel-lifecycle.sh` — passed; the mock v5 kernel now contains both appended table entries, and the lifecycle test exercises the v5 launcher gate.
- `cmake -S src/thread_mpi -B /private/tmp/mpiw-runtime-build -DCMAKE_BUILD_TYPE=Debug`, `cmake --build /private/tmp/mpiw-runtime-build -j4`, and `ctest --test-dir /private/tmp/mpiw-runtime-build --output-on-failure` — passed 11/11, including unlaunched default selection and thread-MPI ranks 1/2/4.
- Native `Communicator.cpp` compile checks with MPICH 5.0.1 and Open MPI 5.0.10 wrappers, provider-mode `Communicator.cpp` compilation, and the generated-interface check script passed. Real native-MPI runtime launch remains blocked by the sandbox process-manager restrictions recorded above.
- A temporary provider-mode Communicator host passed both default-selection and pre-injected construction cases, including local rank/size, no-op reduction, and pre-init constructor checks.
- `make -C src/lib -j4` compiled the serial PLUMED sources including `Communicator.cpp`. `make -C src/lib install-build` relinked install artifacts, but CLI execution is not claimed: the installed dynamic executable cannot load a kernel from its hard-coded `/private/tmp/...` build path, while static execution fails on the unavailable `/usr/local/lib/plumed` install directory. These are environment/install-path failures; they do not validate or invalidate wrapper runtime behavior.


---

## Generated support matrix

# Generated MPI wrapper coverage matrix

Generated from `support-matrix.tsv`, argument metadata, and type/constant metadata.
Provider status is a declaration/mapping classification, not a claim of tested runtime semantics.

| Kind | Name | API coverage | Test/semantic identifier |
|---|---|---|---|
| Function | `MPI_Init` | selected | `init-lifecycle` |
| Function | `MPI_Finalize` | selected | `finalize-owner` |
| Function | `MPI_Initialized` | selected | `initialized-query` |
| Function | `MPI_Comm_dup` | selected | `comm-dup` |
| Function | `MPI_Comm_free` | selected | `comm-free` |
| Function | `MPI_Comm_rank` | selected | `comm-rank` |
| Function | `MPI_Comm_size` | selected | `comm-size` |
| Function | `MPI_Comm_split` | selected | `comm-split` |
| Function | `MPI_Abort` | selected | `abort-child` |
| Function | `MPI_Bcast` | selected | `bcast-data` |
| Function | `MPI_Scatter` | selected | `scatter-root` |
| Function | `MPI_Gather` | selected | `gather-root` |
| Function | `MPI_Allreduce` | selected | `allreduce-in-place` |
| Function | `MPI_Isend` | selected | `isend-wait` |
| Function | `MPI_Wait` | selected | `wait-status` |
| Function | `MPI_Allgather` | selected | `allgather` |
| Function | `MPI_Allgatherv` | selected | `allgatherv` |
| Function | `MPI_Recv` | selected | `recv-status` |
| Function | `MPI_Barrier` | selected | `barrier` |
| Function | `MPI_Get_count` | selected | `get-count` |
| Function | `MPI_Sendrecv` | selected | `sendrecv-status` |
| Function | `MPI_Error_string` | selected | `error-string` |
| Function | `MPI_Comm_create_errhandler` | selected | `thread-mpi-error-handler-unavailable` |
| Function | `MPI_Comm_f2c` | selected | `fint-communicator-import` |
| Function | `MPI_Send` | selected | `mpi-api:MPI_Send` |
| Function | `MPI_Bsend` | selected | `mpi-api:MPI_Bsend` |
| Function | `MPI_Ssend` | selected | `mpi-api:MPI_Ssend` |
| Function | `MPI_Rsend` | selected | `mpi-api:MPI_Rsend` |
| Function | `MPI_Buffer_attach` | selected | `mpi-api:MPI_Buffer_attach` |
| Function | `MPI_Buffer_detach` | selected | `mpi-api:MPI_Buffer_detach` |
| Function | `MPI_Ibsend` | selected | `mpi-api:MPI_Ibsend` |
| Function | `MPI_Issend` | selected | `mpi-api:MPI_Issend` |
| Function | `MPI_Irsend` | selected | `mpi-api:MPI_Irsend` |
| Function | `MPI_Irecv` | selected | `mpi-api:MPI_Irecv` |
| Function | `MPI_Test` | selected | `mpi-api:MPI_Test` |
| Function | `MPI_Request_free` | selected | `mpi-api:MPI_Request_free` |
| Function | `MPI_Waitany` | selected | `mpi-api:MPI_Waitany` |
| Function | `MPI_Testany` | selected | `mpi-api:MPI_Testany` |
| Function | `MPI_Waitall` | selected | `mpi-api:MPI_Waitall` |
| Function | `MPI_Testall` | selected | `mpi-api:MPI_Testall` |
| Function | `MPI_Waitsome` | selected | `mpi-api:MPI_Waitsome` |
| Function | `MPI_Testsome` | selected | `mpi-api:MPI_Testsome` |
| Function | `MPI_Iprobe` | selected | `mpi-api:MPI_Iprobe` |
| Function | `MPI_Probe` | selected | `mpi-api:MPI_Probe` |
| Function | `MPI_Cancel` | selected | `mpi-api:MPI_Cancel` |
| Function | `MPI_Test_cancelled` | selected | `mpi-api:MPI_Test_cancelled` |
| Function | `MPI_Send_init` | selected | `mpi-api:MPI_Send_init` |
| Function | `MPI_Bsend_init` | selected | `mpi-api:MPI_Bsend_init` |
| Function | `MPI_Ssend_init` | selected | `mpi-api:MPI_Ssend_init` |
| Function | `MPI_Rsend_init` | selected | `mpi-api:MPI_Rsend_init` |
| Function | `MPI_Recv_init` | selected | `mpi-api:MPI_Recv_init` |
| Function | `MPI_Start` | selected | `mpi-api:MPI_Start` |
| Function | `MPI_Startall` | selected | `mpi-api:MPI_Startall` |
| Function | `MPI_Sendrecv_replace` | selected | `mpi-api:MPI_Sendrecv_replace` |
| Function | `MPI_Type_contiguous` | selected | `mpi-api:MPI_Type_contiguous` |
| Function | `MPI_Type_vector` | selected | `mpi-api:MPI_Type_vector` |
| Function | `MPI_Type_indexed` | selected | `mpi-api:MPI_Type_indexed` |
| Function | `MPI_Type_commit` | selected | `mpi-api:MPI_Type_commit` |
| Function | `MPI_Type_free` | selected | `mpi-api:MPI_Type_free` |
| Function | `MPI_Type_size` | selected | `mpi-api:MPI_Type_size` |
| Function | `MPI_Get_elements` | selected | `mpi-api:MPI_Get_elements` |
| Function | `MPI_Pack` | selected | `mpi-api:MPI_Pack` |
| Function | `MPI_Unpack` | selected | `mpi-api:MPI_Unpack` |
| Function | `MPI_Pack_size` | selected | `mpi-api:MPI_Pack_size` |
| Function | `MPI_Gatherv` | selected | `mpi-api:MPI_Gatherv` |
| Function | `MPI_Scatterv` | selected | `mpi-api:MPI_Scatterv` |
| Function | `MPI_Alltoall` | selected | `mpi-api:MPI_Alltoall` |
| Function | `MPI_Alltoallv` | selected | `mpi-api:MPI_Alltoallv` |
| Function | `MPI_Reduce` | selected | `mpi-api:MPI_Reduce` |
| Function | `MPI_Op_create` | selected | `mpi-api:MPI_Op_create` |
| Function | `MPI_Op_free` | selected | `mpi-api:MPI_Op_free` |
| Function | `MPI_Reduce_scatter` | selected | `mpi-api:MPI_Reduce_scatter` |
| Function | `MPI_Scan` | selected | `mpi-api:MPI_Scan` |
| Function | `MPI_Group_size` | selected | `mpi-api:MPI_Group_size` |
| Function | `MPI_Group_rank` | selected | `mpi-api:MPI_Group_rank` |
| Function | `MPI_Group_translate_ranks` | selected | `mpi-api:MPI_Group_translate_ranks` |
| Function | `MPI_Group_compare` | selected | `mpi-api:MPI_Group_compare` |
| Function | `MPI_Comm_group` | selected | `mpi-api:MPI_Comm_group` |
| Function | `MPI_Group_union` | selected | `mpi-api:MPI_Group_union` |
| Function | `MPI_Group_intersection` | selected | `mpi-api:MPI_Group_intersection` |
| Function | `MPI_Group_difference` | selected | `mpi-api:MPI_Group_difference` |
| Function | `MPI_Group_incl` | selected | `mpi-api:MPI_Group_incl` |
| Function | `MPI_Group_excl` | selected | `mpi-api:MPI_Group_excl` |
| Function | `MPI_Group_range_incl` | selected | `mpi-api:MPI_Group_range_incl` |
| Function | `MPI_Group_range_excl` | selected | `mpi-api:MPI_Group_range_excl` |
| Function | `MPI_Group_free` | selected | `mpi-api:MPI_Group_free` |
| Function | `MPI_Comm_compare` | selected | `mpi-api:MPI_Comm_compare` |
| Function | `MPI_Comm_create` | selected | `mpi-api:MPI_Comm_create` |
| Function | `MPI_Comm_test_inter` | selected | `mpi-api:MPI_Comm_test_inter` |
| Function | `MPI_Comm_remote_size` | selected | `mpi-api:MPI_Comm_remote_size` |
| Function | `MPI_Comm_remote_group` | selected | `mpi-api:MPI_Comm_remote_group` |
| Function | `MPI_Intercomm_create` | selected | `mpi-api:MPI_Intercomm_create` |
| Function | `MPI_Intercomm_merge` | selected | `mpi-api:MPI_Intercomm_merge` |
| Function | `MPI_Cart_create` | selected | `mpi-api:MPI_Cart_create` |
| Function | `MPI_Dims_create` | selected | `mpi-api:MPI_Dims_create` |
| Function | `MPI_Graph_create` | selected | `mpi-api:MPI_Graph_create` |
| Function | `MPI_Topo_test` | selected | `mpi-api:MPI_Topo_test` |
| Function | `MPI_Graphdims_get` | selected | `mpi-api:MPI_Graphdims_get` |
| Function | `MPI_Graph_get` | selected | `mpi-api:MPI_Graph_get` |
| Function | `MPI_Cartdim_get` | selected | `mpi-api:MPI_Cartdim_get` |
| Function | `MPI_Cart_get` | selected | `mpi-api:MPI_Cart_get` |
| Function | `MPI_Cart_rank` | selected | `mpi-api:MPI_Cart_rank` |
| Function | `MPI_Cart_coords` | selected | `mpi-api:MPI_Cart_coords` |
| Function | `MPI_Graph_neighbors_count` | selected | `mpi-api:MPI_Graph_neighbors_count` |
| Function | `MPI_Graph_neighbors` | selected | `mpi-api:MPI_Graph_neighbors` |
| Function | `MPI_Cart_shift` | selected | `mpi-api:MPI_Cart_shift` |
| Function | `MPI_Cart_sub` | selected | `mpi-api:MPI_Cart_sub` |
| Function | `MPI_Cart_map` | selected | `mpi-api:MPI_Cart_map` |
| Function | `MPI_Graph_map` | selected | `mpi-api:MPI_Graph_map` |
| Function | `MPI_Get_processor_name` | selected | `mpi-api:MPI_Get_processor_name` |
| Function | `MPI_Errhandler_free` | selected | `mpi-api:MPI_Errhandler_free` |
| Function | `MPI_Error_class` | selected | `mpi-api:MPI_Error_class` |
| Function | `MPI_Wtime` | selected | `mpi-api:MPI_Wtime` |
| Function | `MPI_Wtick` | selected | `mpi-api:MPI_Wtick` |
| Function | `MPI_Pcontrol` | selected | `pcontrol-variadic-policy` |
| Function | `MPI_Finalized` | selected | `finalized-query` |
| Function | `MPI_Comm_get_errhandler` | selected | `get-predefined-error-handler` |
| Function | `MPI_Comm_set_errhandler` | selected | `set-predefined-error-handler` |
| Constant | `MPI_FLOAT` | selected | `constant:MPI_FLOAT` |
| Constant | `MPI_DOUBLE` | selected | `constant:MPI_DOUBLE` |
| Constant | `MPI_INT` | selected | `constant:MPI_INT` |
| Constant | `MPI_CHAR` | selected | `constant:MPI_CHAR` |
| Constant | `MPI_UNSIGNED` | selected | `constant:MPI_UNSIGNED` |
| Constant | `MPI_UNSIGNED_LONG` | selected | `constant:MPI_UNSIGNED_LONG` |
| Constant | `MPI_UNSIGNED_LONG_LONG` | selected | `constant:MPI_UNSIGNED_LONG_LONG` |
| Constant | `MPI_LONG_DOUBLE` | selected | `constant:MPI_LONG_DOUBLE` |
| Constant | `MPI_COMM_WORLD` | selected | `constant:MPI_COMM_WORLD` |
| Constant | `MPI_COMM_SELF` | selected | `constant:MPI_COMM_SELF` |
| Constant | `MPI_COMM_NULL` | selected | `constant:MPI_COMM_NULL` |
| Constant | `MPI_IN_PLACE` | selected | `constant:MPI_IN_PLACE` |
| Constant | `MPI_SUM` | selected | `constant:MPI_SUM` |
| Constant | `MPI_PROD` | selected | `constant:MPI_PROD` |
| Constant | `MPI_MAX` | selected | `constant:MPI_MAX` |
| Constant | `MPI_MIN` | selected | `constant:MPI_MIN` |
| Constant | `MPI_STATUS_IGNORE` | selected | `constant:MPI_STATUS_IGNORE` |
| Constant | `MPI_GROUP_NULL` | selected | `constant:MPI_GROUP_NULL` |
| Constant | `MPI_GROUP_EMPTY` | selected | `constant:MPI_GROUP_EMPTY` |
| Constant | `MPI_REQUEST_NULL` | selected | `constant:MPI_REQUEST_NULL` |
| Constant | `MPI_DATATYPE_NULL` | selected | `constant:MPI_DATATYPE_NULL` |
| Constant | `MPI_OP_NULL` | selected | `constant:MPI_OP_NULL` |
| Constant | `MPI_ERRHANDLER_NULL` | selected | `constant:MPI_ERRHANDLER_NULL` |
| Constant | `MPI_ERRORS_ARE_FATAL` | selected | `constant:MPI_ERRORS_ARE_FATAL` |
| Constant | `MPI_ERRORS_RETURN` | selected | `constant:MPI_ERRORS_RETURN` |
| Constant | `MPI_BYTE` | selected | `constant:MPI_BYTE` |
| Constant | `MPI_PACKED` | selected | `constant:MPI_PACKED` |
| Constant | `MPI_SHORT` | selected | `constant:MPI_SHORT` |
| Constant | `MPI_LONG` | selected | `constant:MPI_LONG` |
| Constant | `MPI_UNSIGNED_CHAR` | selected | `constant:MPI_UNSIGNED_CHAR` |
| Constant | `MPI_UNSIGNED_SHORT` | selected | `constant:MPI_UNSIGNED_SHORT` |
| Constant | `MPI_LONG_LONG_INT` | selected | `constant:MPI_LONG_LONG_INT` |
| Constant | `MPI_LONG_LONG` | selected | `constant:MPI_LONG_LONG` |
| Constant | `MPI_FLOAT_INT` | selected | `constant:MPI_FLOAT_INT` |
| Constant | `MPI_DOUBLE_INT` | selected | `constant:MPI_DOUBLE_INT` |
| Constant | `MPI_LONG_INT` | selected | `constant:MPI_LONG_INT` |
| Constant | `MPI_2INT` | selected | `constant:MPI_2INT` |
| Constant | `MPI_SHORT_INT` | selected | `constant:MPI_SHORT_INT` |
| Constant | `MPI_LONG_DOUBLE_INT` | selected | `constant:MPI_LONG_DOUBLE_INT` |
| Constant | `MPI_MAXLOC` | selected | `constant:MPI_MAXLOC` |
| Constant | `MPI_MINLOC` | selected | `constant:MPI_MINLOC` |
| Constant | `MPI_LAND` | selected | `constant:MPI_LAND` |
| Constant | `MPI_BAND` | selected | `constant:MPI_BAND` |
| Constant | `MPI_LOR` | selected | `constant:MPI_LOR` |
| Constant | `MPI_BOR` | selected | `constant:MPI_BOR` |
| Constant | `MPI_LXOR` | selected | `constant:MPI_LXOR` |
| Constant | `MPI_BXOR` | selected | `constant:MPI_BXOR` |
| Constant | `MPI_UNDEFINED` | selected | `constant:MPI_UNDEFINED` |
| Constant | `MPI_ANY_SOURCE` | selected | `constant:MPI_ANY_SOURCE` |
| Constant | `MPI_ANY_TAG` | selected | `constant:MPI_ANY_TAG` |
| Constant | `MPI_PROC_NULL` | selected | `constant:MPI_PROC_NULL` |
| Constant | `MPI_IDENT` | selected | `constant:MPI_IDENT` |
| Constant | `MPI_CONGRUENT` | selected | `constant:MPI_CONGRUENT` |
| Constant | `MPI_SIMILAR` | selected | `constant:MPI_SIMILAR` |
| Constant | `MPI_UNEQUAL` | selected | `constant:MPI_UNEQUAL` |
| Constant | `MPI_CART` | selected | `constant:MPI_CART` |
| Constant | `MPI_GRAPH` | selected | `constant:MPI_GRAPH` |
| Constant | `MPI_MAX_PROCESSOR_NAME` | selected | `constant:MPI_MAX_PROCESSOR_NAME` |
| Constant | `MPI_MAX_ERROR_STRING` | selected | `constant:MPI_MAX_ERROR_STRING` |
| Constant | `MPI_BSEND_OVERHEAD` | selected | `constant:MPI_BSEND_OVERHEAD` |
| Constant | `MPI_SUCCESS` | selected | `constant:MPI_SUCCESS` |
| Constant | `MPI_ERR_BUFFER` | selected | `constant:MPI_ERR_BUFFER` |
| Constant | `MPI_ERR_COUNT` | selected | `constant:MPI_ERR_COUNT` |
| Constant | `MPI_ERR_TYPE` | selected | `constant:MPI_ERR_TYPE` |
| Constant | `MPI_ERR_TAG` | selected | `constant:MPI_ERR_TAG` |
| Constant | `MPI_ERR_COMM` | selected | `constant:MPI_ERR_COMM` |
| Constant | `MPI_ERR_RANK` | selected | `constant:MPI_ERR_RANK` |
| Constant | `MPI_ERR_REQUEST` | selected | `constant:MPI_ERR_REQUEST` |
| Constant | `MPI_ERR_ROOT` | selected | `constant:MPI_ERR_ROOT` |
| Constant | `MPI_ERR_GROUP` | selected | `constant:MPI_ERR_GROUP` |
| Constant | `MPI_ERR_OP` | selected | `constant:MPI_ERR_OP` |
| Constant | `MPI_ERR_TOPOLOGY` | selected | `constant:MPI_ERR_TOPOLOGY` |
| Constant | `MPI_ERR_DIMS` | selected | `constant:MPI_ERR_DIMS` |
| Constant | `MPI_ERR_ARG` | selected | `constant:MPI_ERR_ARG` |
| Constant | `MPI_ERR_UNKNOWN` | selected | `constant:MPI_ERR_UNKNOWN` |
| Constant | `MPI_ERR_TRUNCATE` | selected | `constant:MPI_ERR_TRUNCATE` |
| Constant | `MPI_ERR_OTHER` | selected | `constant:MPI_ERR_OTHER` |
| Constant | `MPI_ERR_INTERN` | selected | `constant:MPI_ERR_INTERN` |
| Constant | `MPI_ERR_PENDING` | selected | `constant:MPI_ERR_PENDING` |
| Constant | `MPI_BOTTOM` | selected | `constant:MPI_BOTTOM` |
| Type | `MPI_Comm_errhandler_function` | selected | `thread-mpi-error-handler-unavailable` |
| Type | `MPI_Comm` | selected | `comm-roundtrip` |
| Type | `MPI_Group` | selected | `group-roundtrip` |
| Type | `MPI_Datatype` | selected | `datatype-roundtrip` |
| Type | `MPI_Request` | selected | `request-roundtrip` |
| Type | `MPI_Status` | selected | `status-roundtrip` |
| Type | `MPI_Fint` | selected | `fint-communicator-import` |
| Type | `MPI_Op` | selected | `type:MPI_Op` |
| Type | `MPI_Errhandler` | selected | `type:MPI_Errhandler` |
| Type | `MPI_Aint` | selected | `type:MPI_Aint` |
| Type | `MPI_User_function` | selected | `type:MPI_User_function` |
| Type | `MPI_Handler_function` | selected | `type:MPI_Handler_function` |
| Type | `MPI_Copy_function` | selected | `type:MPI_Copy_function` |
| Type | `MPI_Delete_function` | selected | `type:MPI_Delete_function` |


## Current configuration checkpoint

Native MPI and internal thread-MPI builds are mutually exclusive. Thread-MPI defaults on when native MPI is unavailable, and `--disable-thread-mpi` permits a fully serial build. `__PLUMED_HAS_MPI` continues to describe native MPI only; `__PLUMED_HAS_THREAD_MPI` describes the internal runtime; config reports `has mpi` as their OR, plus `real_mpi` and `thread_mpi`. The runner uses `real_mpi` when available and falls back to legacy `has mpi` for older executables. Clean thread-only and fully serial builds passed; native MPICH configuration and compilation passed. Native MPI startup and multi-process tests remain blocked by sandbox OFI/socket permissions.
