#!/usr/bin/env python3
"""Independent equations, coordinate forces, LOAD selection and state tests."""

import argparse
import json
import shutil
import subprocess
from pathlib import Path

import benchmark as b
import numpy as np


def driver(exe, d, text, frames, extra=(), dt=0.001, forces=False):
    d = Path(d).resolve()
    d.mkdir(parents=True, exist_ok=True)
    shutil.copy2(b.HERE / "MaxEntAffine.cpp", d / "MaxEntAffine.cpp")
    (d / "plumed.dat").write_text(text)
    (d / "traj.xyz").write_text("".join(b.xyz(r) for r in frames))
    cmd = [
        str(exe),
        "driver",
        "--ixyz",
        "traj.xyz",
        "--timestep",
        str(dt),
        "--kt",
        ".722",
        "--plumed",
        "plumed.dat",
    ]
    if forces:
        cmd += ["--dump-forces", "forces.xyz", "--dump-forces-fmt", "%.17g"]
    with (d / "log").open("w") as f:
        subprocess.run(
            cmd + list(extra), cwd=d, stdout=f, stderr=subprocess.STDOUT, check=True
        )
    return b.read_colvar(d / "COLVAR")


def baseline(r):
    pairs = np.array([r[i] - r[j] for i in range(10) for j in range(i + 1, 10)])
    distances = np.linalg.norm(pairs, axis=1)
    iq = np.sinc(b.Q[:, None] * distances / np.pi).sum(axis=1)
    bonds = np.linalg.norm(np.diff(r, axis=0), axis=1)
    fene = -33.75 * np.log(1 - bonds * bonds / 2.25).sum()
    u = r[:-2] - r[1:-1]
    v = r[2:] - r[1:-1]
    angular = 5 * np.sum(
        np.einsum("ij,ij->i", u, v)
        / np.linalg.norm(u, axis=1)
        / np.linalg.norm(v, axis=1)
    )
    return iq, fene, angular


def numforce(fun, r):
    f = np.zeros_like(r)
    h = 1e-6
    for idx in np.ndindex(r.shape):
        a = r.copy()
        c = r.copy()
        a[idx] += h
        c[idx] -= h
        f[idx] = -(fun(a) - fun(c)) / (2 * h)
    return f


def readforces(p):
    lines = Path(p).read_text().splitlines()
    return np.array(
        [
            [list(map(float, s.split()[1:4])) for s in lines[i + 2 : i + 12]]
            for i in range(0, len(lines), 12)
        ]
    )


def equations(exe, root):
    r = b.initial()
    frames = [r] * 45
    # Nonzero lambda, nonunit A, nonzero B ensure all signs and old-state terms matter.
    x = np.array([3.0, 5.0])
    y = np.array([4.0, 6.0])
    l = np.array([0.2, -0.4])
    A = 1.3
    B = -0.7
    text = "LOAD FILE=MaxEntAffine.cpp\nx: CONSTANT VALUES=3,5\n"
    # CONSTANT vector outputs are x.v-0/x.v-1; use two scalar actions for portability.
    text = "LOAD FILE=MaxEntAffine.cpp\nx: CONSTANT VALUE=3\ny: CONSTANT VALUE=5\n"
    text += "m: MAXENT ARG=x,y AFFINE TYPE=EQUAL AT=4,6 KAPPA=0.0002,0.0002 TAU=50,50 SIGMA=1 PACE=1 PRINT_STRIDE=1 FMT=%.17g AFFINE_LAMBDA=0.2,-0.4 AFFINE_SCALE=1.3 AFFINE_OFFSET=-0.7 AFFINE_KAPPA_SCALE=0.00001 AFFINE_KAPPA_OFFSET=0.000002 TSTART=0.01 TEND=0.03\n"
    text = text.replace(
        "LOAD FILE=MaxEntAffine.cpp\n",
        "LOAD FILE=MaxEntAffine.cpp\nCHECKPOINT FILE=state.cpt STRIDE=7\n",
    )
    text += "PRINT ARG=m.* FILE=COLVAR FMT=%.17g\n"
    out = driver(exe, root / "equations", text, frames)
    rows = []
    ls = []
    aas = []
    bbs = []
    work = 0
    for step in range(45):
        t = (step - 1) * 0.001
        oldl = l.copy()
        oldA = A
        oldB = B
        if t > 0 and t <= 0.03:
            d = 1 + t / 50
            l = oldl + 0.0002 / d * (oldA * x + oldB - y - oldl)
            A = abs(oldA - 0.00001 / d * (oldl @ x + 1 / oldA))
            B = oldB - 0.000002 / d * oldl.sum()
            if t > 0.01:
                ls.append(l.copy())
                aas.append(A)
                bbs.append(B)
        elif t > 0.03:
            l = np.mean(ls, axis=0)
            A = (np.sum(aas) + 1.3) / len(aas)
            B = (np.sum(bbs) - 0.7) / len(bbs)
        work += 0.722 * (l @ (A * x + B) - oldl @ (oldA * x + oldB))
        rows.append([A, B, *l, 0.722 * (l @ (A * x + B)), work])
    expected = np.array(rows)
    actual = np.column_stack(
        [
            out[k]
            for k in [
                "m.scale",
                "m.offset",
                "m.x_coupling",
                "m.y_coupling",
                "m.bias",
                "m.work",
            ]
        ]
    )
    np.testing.assert_allclose(actual, expected, rtol=2e-14, atol=2e-14)
    # Restart inside averaging and after freezing. Full driver trajectory is replayed.
    for cut in [24, 38]:
        d = root / f"restart-{cut}"
        driver(exe, d, text, frames[:cut])
        driver(exe, d, text, frames, extra=["--restart-from-checkpoint", "state.cpt"])
        for name in ["COLVAR", "m.LAGMULT"]:
            assert (d / name).read_bytes() == (
                root / "equations" / name
            ).read_bytes(), str(d / name)
    # Frozen forward-model coefficients must remain fixed while lambda learns.
    fixed = text.replace("AFFINE_KAPPA_SCALE=0.00001", "AFFINE_KAPPA_SCALE=0").replace(
        "AFFINE_KAPPA_OFFSET=0.000002", "AFFINE_KAPPA_OFFSET=0"
    )
    z = driver(exe, root / "fixed", fixed, frames)
    np.testing.assert_array_equal(z["m.scale"], 1.3)
    np.testing.assert_array_equal(z["m.offset"], -0.7)
    # Force a negative Euler proposal: match Davide's absolute-value reflection.
    reflected = text.replace(
        "AFFINE_KAPPA_SCALE=0.00001", "AFFINE_KAPPA_SCALE=10"
    ).replace("TSTART=0.01 TEND=0.03", "")
    z = driver(exe, root / "reflection", reflected, frames[:3])
    proposal = 1.3 - 10 / (1 + 0.001 / 50) * (np.array([0.2, -0.4]) @ x + 1 / 1.3)
    # dot is negative here, choose a positive old lambda to trigger reflection.
    reflected = reflected.replace("AFFINE_LAMBDA=0.2,-0.4", "AFFINE_LAMBDA=0.2,0.4")
    z = driver(exe, root / "reflection-negative", reflected, frames[:3])
    proposal = 1.3 - 10 / (1 + 0.001 / 50) * (np.array([0.2, 0.4]) @ x + 1 / 1.3)
    assert proposal < 0
    np.testing.assert_allclose(z["m.scale"][-1], -proposal, rtol=2e-14)
    # Legacy implementation is copied intact and selected by LOAD; compare to builtin.
    legacy = "x: CONSTANT VALUE=3\ny: CONSTANT VALUE=5\nm: MAXENT ARG=x,y TYPE=EQUAL AT=4,6 KAPPA=0.002,0.003 TAU=50,20 SIGMA=1 PACE=1 PRINT_STRIDE=1 FMT=%.17g TSTART=0.01 TEND=0.03\nPRINT ARG=m.* FILE=COLVAR FMT=%.17g\n"
    driver(exe, root / "legacy-builtin", legacy, frames)
    driver(exe, root / "legacy-loaded", "LOAD FILE=MaxEntAffine.cpp\n" + legacy, frames)
    for name in ["COLVAR", "m.LAGMULT"]:
        assert (root / "legacy-builtin" / name).read_bytes() == (
            root / "legacy-loaded" / name
        ).read_bytes()
    return {
        "max_step_error": float(np.max(np.abs(actual - expected))),
        "restart_inside_average": "bitwise identical",
        "restart_frozen": "bitwise identical",
        "fixed_A_B": "pass",
        "negative_A_reflection": "pass",
        "legacy": "bitwise identical",
    }


def coordinates(exe, root):
    # Extended, bent and compact valid chains generated without any production data.
    frames = []
    for bend in [0.0, 0.35, 0.7]:
        r = np.zeros((10, 3))
        for i in range(1, 10):
            v = np.array([np.cos(bend * i), np.sin(bend * i), 0.13 * np.sin(0.8 * i)])
            r[i] = r[i - 1] + 0.97 * v / np.linalg.norm(v)
        frames.append(r)
    l = np.linspace(-0.4, 0.3, 10)
    A = 1.3
    B = -0.7
    results = {}
    forces = {}
    for route in ["coordination", "saxs"]:
        text = "LOAD FILE=MaxEntAffine.cpp\n" + b.physical(True) + b.observables(route)
        text += b.affine(np.ones(10) * 30, start=100, duration=200).replace(
            "AFFINE TYPE",
            "AFFINE AFFINE_SCALE=1.3 AFFINE_OFFSET=-0.7 AFFINE_LAMBDA="
            + ",".join(map(str, l))
            + " TYPE",
        )
        text += (
            "PRINT ARG="
            + ",".join([*(f"iq{i}" for i in range(1, 11)), "fene", "angular", "m.bias"])
            + " FILE=COLVAR FMT=%.17g\n"
        )
        out = driver(exe, root / route, text, frames, forces=True)
        actual = np.column_stack([out[f"iq{i}"] for i in range(1, 11)])
        expected = np.array([baseline(r)[0] for r in frames])
        np.testing.assert_allclose(actual, expected, atol=1e-11, rtol=1e-13)
        f = readforces(root / route / "forces.xyz")
        forces[route] = f
        fs = []
        for r in frames:

            def energy(c):
                iq, fe, ang = baseline(c)
                return fe + ang + 0.722 * (l @ (A * iq + B))

            fs.append(numforce(energy, r))
        np.testing.assert_allclose(f, fs, atol=2e-6, rtol=2e-7)
        np.testing.assert_allclose(
            out["fene"], [baseline(r)[1] for r in frames], atol=2e-12
        )
        np.testing.assert_allclose(
            out["angular"], [baseline(r)[2] for r in frames], atol=2e-12
        )
        results[route] = {
            "max_value_error": float(np.max(np.abs(actual - expected))),
            "max_force_error": float(np.max(np.abs(f - fs))),
        }
    np.testing.assert_allclose(
        forces["coordination"], forces["saxs"], atol=1e-10, rtol=1e-11
    )
    results["route_force_difference"] = float(
        np.max(np.abs(forces["coordination"] - forces["saxs"]))
    )
    return results


def lj(exe, root):
    results = []
    for distance in [1.1, 2.2, 2.6]:
        d = root / ("lj-" + str(distance))
        d.mkdir(parents=True, exist_ok=True)
        (d / "initial.xyz").write_text(f"2\n100 100 100\nX 0 0 0\nX {distance} 0 0\n")
        (d / "plumed.dat").write_text(
            "d: DISTANCE ATOMS=1,2\nPRINT ARG=d FILE=COLVAR FMT=%.17g\n"
        )
        (d / "simplemd.in").write_text(
            "inputfile initial.xyz\noutputfile final.xyz\ntemperature NVE\ntstep .001\nforcecutoff 2.5\nlistcutoff 3\nnstep 1\nnconfig 1 trajectory.xyz\nnstat 1 energies.dat\n"
        )
        with (d / "simplemd.in").open() as inp, (d / "log").open("w") as log:
            subprocess.run(
                [str(exe), "simplemd"],
                cwd=d,
                stdin=inp,
                stdout=log,
                stderr=subprocess.STDOUT,
                check=True,
            )

        def energy(r):
            return 4 * (r**-12 - r**-6) - 4 * (2.5**-12 - 2.5**-6) if r <= 2.5 else 0.0

        h = 1e-6
        force = (energy(distance + h) - energy(distance - h)) / (2 * h)
        expected = distance - force * 0.001**2
        got = np.loadtxt(d / "COLVAR")[-1]
        assert abs(got - expected) < 1e-12
        potential = np.loadtxt(d / "energies.dat")[3]
        assert (
            abs(potential - energy(got)) < 6e-7
        )  # energies.dat is printed with six decimals
        results.append(
            {
                "distance": distance,
                "position_error": float(abs(got - expected)),
                "energy_error": float(abs(potential - energy(got))),
            }
        )
    return results


def all_q_derivatives(exe, root):
    r = b.initial()
    r[:, 0] = 0.14 * np.sin(np.arange(10))
    maxima = {}
    for route in ["coordination", "saxs"]:
        errors = []
        for i in range(1, 11):
            text = (
                b.observables(route)
                + f"bb: BIASVALUE ARG=iq{i}\nPRINT ARG=iq{i} FILE=COLVAR FMT=%.17g\n"
            )
            d = root / f"{route}-q{i}"
            driver(exe, d, text, [r], forces=True)
            actual = readforces(d / "forces.xyz")[0]
            expected = numforce(lambda c, qindex=i - 1: baseline(c)[0][qindex], r)
            np.testing.assert_allclose(actual, expected, atol=2e-8, rtol=2e-6)
            errors.append(float(np.max(np.abs(actual - expected))))
        maxima[route] = errors
    # Davide skips invalid FENE bonds instead of imposing an infinite wall.
    text = "d: DISTANCE ATOMS=1,2\nf: CUSTOM ARG=d FUNC=-33.75*log(select(1-step(x*x-2.25),1-x*x/2.25,1)) PERIODIC=NO\nfb: BIASVALUE ARG=f\nPRINT ARG=f FILE=COLVAR FMT=%.17g\n"
    frames = []
    for distance in [0.97, 1.49, 1.5, 1.6]:
        rr = b.initial()
        rr[1] = [distance, 0, 0]
        frames.append(rr)
    d = root / "fene-branch"
    out = driver(exe, d, text, frames, forces=True)
    f = readforces(d / "forces.xyz")
    assert np.all(np.isfinite(f))
    np.testing.assert_array_equal(out["f"][-2:], 0)
    np.testing.assert_array_equal(f[-2:], 0)
    return {
        "per_q_max_force_errors": maxima,
        "fene_boundary_branch": "finite and exactly zero at and beyond 1.5",
    }


def initial_polymer_force(exe, root):
    library = b.build_engine(exe)
    d = root / "initial-polymer-force"
    d.mkdir(parents=True, exist_ok=True)
    r = 0.97
    (d / "initial.xyz").write_text(f"2\n100 100 100\nX 0 0 0\nX {r} 0 0\n")
    (d / "plumed.dat").write_text(
        "d: DISTANCE ATOMS=1,2\nf: CUSTOM ARG=d FUNC=-33.75*log(1-x*x/2.25) PERIODIC=NO\nb: BIASVALUE ARG=f\nPRINT ARG=d FILE=COLVAR FMT=%.17g\n"
    )
    (d / "simplemd.in").write_text(
        "inputfile initial.xyz\noutputfile final.xyz\ntemperature NVE\ntstep .001\nforcecutoff 2.5\nlistcutoff 3\nnstep 1\nnconfig 1 trajectory.xyz\nnstat 1 energies.dat\n"
    )
    with (d / "simplemd.in").open() as inp, (d / "log").open("w") as log:
        subprocess.run(
            [str(exe), "--load", str(library), "simplemd"],
            cwd=d,
            stdin=inp,
            stdout=log,
            stderr=subprocess.STDOUT,
            check=True,
        )
    dudr = 4 * (-12 * r**-13 + 6 * r**-7) + 30 * r / (1 - r * r / 2.25)
    expected = r - dudr * 1e-6
    actual = np.loadtxt(d / "COLVAR")[-1, 1]
    assert abs(expected - actual) < 1e-14
    assert (
        "LOAD-able simplemd: initial polymer forces included" in (d / "log").read_text()
    )
    return {
        "expected_distance": expected,
        "actual_distance": float(actual),
        "absolute_error": float(abs(expected - actual)),
    }


def main():
    p = argparse.ArgumentParser(__doc__)
    p.add_argument("--plumed", required=True)
    p.add_argument("--output", required=True)
    a = p.parse_args()
    root = Path(a.output).resolve()
    root.mkdir(parents=True, exist_ok=True)
    exe = Path(a.plumed).resolve()
    report = {
        "coordinates": coordinates(exe, root),
        "equations": equations(exe, root),
        "all_q": all_q_derivatives(exe, root),
        "lj": lj(exe, root),
        "initial_polymer_force": initial_polymer_force(exe, root),
    }
    (root / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
