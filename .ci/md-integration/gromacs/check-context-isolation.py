"""Check context isolation and, with a kernel argument, actual provider bindings."""

import json
import os
import shlex
import subprocess
import sys
from pathlib import Path

build = Path(sys.argv[1]).resolve()
output = Path(sys.argv[2]).resolve()
output.mkdir(parents=True, exist_ok=True)
entries = json.loads((build / "compile_commands.json").read_text())
entry = next(e for e in entries if e["file"].endswith("/mdtypes/iforceprovider.cpp"))
arguments = entry.get("arguments") or shlex.split(entry["command"])
flags = []
index = 1
while index < len(arguments):
    arg = arguments[index]
    if arg in ("-o", "-MF", "-MT", "-MQ"):
        index += 2
        continue
    if arg not in ("-c", "-MD", "-MMD", entry["file"]):
        flags.append(arg)
    index += 1
library = "gromacs_mpi" if list((build / "lib").glob("libgromacs_mpi.*")) else "gromacs"


def check(name, destination, kernel=None):
    destination.mkdir(parents=True, exist_ok=True)
    binary = destination / name
    command = [
        arguments[0],
        *flags,
        str(Path(__file__).with_name(name + ".cpp")),
        "-o",
        str(binary),
        "-L" + str(build / "lib"),
        "-l" + library,
        "-Wl,-rpath," + str(build / "lib"),
        "-pthread",
    ]
    if kernel:
        source = Path(entry["file"]).resolve().parents[3]
        command.append("-I" + str(source))
    (destination / "command.json").write_text(json.dumps(command, indent=2) + "\n")
    with (destination / "compile.log").open("w") as log:
        subprocess.run(
            command,
            cwd=entry["directory"],
            stdout=log,
            stderr=subprocess.STDOUT,
            check=True,
        )
    environment = os.environ.copy()
    if kernel:
        environment["PLUMED_KERNEL"] = str(Path(kernel).resolve())
    with (destination / "output").open("w") as log:
        subprocess.run(
            [str(binary)],
            cwd=destination,
            env=environment,
            stdout=log,
            stderr=subprocess.STDOUT,
            check=True,
            timeout=60,
        )
    print((destination / "output").read_text(), end="")


check("context-isolation", output)
if len(sys.argv) > 3:
    check("provider-input-lifetime", output / "provider-input-lifetime", sys.argv[3])
