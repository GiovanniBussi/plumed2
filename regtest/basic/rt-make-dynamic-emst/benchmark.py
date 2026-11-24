"""Compare exact MST edge sets and timings with a Git baseline in one executable."""

import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--reference-ref", default="cde0962e5")
    parser.add_argument("--sizes", default="100,1000,5000")
    parser.add_argument("--profiles", default="orthorhombic,triclinic,deforming")
    parser.add_argument("--buffers", default="0.2")
    parser.add_argument("--frames", type=int, default=5)
    parser.add_argument("--trials", type=int, default=3)
    parser.add_argument("--motion", type=float, default=0.00001)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    args = parser.parse_args()
    test = Path(__file__).resolve().parent
    root = test.parents[2]
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    for extension in ("h", "cpp"):
        source = subprocess.check_output(
            ["git", "show", f"{args.reference_ref}:src/tools/DynamicTree.{extension}"],
            cwd=root,
            text=True,
        )
        (work / f"BaselineDynamicTree.{extension}").write_text(
            source.replace("DynamicTree", "BaselineDynamicTree")
        )
    (work / "benchmark.cpp").write_text((test / "benchmark.cpp.in").read_text())
    library = root / "src/lib"
    suffix = "dylib" if sys.platform == "darwin" else "so"
    command = shlex.split(args.cxx) + [
        "-O3",
        "-std=c++17",
        f"-I{root / 'src/include'}",
        f"-I{root / 'src/tools'}",
        f"-I{work}",
        str(work / "benchmark.cpp"),
        str(work / "BaselineDynamicTree.cpp"),
        str(root / "src/tools/DynamicTree.cpp"),
        str(library / f"libplumedKernel.{suffix}"),
        f"-Wl,-rpath,{library}",
        "-o",
        str(work / "benchmark"),
    ]
    with (work / "build.log").open("w") as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    env = dict(os.environ)
    for key in ("LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH"):
        env[key] = str(library) + os.pathsep + env.get(key, "")
    with (work / "results.csv").open("w") as output:
        subprocess.run(
            [
                str(work / "benchmark"),
                args.sizes,
                args.profiles,
                args.buffers,
                str(args.frames),
                str(args.trials),
                str(args.motion),
            ],
            check=True,
            env=env,
            stdout=output,
        )
    print((work / "results.csv").read_text(), end="")


if __name__ == "__main__":
    main()
