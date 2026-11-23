"""Ensure the installed package identifies the exact patch source."""

import json
import sys
from pathlib import Path

from package import HERE

prefix, version_file = map(Path, sys.argv[1:])
expected = json.loads((HERE / "manifest.json").read_text())
installed = json.loads((prefix / "share/gromacs-plumed/provenance.json").read_text())
for key in expected:
    if installed[key] != expected[key]:
        raise ValueError("Installed provenance mismatch: " + key)
if installed["patch_mode"] != "runtime":
    raise ValueError("Package did not use runtime patching")
marker = "PLUMED_" + expected["snapshot_date"] + "_g" + expected["plumed_commit"][:12]
if marker not in version_file.read_text():
    raise ValueError("gmx --version lost the PLUMED patch identity")
print("Installed package patch provenance verified:", expected["plumed_commit"])
