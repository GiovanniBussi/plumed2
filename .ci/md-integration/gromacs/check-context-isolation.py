"""Compile and run the context isolation fixture against the selected host build."""

import json
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
binary = output / "context-isolation"
command = [
    arguments[0],
    *flags,
    str(Path(__file__).with_name("context-isolation.cpp")),
    "-o",
    str(binary),
    "-L" + str(build / "lib"),
    "-l" + library,
    "-Wl,-rpath," + str(build / "lib"),
    "-pthread",
]
(output / "command.json").write_text(json.dumps(command, indent=2) + "\n")
with (output / "compile.log").open("w") as log:
    subprocess.run(
        command,
        cwd=entry["directory"],
        stdout=log,
        stderr=subprocess.STDOUT,
        check=True,
    )
with (output / "output").open("w") as log:
    subprocess.run(
        [str(binary)], stdout=log, stderr=subprocess.STDOUT, check=True, timeout=60
    )
print((output / "output").read_text(), end="")
