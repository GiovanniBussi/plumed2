"""Matched startup timings with isolated baseline/current kernels and RCSB 1S72.

No Python dependencies. Run only after other local builds/tests have stopped.
Each kernel runs in a fresh process; timing includes MOLINFO/input setup and
the first calculation, where local static trees are built lazily.
"""

import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import shutil
import statistics
import subprocess
import time
import urllib.request


def prepare(source, work):
    """Keep first-model/first-altloc atoms, preserving residue/block identity."""
    text = source.read_text()
    output, atoms, residues, altlocs = [], [], {}, {}
    block = 0
    for line in text.splitlines():
        record = line[:6].strip()
        if record in ("ENDMDL", "END"):
            break
        if record == "CRYST1":
            output.append(line)
        if record == "TER":
            output.append("TER")
            block += 1
        if record not in ("ATOM", "HETATM"):
            continue
        key = (block, line[21:22], int(line[22:26]), line[26:27])
        altkey = key[1:]
        altloc = line[16:17].strip()
        if altloc and altloc != altlocs.setdefault(altkey, altloc):
            continue
        index = len(atoms) + 1
        # Match ATOM_ID=order and remove already resolved altloc markers.
        output.append(line[:6] + f"{index:5d}" + line[11:16] + " " + line[17:])
        xyz = [float(line[k : k + 8]) * 0.1 for k in (30, 38, 46)]
        atoms.append(xyz)
        entry = residues.setdefault(key, {"name": line[17:20].strip(), "atoms": {}})
        entry["atoms"][line[12:16].strip()] = index
    output.append("END")
    pdb = work / "reference.pdb"
    pdb.write_text("\n".join(output) + "\n")
    positions = work / "positions.dat"
    with positions.open("w") as stream:
        stream.write(f"{len(atoms)}\n")
        for xyz in atoms:
            stream.write(" ".join(map(str, xyz)) + "\n")
    rna = [r for r in residues.values() if r["name"] in ("A", "C", "G", "U")]

    def triplets(names):
        return [
            r["atoms"][name]
            for r in rna
            if all(n in r["atoms"] for n in names)
            for name in names
        ]

    ring = triplets(("C2", "C4", "C6"))
    rna_atoms = [i for r in rna for i in r["atoms"].values()]
    selections = {
        "triplets": [ring],
        "rna": [rna_atoms],
        "all": [list(range(1, len(atoms) + 1))],
        "repeat": [ring] * 4,
        "distinct": [
            ring,
            triplets(("C1'", "C2'", "C3'")),
            triplets(("C3'", "C4'", "C5'")),
        ],
        "one-per-residue": [
            [next(iter(r["atoms"].values())) for r in residues.values()]
        ],
    }
    residue_for_atom = {
        i: key for key, r in residues.items() for i in r["atoms"].values()
    }
    counts = {
        case: [
            {"atoms": len(s), "residues": len({residue_for_atom[i] for i in s})}
            for s in lists
        ]
        for case, lists in selections.items()
    }
    return selections, counts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--baseline-kernel", type=Path, required=True)
    parser.add_argument("--kernel", type=Path, required=True)
    parser.add_argument("--pdb", type=Path)
    parser.add_argument(
        "--cases", default="triplets,rna,all,repeat,distinct,one-per-residue"
    )
    parser.add_argument("--trials", type=int, default=3)
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--cxx", default="clang++")
    args = parser.parse_args()
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    source = args.pdb.resolve() if args.pdb else work / "1S72.pdb"
    if not source.exists():
        with urllib.request.urlopen(
            "https://files.rcsb.org/download/1S72.pdb", timeout=60
        ) as remote:
            source.write_bytes(remote.read())
    selections, counts = prepare(source, work)
    root = Path(__file__).resolve().parents[2]
    with (work / "build.log").open("w") as log:
        subprocess.run(
            [
                args.cxx,
                "-O3",
                "-std=c++17",
                f"-I{root / 'src/wrapper'}",
                str(Path(__file__).with_name("startup.cpp")),
                "-ldl",
                "-o",
                str(work / "startup"),
            ],
            check=True,
            stdout=log,
            stderr=subprocess.STDOUT,
        )
    manifest = {
        "source_url": "https://files.rcsb.org/download/1S72.pdb",
        "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "reference_sha256": hashlib.sha256(
            (work / "reference.pdb").read_bytes()
        ).hexdigest(),
        "baseline_kernel": str(args.baseline_kernel.resolve()),
        "kernel": str(args.kernel.resolve()),
        "baseline_kernel_sha256": hashlib.sha256(
            args.baseline_kernel.read_bytes()
        ).hexdigest(),
        "kernel_sha256": hashlib.sha256(args.kernel.read_bytes()).hexdigest(),
        "trials": args.trials,
        "selections": counts,
    }
    (work / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    records, expected = [], {}
    modes = {
        "baseline": args.baseline_kernel.resolve(),
        "cached-full": args.kernel.resolve(),
        "cached-residue": args.kernel.resolve(),
    }
    for case in args.cases.split(","):
        for trial in range(args.trials):
            # Rotate mode order rather than always warming the host with baseline.
            order = list(modes)
            order = order[trial % 3 :] + order[: trial % 3]
            for mode in order:
                directory = work / f"{case}-{mode}-{trial}"
                directory.mkdir(exist_ok=True)
                for name in ("reference.pdb", "positions.dat"):
                    target = directory / name
                    if not target.exists():
                        target.symlink_to(work / name)
                lines = [
                    "MOLINFO STRUCTURE=reference.pdb ATOM_ID=order PYTHON_BIN=no WHOLE"
                    + (" SMALL_RESIDUES" if mode == "cached-residue" else "")
                ]
                for i, selected in enumerate(selections[case]):
                    lines.append(
                        f"g{i}: GYRATION_SCALAR ATOMS=" + ",".join(map(str, selected))
                    )
                lines.append(
                    "PRINT ARG="
                    + ",".join(f"g{i}" for i in range(len(selections[case])))
                    + " FILE=colvar FMT=%.15g"
                )
                (directory / "plumed.dat").write_text("\n".join(lines) + "\n")
                started = time.perf_counter()
                run = subprocess.run(
                    [str(work / "startup"), str(modes[mode])],
                    cwd=directory,
                    check=False,
                    capture_output=True,
                    text=True,
                    timeout=args.timeout,
                )
                process_ms = (time.perf_counter() - started) * 1000
                (directory / "stderr.log").write_text(run.stderr)
                run.check_returncode()
                fields = list(
                    map(float, run.stdout.strip().splitlines()[-1].split(","))
                )
                values = [
                    list(map(float, line.split()))[1:]
                    for line in (directory / "colvar").read_text().splitlines()
                    if line and not line.startswith("#")
                ][-1]
                if case not in expected:
                    expected[case] = values
                if len(values) != len(expected[case]) or any(
                    not math.isfinite(a)
                    or not math.isclose(a, b, rel_tol=1e-11, abs_tol=1e-11)
                    for a, b in zip(values, expected[case])
                ):
                    raise RuntimeError(
                        f"CV mismatch for {case}/{mode}/{trial}: {values}"
                    )
                record = {
                    "case": case,
                    "mode": mode,
                    "trial": trial,
                    "actions": len(values),
                    "load_ms": fields[0],
                    "setup_ms": fields[1],
                    "first_calc_ms": fields[2],
                    "startup_ms": fields[3],
                    "process_ms": process_ms,
                }
                records.append(record)
                print(json.dumps(record), flush=True)
                with (work / "trials.csv").open("w", newline="") as stream:
                    writer = csv.DictWriter(
                        stream, fieldnames=list(record), lineterminator="\n"
                    )
                    writer.writeheader()
                    writer.writerows(records)
    with (work / "summary.csv").open("w", newline="") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(
            (
                "case",
                "mode",
                "actions",
                "median_startup_ms",
                "median_setup_ms",
                "median_first_calc_ms",
            )
        )
        for case in args.cases.split(","):
            for mode in modes:
                rows = [r for r in records if r["case"] == case and r["mode"] == mode]
                writer.writerow(
                    (
                        case,
                        mode,
                        len(selections[case]),
                        *(
                            statistics.median(r[key] for r in rows)
                            for key in ("startup_ms", "setup_ms", "first_calc_ms")
                        ),
                    )
                )
    shutil.copyfile(work / "summary.csv", work / "results.csv")


if __name__ == "__main__":
    main()
