This exercises internal WithCmd shape deduction rather than the standalone wrapper.
`matrix.cpp` reuses rt-make-10/main.inc and compares its numerical output and
success/failure decisions with that wrapper fixture. Only exception prose is
normalized, since internal exceptions include source locations.

`main.cpp` checks scalar types, constness, ranks, extents, element counts,
copy/temporary flags, old/new virtual overrides, all key types, explicit
TypesafePtr arguments, empty containers, strings and native/custom fixed types.
Real PlumedMain commands check positions, writable forces, shape rejection and
retention rejection. `rejected.make`, invoked by the normal regression hook,
requires compilation to reject nested dynamic containers and vector<bool>.

Run after building and sourcing sourceme.sh:

    make -C regtest/basic/rt-make-withcmd-shapes test

The public internal customization is PLMD::CmdArray<T>, with value_type and a
true_type base. Like the wrapper's customization, it asserts that the object
consists of packed, contiguous value_type elements. C++ standard-layout and size
checks catch some mistakes; the specialization author remains responsible for
the actual layout contract. Constness is propagated by the helper. Native
VectorTyped/TensorTyped use their actual data() accessors.

Raw scalar pointers keep unknown size for compatibility. A pointer to fixed
arrays has an unknown outer extent (SIZE_MAX), followed by fixed extents. No
strict mode is introduced. Zero terminates shape metadata, as in the wrapper;
empty nested arrays consequently cannot retain a zero axis.
