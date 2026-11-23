"""Download verified GROMACS source and apply the triggered checkout's runtime patch."""

import difflib
import os
import subprocess
import urllib.request
from pathlib import Path

from package import HERE, ROOT, digest, read, write


def main():
    manifest = read(HERE / "manifest.json")
    config = manifest["gromacs"]
    temporary = Path(os.environ["RUNNER_TEMP"])
    archive = temporary / "gromacs.tar.gz"
    urllib.request.urlretrieve(config["source_url"], archive)
    if digest(archive) != config["source_sha256"]:
        raise ValueError("GROMACS archive checksum mismatch")
    source = temporary / "gromacs-source"
    source.mkdir()
    subprocess.run(
        ["tar", "-xzf", str(archive), "--strip-components=1", "-C", str(source)],
        check=True,
    )
    patch = ROOT / "patches" / (config["patch_engine"] + ".diff")
    originals = {}
    for original in sorted(patch.rglob("*.preplumed")):
        relative = str(original.relative_to(patch)).removesuffix(".preplumed")
        if (source / relative).read_bytes() != original.read_bytes():
            raise ValueError("Distributed patch baseline mismatch: " + relative)
        originals[relative] = (source / relative).read_text()
    if not originals:
        raise ValueError("Missing distributed patch originals")
    subprocess.run(
        ["plumed", "patch", "-p", "--runtime", "-e", config["patch_engine"]],
        cwd=source,
        check=True,
    )
    if list(source.rglob("*.rej")):
        raise ValueError("Rejected patch hunks")
    if (source / "Plumed.h").read_bytes() != (
        ROOT / "src/wrapper/Plumed.h"
    ).read_bytes():
        raise ValueError("Patched wrapper differs from triggered PLUMED checkout")
    # Make the staged source independent of the temporary patcher's symlinks.
    for name in ("Plumed.h", "Plumed.inc", "Plumed.cmake"):
        path = source / name
        if path.is_symlink():
            content = path.read_bytes()
            path.unlink()
            path.write_bytes(content)
    diff = "".join(
        "".join(
            difflib.unified_diff(
                text.splitlines(keepends=True),
                (source / relative).read_text().splitlines(keepends=True),
                fromfile="a/" + relative,
                tofile="b/" + relative,
            )
        )
        for relative, text in originals.items()
    )
    (source / "applied-plumed.patch").write_text(diff)
    # New provider/header files are identified in addition to the applied diff.
    manifest["applied_patch_sha256"] = digest(source / "applied-plumed.patch")
    manifest["installed_wrapper_sha256"] = digest(source / "Plumed.h")
    manifest["added_files"] = {
        str(p.relative_to(source)): digest(p)
        for p in (
            source / "Plumed.h",
            source / "Plumed.cmake",
            source / "src/gromacs/mdrun/plumedexchange.h",
        )
        if p.is_file()
    }
    manifest["variant"] = {
        key: os.environ["GROMACS_" + key.upper()] for key in ("MPI", "GPU")
    }
    manifest["tested_runtime"] = read(
        Path(os.environ["CONDA_PREFIX"]) / "share/plumed-gromacs-build/provenance.json"
    )
    metadata = list(
        (Path(os.environ["CONDA_PREFIX"]) / "conda-meta").glob("plumed-*.json")
    )
    if len(metadata) != 1:
        raise ValueError("Expected one temporary PLUMED package")
    runtime = read(metadata[0])
    manifest["tested_runtime_package"] = {
        key: runtime.get(key)
        for key in ("name", "version", "build", "subdir", "sha256")
    }
    write(source / "plumed-provenance.json", manifest)
    with open(os.environ["GITHUB_ENV"], "a") as stream:
        stream.write("GROMACS_SOURCE=" + str(source) + "\n")


if __name__ == "__main__":
    main()
