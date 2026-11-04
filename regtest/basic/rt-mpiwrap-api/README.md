# Common mapped-MPI API regression

`main.cpp` uses the shared `src/mpiwrap` launcher and four ranks. The ordinary
regtest runner supplies `-n 4` from `mpiprocs=4`; it uses `mpiexec` for a native
build and the internal thread-MPI launcher for a serial build. No optional
PLUMED module is needed. Each callback owns its buffers, handles, and objects.

The same body exercises all 118 mapped functions. The tests verify values,
rank/group/topology relationships, derived datatype layout, request completion
indices/statuses, buffer attachment, packing, persistent requests, and inquiries.
Ready sends have matching receives posted before a barrier. A full native MPI
run also exercises standard reduction and error-handler callback registration.
Providers intentionally omit these callback constructors; the test verifies
their clean unavailable exceptions instead. Error-handler callbacks are never
installed on a communicator or triggered.

`thread-mpi-contract.json` is an explicit expected capability list. The test
checks **both** its available and unavailable entries, so losing a binding cannot
turn a passing semantic test into a silently skipped test. Every missing entry
is called with structurally valid dummy output storage and must throw a
`std::runtime_error` identifying that MPI function. These dummy calls never
reach a native MPI implementation.

Native collective entries are tested with derived types as well. When
`Allgather` is implemented by the shared fallback, the test verifies that a
custom contiguous datatype is rejected clearly. This limitation belongs to the
fallback, not to native MPI or other operations on derived types.

`MPI_Init` and `MPI_Finalize` belong to the shared launcher: the rank body checks
initialization/finalization state and the caller checks finalization after the
launch. Calling either lifecycle function directly through the tMPI frontend
must throw because the launcher owns it. `MPI_Abort` is run by the after-hook in
a separate invocation of the same executable; the checker requires a diagnostic
proving the intended call was reached, a failing exit status, and no return from
the call. A timeout is a failure, never a passing abort test.

`check-coverage.py` reads the generator's actual argument metadata and requires a
real call in the test source or an explicit lifecycle/destructive owner from
`lifecycle-coverage.json` for every mapping. Comments do not satisfy coverage.
It also checks that the generated unavailable-call and capability checks match
the current inventory. Run it before a mapping change is committed:

```
python3 regtest/basic/rt-mpiwrap-api/check-coverage.py
```

For a deliberate inventory change, first add a semantic scenario and review the
capability contract, then regenerate the two local includes with `--write`.
This mechanical check prevents forgotten scenarios; semantic quality still
requires review. The test does not claim exhaustive parameter/error-path
coverage of the MPI standard.

An already built executable can be checked independently, including the
isolated abort case:

```
python3 check-isolated.py ./exe
python3 check-isolated.py mpiexec -n 4 ./exe
```

Native builds require MPI-2.2 or later for `MPI_C_BOOL` and `MPI_INT64_T`, as do
these particular datatype scenarios. They do not change the MPI version
requirements of the provider itself.
