WRAPAROUND residue grouping regression
=====================================

A reference with three-/four-atom residues and a one-atom ion includes repeated
residue numbers across chains, an insertion code, and repeated identifiers after
TER. Expected anchors and partitions are explicit: output is compared against
existing GROUPBY commands with those anchors placed first. Current coordinates
are changed independently of reference geometry, with arbitrary per-atom image
shifts, in both rectangular and skew cells.

Checks cover partial selections using batch-only centers, nonconsecutive repeated reference identifiers, stable duplicate
removal, separate contiguous runs, PAIR order and repeated references, virtual
COM references, overlapping ATOMS/AROUND, unchanged unselected atoms, STRIDE,
empty selections and downstream DUMPATOMS. Invalid declaration, missing atomic
or virtual-atom metadata, conflicting GROUPBY and both PAIR count checks must
produce specific diagnostics. Legacy GROUPBY anchors remain unchanged.
