#!/usr/bin/env python3
"""Block-aware scientific summaries and SI figures from benchmark output."""

import argparse
import json
import os
from pathlib import Path

import numpy as np

os.environ.setdefault(
    "MPLCONFIGDIR", str(Path(__file__).resolve().parent / ".mpl-cache")
)
import matplotlib

matplotlib.use("Agg")
import benchmark as b
import matplotlib.pyplot as plt


def block_mean_se(x, block=500):
    x = np.asarray(x)
    n = len(x) // block
    means = x[: n * block].reshape((n, block) + x.shape[1:]).mean(axis=1)
    return x.mean(axis=0), means.std(axis=0, ddof=1) / np.sqrt(n)


def summarize(d):
    m = json.loads((d / "manifest.json").read_text())
    c = b.read_colvar(d / "COLVAR")
    # Davide freeze/collection starts for istep>nequilib+nonthefly.
    mask = c["time"] > m["equilibration"] + m["evolution"] + 0.001
    iq = np.column_stack([c[f"iq{i}"][mask] for i in range(1, 11)])
    rg = c["rg"][mask]
    angle = c["a4"][mask]
    bonds = np.column_stack([c[f"d{i}"] for i in range(1, 10)])
    assert np.all(bonds < 1.5), f"FENE boundary crossed: {d}"
    assert all(np.isfinite(v).all() for v in c.values()), f"nonfinite output: {d}"
    r = {
        "route": m["route"],
        "phase": m["phase"],
        "seed": m["seed"],
        "runtime_seconds": m["runtime_seconds"],
        "nframes": len(iq),
        "max_sampled_bond": float(bonds.max()),
        "rg_mean": float(rg.mean()),
        "angle_mean": float(angle.mean()),
    }
    r["rg_block_se"] = float(block_mean_se(rg)[1])
    r["angle_block_se"] = float(block_mean_se(angle)[1])
    r["iq_mean"], r["iq_block_se"] = (x.tolist() for x in block_mean_se(iq))
    energies = np.loadtxt(d / "energies.dat")
    r["mean_temperature"] = float(
        energies[energies[:, 1] > m["equilibration"] + m["evolution"], 2].mean()
    )
    if m["phase"] == "joint":
        r["target"] = m["target"]
        A = c["m.scale"][mask]
        B = c["m.offset"][mask]
        ls = np.column_stack([c[f"m.iq{i}_coupling"][mask] for i in range(1, 11)])
        assert np.ptp(A) == 0 and np.ptp(B) == 0 and np.all(np.ptp(ls, axis=0) == 0)
        assert np.all(c["m.frozen"][mask] == 1)
        r.update(A=float(A[0]), B=float(B[0]), lambda_frozen=ls[0].tolist())
        g = A[:, None] * iq + B[:, None]
        residual = g - np.array(m["target"]) - ls
        r["stationarity_mean"], r["stationarity_block_se"] = (
            x.tolist() for x in block_mean_se(residual)
        )
        r["spectrum_mean"], r["spectrum_block_se"] = (
            x.tolist() for x in block_mean_se(g)
        )
        r["outer_A_residual"] = float(np.dot(ls[0], iq.mean(axis=0)) + 1 / A[0])
        r["outer_B_residual"] = float(ls[0].sum())
        r["A_window_sd"] = float(
            c["m.scale"][
                (c["time"] > m["equilibration"] + m["evolution"] / 2)
                & (c["time"] <= m["equilibration"] + m["evolution"])
            ].std(ddof=1)
        )
        r["B_window_sd"] = float(
            c["m.offset"][
                (c["time"] > m["equilibration"] + m["evolution"] / 2)
                & (c["time"] <= m["equilibration"] + m["evolution"])
            ].std(ddof=1)
        )
    return r, c, mask


def main():
    p = argparse.ArgumentParser(__doc__)
    p.add_argument("--runs", required=True)
    p.add_argument("--output", required=True)
    a = p.parse_args()
    root = Path(a.runs)
    out = Path(a.output)
    out.mkdir(parents=True, exist_ok=True)
    reports = []
    data = {}
    masks = {}
    for d in sorted(root.glob("*/*")):
        if not (d / "manifest.json").exists():
            continue
        m = json.loads((d / "manifest.json").read_text())
        if "runtime_seconds" not in m:
            continue
        r, c, mask = summarize(d)
        reports.append(r)
        data[(r["route"], r["phase"], r["seed"])] = c
        masks[(r["route"], r["phase"], r["seed"])] = mask
    (out / "runs-summary.json").write_text(json.dumps(reports, indent=2) + "\n")
    references = [r["iq_mean"] for r in reports if r["phase"] == "reference"]
    if references:
        np.savetxt(
            out / "targets-plumed.dat",
            np.column_stack([b.Q, 1.5 * np.mean(references, axis=0) + 2, np.ones(10)]),
            header="q I_exp sigma; independent PLUMED reference means, both routes/all seeds",
        )
    joint = [r for r in reports if r["phase"] == "joint"]
    target = (
        np.array(joint[0]["target"])
        if joint
        else np.loadtxt(b.HERE / "targets-original.dat")[:, 1]
    )
    for route in ["coordination", "saxs"]:
        fig, axes = plt.subplots(1, 3, figsize=(12, 3.5))
        for phase, label, color in [
            ("prior", "prior", "C0"),
            ("reference", "reference", "C1"),
            ("joint", "PLUMED joint", "C2"),
        ]:
            rr = [r for r in reports if r["route"] == route and r["phase"] == phase]
            if not rr:
                continue
            vals = np.array(
                [
                    r["spectrum_mean"]
                    if phase == "joint"
                    else np.array(r["iq_mean"]) * (1.5 if phase == "reference" else 1)
                    + (2 if phase == "reference" else 0)
                    for r in rr
                ]
            )
            axes[0].errorbar(
                b.Q,
                vals.mean(axis=0),
                yerr=(
                    vals.std(axis=0, ddof=1) / np.sqrt(len(vals))
                    if len(vals) > 1
                    else np.array(
                        rr[0]["spectrum_block_se"]
                        if phase == "joint"
                        else rr[0]["iq_block_se"]
                    )
                    * (1.5 if phase == "reference" else 1)
                ),
                label=label,
                color=color,
                capsize=2,
            )
            for j, field, bins in [
                (1, "rg", np.linspace(0.7, 3, 65)),
                (2, "a4", np.linspace(0, np.pi, 65)),
            ]:
                arrays = [
                    data[(route, phase, r["seed"])][field][
                        masks[(route, phase, r["seed"])]
                    ]
                    for r in rr
                ]
                axes[j].hist(
                    np.concatenate(arrays),
                    bins=bins,
                    density=True,
                    histtype="step",
                    label=label,
                    color=color,
                )
        axes[0].plot(b.Q, target, "k.", label="target data")
        for ax, xlabel in zip(axes, ["q", "Rg", "central angle (rad)"]):
            ax.set_xlabel(xlabel)
        axes[0].set_ylabel("intensity")
        axes[1].set_ylabel("density")
        axes[2].set_ylabel("density")
        axes[0].legend(fontsize=8)
        fig.suptitle(f"Independent PLUMED reproduction: {route.upper()}")
        fig.tight_layout()
        fig.savefig(out / f"{route}-reproduction.pdf")
        fig.savefig(out / f"{route}-reproduction.png", dpi=160)
        plt.close(fig)
        fig, axes = plt.subplots(3, 1, figsize=(8, 7), sharex=True)
        for seed in [0, -12345, -23456]:
            key = (route, "joint", seed)
            if key not in data:
                continue
            c = data[key]
            mask = c["time"] <= 2010
            axes[0].plot(
                c["time"][mask] - 10, c["m.scale"][mask], label=str(seed), lw=0.8
            )
            axes[1].plot(c["time"][mask] - 10, c["m.offset"][mask], lw=0.8)
            if seed == 0:
                for i in range(1, 11):
                    axes[2].plot(
                        c["time"][mask] - 10, c[f"m.iq{i}_coupling"][mask], lw=0.6
                    )
        for ax, yl in zip(axes, ["A", "B", "lambda (seed 0)"]):
            ax.set_ylabel(yl)
            ax.axvspan(1000, 2000, color="gray", alpha=0.12)
        axes[0].legend(title="seed")
        axes[2].set_xlabel("time since equilibration")
        fig.tight_layout()
        fig.savefig(out / f"{route}-learning.pdf")
        plt.close(fig)
    print(
        json.dumps(
            [
                {
                    k: r[k]
                    for k in ["route", "phase", "seed", "rg_mean", "mean_temperature"]
                    + (["A", "B", "outer_B_residual"] if r["phase"] == "joint" else [])
                }
                for r in reports
            ],
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
