# Long-long MPI semantics

The ordinary four-rank make regression tests both signed and unsigned long long.
In an embedded tMPI build it disables all three long-long value accessors between
rank barriers, forcing the fallback used by GROMACS builds without
`SIZEOF_LONG_LONG_INT`. It repeats launches at 4, 1, 2 and 4 ranks to verify native
datatype cache lifetimes. It also exercises the Communicator specializations used
by checkpoint and OPES code. Native MPI builds run the same checks directly.

Each forced-missing run tests automatic path selection, then disables the long
accessors to force the generic path even on LP64. A provider Allreduce counter
asserts that native-long reductions are used only with matching ranges.

The standalone harness also builds with MSVC:

```sh
cmake -S regtest/basic/rt-mpiwrap-long-long -B build/long-long-mpi
cmake --build build/long-long-mpi --config Release
ctest --test-dir build/long-long-mpi -C Release --verbose
```

The existing Windows thread-MPI CI step runs this harness at 1, 2 and 4 ranks.
Its Windows build asserts `sizeof(long)==4` and `sizeof(long long)==8`, and the
counter asserts the full-width path is used even with long accessors available.

`checks.h` covers all currently supported datatype-taking function families:
basic and vector collectives, every valid predefined integer reduction operator,
chunked reductions, all supported point-to-point send modes, request completion
families, persistent requests, cancellation, probes and count queries, supported
datatype constructors, size, and packing. Tests condition calls on provider
function availability; the fallback does not invent unrelated tMPI functions.

For the larger native-provider surface, run:

```sh
sh regtest/basic/rt-mpiwrap-long-long/check-provider.sh \
  /opt/local/bin/mpicc-openmpi-mp /opt/local/bin/mpiexec-openmpi-mp \
  --host localhost:4 --bind-to none
```

This builds an independent Open MPI host and a provider-mode frontend. At 1, 2
and 4 ranks it checks native support, then a provider deliberately stripped of
the three long-long values and advertised as homogeneous/threaded for the test.
That deliberate test descriptor allows validating optional native functions
with the same fallback; ordinary real-MPI providers never use byte emulation.
The script uses Open MPI's `--showme:link` compiler query.

The standalone direct path can also be compiled without a PLUMED build:

```sh
mpicxx -std=c++17 -D__PLUMED_HAS_MPI -Isrc \
  regtest/basic/rt-mpiwrap-long-long/native-direct.cpp -o /tmp/wide-native
mpiexec -n 4 /tmp/wide-native
```
