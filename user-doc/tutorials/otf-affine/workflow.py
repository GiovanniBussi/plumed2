#!/usr/bin/env python3
"""Run both routes and three seeds, then produce figures and statistical summaries."""

import argparse
import json
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import benchmark as b


def main():
    p = argparse.ArgumentParser(__doc__)
    p.add_argument("--plumed", required=True)
    p.add_argument("--output", default="runs")
    p.add_argument("--jobs", type=int, default=1)
    p.add_argument("--quick", action="store_true")
    p.add_argument("--target", default=str(b.HERE / "targets-original.dat"))
    a = p.parse_args()
    root = Path(a.output).resolve()
    exe = Path(a.plumed).resolve()
    b.build_engine(exe)
    targets = b.np.loadtxt(a.target)[:, 1]
    jobs = [
        (route, phase, seed)
        for route in ["coordination", "saxs"]
        for phase in ["prior", "reference", "joint"]
        for seed in [0, -12345, -23456]
    ]

    def task(spec):
        route, phase, seed = spec
        d = b.generate(
            root / route / f"{phase}-{seed}",
            route,
            phase,
            seed,
            targets if phase == "joint" else None,
            a.quick,
        )
        b.run(d, exe)
        m = json.loads((d / "manifest.json").read_text())
        print(route, phase, seed, f"{m['runtime_seconds']:.2f} s", flush=True)

    with ThreadPoolExecutor(max_workers=a.jobs) as pool:
        list(pool.map(task, jobs))
    if not a.quick:
        subprocess.run(
            [
                sys.executable,
                str(b.HERE / "analyze.py"),
                "--runs",
                str(root),
                "--output",
                str(root / "results"),
            ],
            check=True,
        )


if __name__ == "__main__":
    main()
