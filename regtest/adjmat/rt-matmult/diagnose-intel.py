#!/usr/bin/env python3
"""Temporary Intel CI diagnosis; preserve the original regression and references."""

import math
import os
from pathlib import Path
import shutil
import subprocess

TEST = Path(__file__).resolve().parent
ROOT = TEST.parents[2]
OUTPUT = TEST / "intel-diagnostics"
OUTPUT.mkdir(exist_ok=True)


def run(command, directory, filename, env=None):
    print(f"\n== {directory.name}: {' '.join(command)}", flush=True)
    with (directory / filename).open("w") as log:
        result = subprocess.run(
            command, cwd=directory, env=env, stdout=log, stderr=subprocess.STDOUT
        )
    print((directory / filename).read_text(), flush=True)
    print(f"exit status: {result.returncode}", flush=True)
    return result.returncode


def xyz_forces(path):
    lines = path.read_text().splitlines()
    values = []
    index = 0
    while index < len(lines):
        count = int(lines[index])
        # The comment line holds the virial, not an atom force.
        for line in lines[index + 2 : index + 2 + count]:
            values.extend(float(value) for value in line.split()[1:])
        index += count + 2
    return values


def compare_forces(actual, reference):
    a = xyz_forces(actual)
    b = xyz_forces(reference)
    if len(a) != len(b):
        raise ValueError(f"force count mismatch: {len(a)} vs {len(b)}")
    norm = sum(value * value for value in b)
    scale = sum(x * y for x, y in zip(a, b)) / norm if norm else math.nan
    residual = max(abs(x - scale * y) for x, y in zip(a, b))
    print(f"forces vs {reference}: fitted scale={scale:.12g}, residual={residual:.12g}")


for name in (
    "PLUMED_NUM_THREADS",
    "OMP_NUM_THREADS",
    "MKL_NUM_THREADS",
    "OMP_DYNAMIC",
    "PLUMED_KERNEL",
    "PLUMED_MPI_WRAPPER",
    "PLUMED_MPI_WRAPPER_FORCE",
):
    print(f"{name}={os.environ.get(name, '<unset>')}", flush=True)
for command in (["icpx", "--version"], ["plumed", "config", "show"]):
    run(command, OUTPUT, command[0] + "-configuration.log")
for library in [shutil.which("plumed"), *ROOT.glob("src/lib/libplumedKernel.so")]:
    if library:
        run(["ldd", str(library)], OUTPUT, Path(library).name + "-ldd.log")

# Keep the failing regression exactly as shipped, including its references.
status = run(["make", "test"], TEST, "intel-diagnostics/original-regtest.log")
report = TEST / "report.txt"
if report.exists():
    print(report.read_text(), flush=True)
    status = status or int("FAILURE" in report.read_text())
else:
    print("ERROR: regression produced no report.txt", flush=True)
    status = 1

original = (TEST / "plumed.dat").read_text()
prints = "\nPRINT ARG=diag.vecs-1 FILE=eigenvector FMT=%.16g\n"
# AVOID_THREAD_GATHER selects the two-pass force accumulation path; compare it
# with the default and with one thread. Separate inputs probe alias accumulation.
cases = [
    ("default", original, 2, False),
    ("one-thread", original, 1, False),
    ("avoid-thread-gather", original.replace("ARG=c1,c1", "ARG=c1,c1 AVOID_THREAD_GATHER"), 2, False),
    ("separate-inputs", original.replace("c12: MATRIX_PRODUCT ARG=c1,c1", "c2: CONTACT_MATRIX GROUP=1-7 SWITCH={RATIONAL R_0=2.6 NN=6 MM=12}\nc12: MATRIX_PRODUCT ARG=c1,c2"), 2, False),
    # Numerical forces are diagnostic only, never new regression references.
    ("finite-difference", original, 2, True),
]
for name, text, threads, numerical in cases:
    directory = OUTPUT / name
    directory.mkdir(exist_ok=True)
    shutil.copyfile(TEST / "isomers.xyz", directory / "isomers.xyz")
    (directory / "plumed.dat").write_text(text + prints)
    env = dict(os.environ, PLUMED_NUM_THREADS=str(threads), OMP_NUM_THREADS=str(threads))
    command = [
        "plumed", "--no-mpi", "driver", "--plumed", "plumed.dat", "--ixyz",
        "isomers.xyz", "--dump-forces", "forces", "--dump-forces-fmt=%.16g",
    ]
    if numerical:
        command += ["--debug-forces", "forces.num"]
    code = run(command, directory, "driver.log", env)
    status = status or code
    if code:
        continue
    try:
        compare_forces(directory / "forces", TEST / "forces.reference")
        for line in (directory / "eigenvector").read_text().splitlines():
            if line.startswith("#") or not line.strip():
                continue
            row = [float(value) for value in line.split()]
            print(f"time={row[0]:g} eigenvector norm squared={sum(x*x for x in row[1:]):.16g}")
        print("eigenvalues:\n" + (directory / "colvar").read_text())
        if numerical:
            rows = [
                [float(value) for value in line.split()]
                for line in (directory / "forces.num").read_text().splitlines()
                if line.strip() and not line.startswith("#")
            ]
            print("max analytical/numerical discrepancy (including virial):",
                  max(abs(row[1] - row[2]) for row in rows))
    except (OSError, ValueError) as error:
        print(f"ERROR reading diagnostic outputs: {error}", flush=True)
        status = 1

raise SystemExit(status)
