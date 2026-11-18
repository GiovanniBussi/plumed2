#!/usr/bin/env python3
"""Optional direct comparison with Davide's pinned original numba function."""

import argparse
import json
import os
import sys
from pathlib import Path

import benchmark as b
import numpy as np
from validate import driver


def dynamics(exe, root, target):
    from simpleMD import SimpleMD

    d = root / "joint-dynamics"
    b.generate(d, "coordination", "joint", 0, target, quick=True)
    text = (
        (d / "plumed.dat")
        .read_text()
        .replace("AFFINE_START=10", "AFFINE_START=0.01")
        .replace("TSTART=1 TEND=2", "TSTART=0.01 TEND=0.02")
        .replace("STRIDE=100 FILE", "STRIDE=1 FILE")
    )
    (d / "plumed.dat").write_text(text)
    inp = (
        (d / "simplemd.in")
        .read_text()
        .replace("nstep 13000", "nstep 40")
        .replace("nconfig 1000", "nconfig 1")
        .replace("nstat 100", "nstat 1")
    )
    (d / "simplemd.in").write_text(inp)
    b.run(d, exe)
    model = SimpleMD(
        temperature=0.722,
        tstep=0.001,
        friction=1.0,
        k_fene=30.0,
        maxbond=1.5,
        k_angle=0.0,
        q_SAXS=b.Q,
        cell=np.array([100.0, 100.0, 100.0]),
        positions=b.initial(),
        nstep=40,
        nstat=1,
        nconfig=1,
        nequilib=10,
        nonthefly=20,
        idum=0,
        window_M=5,
        exp_SAXS=target,
        k_SAXS=0.2,
        tau_SAXS=50000.0,
        k_fm=[0.01, 0.002],
        tau_fm=50000.0,
        sigma_SAXS=np.ones(10),
        grad_fm_reg_fun="default",
        fm_SAXS_0=np.array([1.0, 0.0]),
    )
    model.run()
    lines = (d / "trajectory.xyz").read_text().splitlines()
    cpp = np.array(
        [
            [list(map(float, line.split()[1:4])) for line in lines[i + 2 : i + 12]]
            for i in range(0, len(lines), 12)
        ]
    )
    original = np.array([frame[1] for frame in model.trajectory])
    coordinate_error = float(np.max(np.abs(cpp - original)))
    assert coordinate_error < 5.1e-8  # XYZ has seven decimal places
    c = b.read_colvar(d / "COLVAR")
    l = np.column_stack([c[f"m.iq{i}_coupling"][1:] for i in range(1, 11)])
    error = max(
        np.max(np.abs(c["m.scale"][1:] - model.fm_otf[0])),
        np.max(np.abs(c["m.offset"][1:] - model.fm_otf[1])),
        np.max(np.abs(l - model.lambda_otf.T)),
    )
    assert error < 2e-12
    return {
        "max_coordinate_error": coordinate_error,
        "coordinate_print_precision": "7 decimals",
        "max_parameter_error": float(error),
        "steps": 40,
        "includes": "all phase boundaries, initial forces and original RNG",
    }


def main():
    p = argparse.ArgumentParser(__doc__)
    p.add_argument("--original", required=True)
    p.add_argument("--plumed", required=True)
    p.add_argument("--output", required=True)
    a = p.parse_args()
    root = Path(a.output).resolve()
    root.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("NUMBA_CACHE_DIR", str(root / "numba-cache"))
    sys.path.insert(0, str(Path(a.original).resolve()))
    from simpleMD import _grad_fm_reg_fun_default, _update_parameters

    x = np.array([3.0, 5.0])
    target = np.array([4.0, 6.0])
    l = np.array([0.2, -0.4])
    fm = np.array([1.3, -0.7])
    text = "LOAD FILE=MaxEntAffine.cpp\nx: CONSTANT VALUE=3\ny: CONSTANT VALUE=5\nm: MAXENT ARG=x,y AFFINE TYPE=EQUAL AT=4,6 KAPPA=0.0002,0.0002 TAU=50,50 SIGMA=1 PACE=1 PRINT_STRIDE=1 FMT=%.17g AFFINE_LAMBDA=0.2,-0.4 AFFINE_SCALE=1.3 AFFINE_OFFSET=-0.7 AFFINE_KAPPA_SCALE=0.00001 AFFINE_KAPPA_OFFSET=0.000002 TSTART=0.01 TEND=0.03\nPRINT ARG=m.* FILE=COLVAR FMT=%.17g\n"
    out = driver(Path(a.plumed).resolve(), root / "loaded", text, [b.initial()] * 45)
    ls = []
    fms = []
    maxerr = 0.0
    for step in range(45):
        i = step - 1
        if 0 < i <= 30:
            _update_parameters(
                i,
                0.001,
                0.722,
                x,
                target,
                np.ones(2),
                l,
                fm,
                0.2,
                50000.0,
                np.array([0.01, 0.002]),
                50000.0,
                0.0,
                1.0,
                1.0,
                _grad_fm_reg_fun_default,
            )
            if i > 10:
                ls.append(l.copy())
                fms.append(fm.copy())
        elif i > 30:
            l = np.mean(ls, axis=0)
            fm = (np.sum(fms, axis=0) + np.array([1.3, -0.7])) / len(fms)
        got = np.array(
            [
                out[k][step]
                for k in ["m.scale", "m.offset", "m.x_coupling", "m.y_coupling"]
            ]
        )
        maxerr = max(maxerr, float(np.max(np.abs(got - np.r_[fm, l]))))
    assert maxerr < 1e-13
    result = {
        "max_absolute_difference": maxerr,
        "original_prior_function": "_grad_fm_reg_fun_default",
        "original_commit": "4ddf15b808c2602e5b8191a67c2bad01ffa8bd5e",
    }
    result["dynamics"] = dynamics(
        Path(a.plumed).resolve(),
        root,
        np.loadtxt(b.HERE / "targets-original.dat")[:, 1],
    )
    (root / "original-check.json").write_text(json.dumps(result, indent=2) + "\n")
    print(result)


if __name__ == "__main__":
    main()
