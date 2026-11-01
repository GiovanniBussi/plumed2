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

The proposed C++ convenience is `PLMD::Kernel`, with static factories `Kernel::openStatic()`, `Kernel::open(path, mode)`, and `Kernel::openSelected()`, methods `injectMPI(descriptor)`, `loadMPIWrapper(path)`, and `create()`, and an RAII destructor. It throws on failure and owns the corresponding C handle. These names/signatures are proposed public contracts, not implemented declarations.

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
