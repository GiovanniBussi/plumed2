Static tree startup benchmark
============================

This benchmark compares an unmodified upstream kernel with the cached full-tree
and cached residue-tree policies. Build both with the same compiler and flags.
Keep the baseline kernel before editing/rebuilding the checkout. For example:

```
./configure --disable-mpi --disable-openmp --disable-libsearch --enable-modules=reset
make -j8
cp src/lib/libplumedKernel.dylib /path/inside/project/baseline.dylib
# Apply the changes and rebuild with the same configuration.
python3 developer-doc/static-tree-benchmark/run.py \
  --work /path/inside/project/startup-benchmark \
  --baseline-kernel /path/inside/project/baseline.dylib \
  --kernel src/lib/libplumedKernel.dylib --trials 3
```

Run after other local build/test jobs have stopped. `run.py` uses only the Python
standard library and compiles a standalone C++ wrapper with the selected C++
compiler. It loads exactly one kernel per fresh process. On Linux use the actual
`.so` kernel paths; the C++ runner itself is independent of the library suffix.

Data are downloaded from https://files.rcsb.org/download/1S72.pdb, the same large
ribosomal subunit entry used for the ERMSD optimization. `--pdb` can supply a
previous download. Input and normalized reference SHA256 hashes and selection
counts are recorded in `manifest.json`. Only the first model and the first
alternate conformation per residue are retained. Coordinates and all atoms of
each reference residue are retained; selections do not truncate residue centers.
The normalized reference uses `ATOM_ID=order` to align MD indices without TER gaps.

Cases (`--cases` accepts a comma-separated subset):

* `triplets`: C2/C4/C6 from every complete standard RNA residue (the ERMSD-style
  selection, three atoms per residue).
* `rna`: every atom of standard A/C/G/U residues.
* `all`: every retained atom, including proteins, ions and water.
* `repeat`: four local GYRATION_SCALAR actions requesting the same RNA triplets.
* `distinct`: three actions using different ring/ribose triplets.
* `one-per-residue`: one selected atom per reference residue, a negative control
  for geometric reduction.

Every mode and trial must produce the same finite gyration values for a case.
The 100 nm cubic cell leaves the deposited structure unwrapped, so this checks
that topology changes preserve these values without making an imaging ambiguity
part of the timing. The regression fixture separately checks nontrivial wrapping,
forces and virial. Each case uses its own Plumed instance and MOLINFO reference.

`startup_ms` is kernel setup plus the first calculation: it includes MOLINFO
parsing, flag-specific metadata preparation, action creation, tree construction,
cache lookup and ordinary first-frame work. Trees for local actions are built
on that first calculation. Kernel dynamic loading is reported separately, as is
external process wall time. Download, normalization, Python generation and the
runner's coordinate-file parsing are outside this timer. This measures startup,
not MD throughput. Modes rotate order between trials. Host load is uncontrolled;
report medians and the raw trial spread, not universal speedup guarantees.

`trials.csv` records every trial; `summary.csv`/`results.csv` contain medians.
Logs and per-case inputs are retained in the work directory. The full-atom
baseline can be slow because its work is quadratic; use `--cases triplets` for
a short smoke benchmark before running all cases.
