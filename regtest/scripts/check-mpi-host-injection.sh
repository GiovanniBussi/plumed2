#!/usr/bin/env bash
# Build the MPI host after the kernel: no MPI link or initialization in PLUMED.
set -euo pipefail
if (($# != 3)); then
  echo 'usage: check-mpi-host-injection.sh MPI_CXX_COMPILER MPIEXEC SERIAL_KERNEL' >&2
  exit 2
fi
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TASK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/plumed-host-injection.XXXXXX")
trap 'rm -rf "$TASK_DIR"' EXIT
"$1" -std=c++98 "$ROOT/regtest/scripts/mpi-cli.cpp" -ldl -o "$TASK_DIR/mpi-host"
python3 - "$ROOT" "$TASK_DIR" "$2" "$3" <<'PYTHON'
import difflib
import os
import pathlib
import shutil
import subprocess
import sys

root, directory, launcher, kernel = map(pathlib.Path, sys.argv[1:])
root, directory, launcher, kernel = [p.resolve() for p in (root, directory, launcher, kernel)]
fixture = root / "regtest/basic/rt-mpi0"
environment = os.environ.copy()
for name in ("PLUMED_MPI_WRAPPER", "PLUMED_MPI_WRAPPER_FORCE", "PLUMED_LOAD_DEBUG"):
    environment.pop(name, None)
environment["PLUMED_KERNEL"] = str(kernel)
for explicit in (False, True):
    for ranks in (1, 2, 4):
        work = directory / f"{'path' if explicit else 'selected'}-{ranks}"
        work.mkdir()
        shutil.copyfile(fixture / "plumed.dat", work / "plumed.dat")
        if explicit:
            environment["PLUMED_MPI_HOST_EXPLICIT_PATH"] = str(kernel)
            # Explicit-path creation must not accidentally use the environment.
            environment["PLUMED_KERNEL"] = str(work / "missing-kernel")
        else:
            environment.pop("PLUMED_MPI_HOST_EXPLICIT_PATH", None)
            environment["PLUMED_KERNEL"] = str(kernel)
        result = subprocess.run(
            [str(launcher), "-n", str(ranks), str(directory / "mpi-host"),
             "driver", "--plumed=plumed.dat", "--timestep=0.05", "--ixyz",
             str(root / "regtest/trajectories/trajectory.xyz"), "--dump-forces", "ff"],
            cwd=work, env=environment, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, timeout=120,
        )
        if result.returncode:
            print(result.stdout, file=sys.stderr)
            raise SystemExit(f"host injection failed: explicit={explicit}, ranks={ranks}")
        for reference in fixture.glob("*.reference"):
            name = reference.name[:-len(".reference")]
            if name.startswith("ff.") and int(name.split(".")[1]) >= ranks:
                continue
            # Driver adds the rank suffix only when the communicator has
            # more than one rank. The reference fixture uses four ranks.
            actual = work / ("ff" if ranks == 1 and name == "ff.0" else name)
            expected = work / (name + ".expected")
            if not actual.is_file():
                raise SystemExit(f"host injection did not produce {name}")
            shutil.copyfile(reference, expected)
            # Use the same portable normalization as the ordinary regtests.
            subprocess.run([str(root / "regtest/scripts/fixzeroes.sh"), str(actual)], check=True)
            subprocess.run([str(root / "regtest/scripts/fixzeroes.sh"), str(expected)], check=True)
            if actual.read_bytes() != expected.read_bytes():
                print("".join(difflib.unified_diff(
                    expected.read_text().splitlines(True), actual.read_text().splitlines(True),
                    fromfile=str(reference), tofile=str(actual))), file=sys.stderr)
                raise SystemExit(f"host injection differs: {name}, explicit={explicit}, ranks={ranks}")
        print(f"host injection passed: explicit={explicit}, ranks={ranks}")
PYTHON
