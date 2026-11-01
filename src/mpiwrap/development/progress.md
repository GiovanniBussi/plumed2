# Stage 1 evidence and review checkpoint

This is the delegated stage-one inventory and ABI proposal only. No public API, build integration, vendored source, or production implementation was changed. The MPI-1 candidate names have been enumerated, but this checkpoint is not a completed/accepted inventory: the full per-entry MPI-5 C signature and provider runtime/export audit remains open, and decisions in `abi-design.md` are proposals for primary-agent and owner review.

## Revisions and read-only inputs

- Implementation clone: `/private/tmp/mpiw-IozTkN/plumed2`, branch `mpiw`, starting revision `5fe7ea4848dd22f239400c21debfc554d7f1fe28`; it was clean at assignment start. The revision is one PLUMED source change above the plan's observed `190c7b48c849712b59404f76f69c478eaa71c20c`.
- Plan: `/Users/bussi/plumed-codex/mpiw-plan.md`; content hash before delegation was `a07b52f92dadb20ef5df04afac5cce3c38a68a31564b51a9e81e791297c0aee1`. The plan and initial instructions were read-only and were not edited.
- Workspace policy: `/Users/bussi/plumed-codex/AGENTS.md`, read in full. No `AGENTS.md` exists below the clone root.
- Initial instructions: `/Users/bussi/plumed-codex/mpiw-initial-instructions.md`, read in full.
- Old draft: `/Users/bussi/plumed-codex/mpiw_old_draft` at `739938faa003da23fb0a58c43615261d50301622`; pre-existing untracked files were left untouched. Its `interface.dat` was used only as a candidate prototype transcription source, not as authoritative API scope.
- GROMACS reference worktree: `/Users/bussi/plumed-codex/gromacs` at `d2f39ef7cff523a40234faac41d378cb4d71d676`; stable tag `v2026.4` resolves to `6c69f720aa7378fddfee67f99d51aff21ccfb0a2`. The probe extracts only that pinned commit's thread-MPI headers from local Git objects.

## Inventory performed

The direct textual scan was:

```sh
rg -n 'MPI_[A-Za-z0-9_]+' src regtest --glob '*.{h,cpp,c}'
```

A supplementary comment-stripped call-token comparison was run over C/C++ source files in `src/` and `regtest/` against matrix names. It found 23 current callable MPI tokens and no uncaptured callable token. It includes `MPI_Comm_f2c`/`MPI_Fint` in the Fortran communicator path and `MPI_Sendrecv` in `regtest/pathbias/rt-conditional-path/main.cpp`. Current code's other MPI operations are in `src/main/main.cpp` and `src/tools/Communicator.cpp`; comment/documentation mentions were not counted as calls. MPI-using regtest helper files were listed in the earlier scan and are within the candidate scope.

`support-matrix.tsv` has 228 rows at this checkpoint: current-use function/constants/types and helper, the candidate MPI-1 function API, MPI-1 representation/callback types and named constants, plus explicit MPI-5-removed spellings and deprecated-but-retained MPI-1 keyval/attribute entries. Function candidates retain prototype text transcribed from the read-only old draft manifest; the source/provenance column labels this and links the corresponding MPI-1.1 chapter area. This transcript has not been exhaustively reconciled against every official MPI-5 binding. Thus names are enumerated as candidates, but exact final binding/signature and argument-conversion metadata remain incomplete for many rows. In particular, candidate signatures need the MPI-5 const audit before generation. The frontend declarations, provider-facing callbacks, and manifest must stay MPI-5 const-correct; only the native C-compatible adapter may locally cast eligible MPI-5 input-only pointers to matching mutable pointer types for old MPI headers. Preserve input-only semantics and normal output/in/out handling. This does not allow unrelated casts or callback signature changes; if an old implementation writes through an input-only pointer, report it as incompatible. A later implementation check should compile against modern headers and a minimal legacy-signature mock; that compatibility test is not part of this checkpoint. Do not generate production bindings from this matrix yet.

The MPI-1.1 function sections consulted were the official MPI Forum C bindings for point-to-point, collectives, groups/communicators, topology, environmental inquiry, and profiling: <https://www.mpi-forum.org/docs/mpi-1.1/mpi-11-html/node170.html>, <https://www.mpi-forum.org/docs/mpi-1.1/mpi-11-html/node171.html>, <https://www.mpi-forum.org/docs/mpi-1.1/mpi-11-html/node172.html>, <https://www.mpi-forum.org/docs/mpi-1.1/mpi-11-html/node173.html>, and <https://www.mpi-forum.org/docs/mpi-1.1/mpi-11-html/node174.html>, and https://www.mpi-forum.org/docs/mpi-1.1/mpi-11-html/node175.html>. The MPI-5 deprecated/removed table was checked at <https://www.mpi-forum.org/docs/mpi-5.0/mpi50-report/node44.htm>. It confirms removal of `MPI_Address`, `MPI_Type_hindexed`, `MPI_Type_hvector`, `MPI_Type_struct`, `MPI_Type_extent`, `MPI_Type_lb`, `MPI_Type_ub`, legacy `MPI_Errhandler_create/get/set` and `MPI_Handler_function`; it marks keyval/attr legacy names deprecated but not removed. The matrix records relevant removed aliases as excluded and retains deprecated-but-still-standard forms as candidates where they were in MPI-1.1. Modern MPI-5 argument constness and any other API changes still need per-entry validation.

`thread-mpi-api.tsv` lists all 72 callable `tMPI_*` names extracted from pinned `tmpi.h`, with each row's standard MPI macro alias (if present), launcher/helper status, or no-standard-mapping status. It reports declarations and macro mappings only: this assignment did not inspect a linked tMPI binary's exported symbols or exercise its full API. The pinned compatibility header has no `MPI_Allgather`, `MPI_Allgatherv`, or `MPI_Comm_f2c` mapping. It maps `MPI_Sendrecv` to the declared tMPI entry. tMPI `MPI_Init` is not a direct standard-signature entry: `tMPI_Init` takes an additional callback; the intended internal launcher entry is `tMPI_Init_fn`. The pinned documentation says return from that callback finalizes the rank path. Custom tMPI communicator error-handler creation remains unavailable because standard `MPI_Comm_errhandler_function` is variadic, while tMPI expects `void(tMPI_Comm*,int*)` and a pointer to that callback type; no cast is valid.

## Probe commands and actual results

The reproducible command was:

```sh
src/mpiwrap/development/probe-layouts.sh
```

It creates and removes a private `mktemp` workspace, compiles locally installed MPICH/Open MPI layout probes with their wrapper compiler overrides, compiles the descriptor probe under C++17 and C11 and diffs their layout output, then extracts the pinned GROMACS include tree and runs exact signature/layout checks. No external install, clone, fetch, or pull was used. Exact output from the successful run:

```text
[mpich] wrapper=/Users/bussi/miniconda3/envs/mpich/bin/mpicxx cxx=/usr/bin/clang++
library=MPICH Version:      5.0.1
MPI_VERSION=5 MPI_SUBVERSION=0
MPI_Comm=4/4 MPI_Group=4/4 MPI_Datatype=4/4
MPI_Request=4/4 MPI_Op=4/4 MPI_Errhandler=4/4 MPI_Status=20/4
[openmpi] wrapper=/Users/bussi/miniconda3/envs/openmpi/bin/mpicxx cxx=/usr/bin/clang++
library=Open MPI v5.0.10
MPI_VERSION=3 MPI_SUBVERSION=1
MPI_Comm=8/8 MPI_Group=8/8 MPI_Datatype=8/8
MPI_Request=8/8 MPI_Op=8/8 MPI_Errhandler=8/8 MPI_Status=24/8
[provider-abi] C11 and C++17 descriptor layouts match
status sizeof=144 align=4 source=0 tag=4 error=8 raw_size=12 raw=16
descriptor sizeof=208 align=8 functions=160 metadata=176 identity_rank=184 token=200
function-entry sizeof=32 align=8 name=8 signature=16 pointer=24
metadata-entry sizeof=40 align=8 name=16 value-size=24 value=32
uintptr_t=8 wrapper-handle=8 wrapper-status-capacity=128
native-C-int sizeof=4 signed=1 MPI_Fint metadata=provider-reported
generic-function-pointer roundtrip=pass integer-handle-bytes=pass pointer-handle-bytes=pass entry-stride=pass
[gromacs-tmpi] tag=v2026.4 commit=6c69f720aa7378fddfee67f99d51aff21ccfb0a2
[gromacs-tmpi] verified Error_string and Create_errhandler callback declarations
tMPI_Comm=8/8 tMPI_Group=8/8 tMPI_Datatype=8/8
tMPI_Request=8/8 tMPI_Op=4/4 tMPI_Errhandler=8/8 tMPI_Status=32/8
```

MPICH and Open MPI's wrappers use an embedded compiler command absent from the default environment `PATH`. A first MPICH attempt without `MPICH_CXX` failed with `arm64-apple-darwin20.0.0-clang++: command not found`. The successful script sets `MPICH_CXX` and `OMPI_CXX` to `/usr/bin/clang++` by default; environment overrides are supported. Open MPI reports MPI 3.1 headers despite local library package version 5.0.10; capability must follow compile/link capability rather than package major version. No MPI communication-semantic tests were run.

The C++ probe validates generic function-pointer carrier conversion back to the original callback type and invokes the restored callback, representative integer and pointer handle-byte roundtrips and equality, and fixed entry stride. The independent C11 and C++17 layout output is compared by the shell script. `tMPI_Status` (32/8), `tMPI_Op` (4/4), and all other matrix opaque handle types are measured. The probe verifies signatures of pinned `Error_string`, `Create_errhandler`, `Init`, `Init_fn`, and `Sendrecv`; it does not probe semantic behavior or the exported-symbol table.

## Files changed in the clone

- `src/mpiwrap/development/support-matrix.tsv`: current-use and candidate MPI-1 union rows; removed aliases are explicitly excluded and incomplete signature/provider evidence is labelled.
- `src/mpiwrap/development/thread-mpi-api.tsv`: stable pinned tMPI name inventory with standard MPI macro mapping/helper/unmapped classification; export status deliberately remains unprobed.
- `src/mpiwrap/development/abi-design.md`: review-only two-ABI/lifecycle/data transport proposal with typed callback examples, ownership/lifetimes, configured capacity policy, and open decisions.
- `src/mpiwrap/development/provider-abi-proposal.h`: C-compatible proposal declaration only, not installed or used by production.
- `src/mpiwrap/development/probe-layouts.sh`, `probe-abi.cpp`, and `probe-abi-c.c`: local reproducible signature/layout/transport probes.

The new module is ignored by `src/.gitignore`; only these assigned artifacts should be force-added. No `module.type`, build integration, source vendoring, or generated/public API change was made.

## Decisions and questions for review

The ABI defaults and shape are not frozen. Current proposals are: default wrapper handle storage `uintptr_t`, default status raw capacity 128 bytes, wrapper-owned status fields at offsets 0/4/8 and raw bytes at offset 16 on the measured compiler, byte-copy conversion for opaque handles, exact typed restoration before callback invocation, fixed function/metadata entry strides per provider ABI major, and inject-before-first-object lifecycle. Configuration must select capacities once and use them consistently; provider descriptors must exactly report configured capacities and cannot alter them.

Parent/owner review is specifically requested for: whether the fixed header should be smaller than the probed proposal (currently four capability words and six reserved words); which mandatory representation values stay fixed versus named required metadata; one authority for duplicated descriptive/runtime values (the proposal forbids duplication of fixed facts); runtime identity anchors and host-token rules; public static/dynamic kernel/provider handle ownership and API spellings; default/custom handle and status capacities; and callback signature IDs. Optional named metadata additions can be layout/version neutral. New fixed descriptor fields require ABI versioning. Counted entry arrays are not variable-stride: entry records are frozen within one major.

Remaining stage-one work before acceptance: verify every candidate's exact MPI-5 C prototype/signature and correct constness; complete the MPI-1 constants/types audit against the authoritative standard; check tMPI declarations and compatibility macro signatures entry-by-entry for the selected API; probe stable tMPI linked/export behavior if the chosen local source/build artifacts make that practical; settle callback argument metadata and per-function argument conversion/test plans. None of those gaps blocks review of the current-use slice or the concrete ABI questions, but the full matrix is not yet generation-ready.
