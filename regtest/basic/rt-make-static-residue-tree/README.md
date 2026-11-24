Static reconstruction regression
===============================

The fixture writes a small reference with repeated residue numbers in different
chains, an insertion code, a repeated chain/residue after TER, and coincident
representatives. Explicit expected edge sets check representative selection from
requested atoms using the center of the full reference residue, the reduced
EMST and intra-residue stars. Reordered requests check the original image anchor
and parent-before-child traversal. Independent atom image shifts exercise both
orthorhombic and triclinic reconstruction.

Pointer identity checks shared immutable topology, cache separation by request
order and MOLINFO instance, expiry, and pruning of expired keys while retaining
live entries. Actual GYRATION_SCALAR actions and duplicate WHOLEMOLECULES entities
check that callers share the tree. Values, forces and virial are compared with
the legacy reconstruction on the same wrapped coordinates.

The default policy is checked on a material encoded as one long residue: the
tree remains the full selected-atom EMST. Empty/singleton selections, duplicate
requested atoms, zero-length representative links, missing reference atoms and
rejection of SMALL_RESIDUES without WHOLE are also exercised. Missing-reference
links retain previous-atom connectivity; a missing first atom remains an error.

These tests validate reconstruction under the compact-residue assumption. They
do not promise that a residue star matches the unrestricted full-atom EMST, or
that direct stars unwrap an arbitrarily extended residue in a small cell.
