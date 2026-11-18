#!/usr/bin/env python3
"""Generate/run the source-faithful ten-bead simplemd reproduction."""

import argparse
import json
import shutil
import subprocess
import time
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
Q = np.arange(1, 11) / 10
DT = 0.001


def xyz(positions):
    return "10\n100 100 100\n" + "".join(
        "X " + " ".join(f"{v:.16g}" for v in r) + "\n" for r in positions
    )


def initial():
    # Davide's generate_chain(10, 1.): straight chain along z.
    r = np.zeros((10, 3))
    r[:, 2] = np.arange(10)
    return r


def physical(reference=False):
    lines = []
    for i in range(1, 10):
        lines += [
            f"d{i}: DISTANCE ATOMS={i},{i + 1}",
            f"f{i}: CUSTOM ARG=d{i} FUNC=-33.75*log(select(1-step(x*x-2.25),1-x*x/2.25,1)) PERIODIC=NO",
        ]
    lines += [
        "fene: COMBINE ARG=" + ",".join(f"f{i}" for i in range(1, 10)) + " PERIODIC=NO",
        "fb: BIASVALUE ARG=fene",
    ]
    for i in range(1, 9):
        lines += [f"a{i}: ANGLE ATOMS={i},{i + 1},{i + 2}"]
        if reference:
            lines += [f"c{i}: CUSTOM ARG=a{i} FUNC=5*cos(x) PERIODIC=NO"]
    if reference:
        lines += [
            "angular: COMBINE ARG="
            + ",".join(f"c{i}" for i in range(1, 9))
            + " PERIODIC=NO",
            "ab: BIASVALUE ARG=angular",
        ]
    lines += ["rg: GYRATION ATOMS=1-10 NOPBC"]
    return "\n".join(lines) + "\n"


def observables(route):
    if route == "coordination":
        return (
            "\n".join(
                f"iq{i}: COORDINATION GROUPA=1-10 NOPBC SWITCH={{CUSTOM R_0={10 / i:.16g} FUNC=sin(x)/x}}"
                for i in range(1, 11)
            )
            + "\n"
        )
    return (
        "s: SAXS ATOMS=1-10 NOPBC "
        + " ".join(f"PARAMETERS{i}=1" for i in range(1, 11))
        + " "
        + " ".join(f"QVALUE{i}={i / 100:.16g}" for i in range(1, 11))
        + "\n"
        + "\n".join(
            f"iq{i}: CUSTOM ARG=s.q-{i - 1} FUNC=50*x-5 PERIODIC=NO"
            for i in range(1, 11)
        )
        + "\n"
    )


def affine(target, start=10, duration=2000):
    return (
        "m: MAXENT ARG="
        + ",".join(f"iq{i}" for i in range(1, 11))
        + " AFFINE TYPE=EQUAL SIGMA=1 PACE=1 PRINT_STRIDE=1000 FMT=%.17g AT="
        + ",".join(f"{x:.17g}" for x in target)
        + " KAPPA="
        + ",".join(["0.0002"] * 10)
        + " TAU="
        + ",".join(["50"] * 10)
        + f" AFFINE_KAPPA_SCALE=0.00001 AFFINE_KAPPA_OFFSET=0.000002 AFFINE_START={start:.12g} TSTART={duration / 2:.12g} TEND={duration:.12g}\n"
    )


def generate(out, route, phase, seed, target=None, quick=False):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    equil = 10
    duration = 2 if quick else 2000
    prod = 1 if quick else 1000
    nstep = round((equil + (duration if phase == "joint" else 0) + prod) / DT)
    plugin = HERE / "MaxEntAffine.cpp"
    shutil.copy2(plugin, out / "MaxEntAffine.cpp")
    lines = (
        "LOAD FILE=MaxEntAffine.cpp\n"
        + physical(phase == "reference")
        + observables(route)
    )
    args = [
        *(f"iq{i}" for i in range(1, 11)),
        "rg",
        "a4",
        *(f"d{i}" for i in range(1, 10)),
        "fene",
    ]
    if phase == "joint":
        if target is None:
            raise ValueError("joint phase requires target")
        lines += affine(target, equil, duration)
        args += [
            "m.scale",
            "m.offset",
            "m.frozen",
            "m.bias",
            "m.work",
            *(f"m.iq{i}_coupling" for i in range(1, 11)),
        ]
    lines += "PRINT ARG=" + ",".join(args) + " STRIDE=100 FILE=COLVAR FMT=%.17g\n"
    (out / "plumed.dat").write_text(lines)
    (out / "initial.xyz").write_text(xyz(initial()))
    (out / "simplemd.in").write_text(
        f"inputfile initial.xyz\noutputfile final.xyz\ntemperature 0.722\ntstep 0.001\nfriction 1\nforcecutoff 2.5\nlistcutoff 3\nnstep {nstep}\nnconfig 1000 trajectory.xyz\nnstat 100 energies.dat\nidum {seed}\nwrapatoms false\n"
    )
    (out / "manifest.json").write_text(
        json.dumps(
            {
                "route": route,
                "phase": phase,
                "seed": seed,
                "dt": DT,
                "nstep": nstep,
                "equilibration": equil,
                "evolution": duration if phase == "joint" else 0,
                "production": prod,
                "target": None if target is None else list(target),
                "source_clock": "istep = PLUMED_step-1; decay includes equilibration",
                "averaging": "post-update states with istep>nequilib+nonthefly/2 and <=nequilib+nonthefly",
                "plugin": str(plugin),
            },
            indent=2,
        )
        + "\n"
    )
    return out


def build_engine(plumed):
    exe = str(Path(plumed).resolve())
    ext = subprocess.check_output([exe, "info", "--soext"], text=True).strip()
    source = HERE / "SimpleMDInit.cpp"
    library = HERE / ("SimpleMDInit." + ext)
    if not library.exists() or library.stat().st_mtime < source.stat().st_mtime:
        subprocess.run([exe, "mklib", str(source)], cwd=HERE, check=True)
    return library


def run(out, plumed):
    out = Path(out).resolve()
    library = build_engine(plumed)
    with (out / "simplemd.in").open() as inp, (out / "run.log").open("w") as log:
        t = time.monotonic()
        subprocess.run(
            [str(Path(plumed).resolve()), "--load", str(library), "simplemd"],
            cwd=out,
            stdin=inp,
            stdout=log,
            stderr=subprocess.STDOUT,
            check=True,
        )
    p = out / "manifest.json"
    m = json.loads(p.read_text())
    m["runtime_seconds"] = time.monotonic() - t
    m["engine"] = "simplemd with LOAD-able initial polymer-force evaluation"
    p.write_text(json.dumps(m, indent=2) + "\n")


def read_colvar(path):
    with open(path) as f:
        names = f.readline().split()[2:]
    return {n: v for n, v in zip(names, np.loadtxt(path, ndmin=2).T)}


def main():
    p = argparse.ArgumentParser(__doc__)
    p.add_argument("--plumed", required=True)
    p.add_argument("--output", required=True)
    p.add_argument("--route", choices=["coordination", "saxs"], required=True)
    p.add_argument("--phase", choices=["prior", "reference", "joint"], required=True)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--target")
    p.add_argument("--quick", action="store_true")
    p.add_argument("--generate-only", action="store_true")
    a = p.parse_args()
    target = None if not a.target else np.loadtxt(a.target)[:, 1]
    out = generate(a.output, a.route, a.phase, a.seed, target, a.quick)
    if not a.generate_only:
        run(out, a.plumed)


if __name__ == "__main__":
    main()
