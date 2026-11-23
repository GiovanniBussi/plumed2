"""Immutable source manifests, package coverage and test-label publication."""

import argparse
import datetime
import functools
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
PLATFORMS = {
    "linux-64": "ubuntu-24.04",
    "linux-aarch64": "ubuntu-24.04-arm",
    "osx-64": "macos-15-intel",
    "osx-arm64": "macos-15",
}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    return json.loads(Path(path).read_text())


def write(path, data):
    Path(path).write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")


def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


@functools.lru_cache(maxsize=1)
def cuda_versions():
    return subprocess.check_output(
        [sys.executable, str(ROOT / "conda/gpu-plan.py"), "--cuda-versions"], text=True
    ).split()


def cuda_variants():
    return ["cpu"] + ["cuda" + v.replace(".", "") for v in cuda_versions()]


def matrix():
    return [
        {"platform": platform, "os": runner, "mpi": mpi, "gpu": gpu}
        for platform, runner in PLATFORMS.items()
        for gpu in (cuda_variants() if platform == "linux-64" else ["cpu"])
        for mpi in ("tmpi", "openmpi")
    ]


def prepare():
    if git("status", "--porcelain", "--untracked-files=no"):
        raise ValueError("Refusing to identify a modified source tree as a Git commit")
    if git(
        "ls-files",
        "--others",
        "--exclude-standard",
        "--",
        "src",
        "patches",
        "conda/gromacs",
    ):
        raise ValueError(
            "Untracked patch/build source cannot be identified by a Git commit"
        )
    sha = git("rev-parse", "HEAD")
    if not re.fullmatch("[a-f0-9]{40}", sha):
        raise ValueError("Expected full Git commit")
    date = datetime.datetime.now(datetime.timezone.utc).date()
    config = read(HERE / "config.json")
    source_version = next(
        line.strip()
        for line in (ROOT / "VERSION.txt").read_text().splitlines()
        if line.strip() and not line.startswith("#")
    )
    files = [ROOT / "patches" / (config["patch_engine"] + ".config")]
    files += sorted((ROOT / "patches" / (config["patch_engine"] + ".diff")).rglob("*"))
    files += [
        ROOT / "src/wrapper/Plumed.h",
        ROOT / "patches/patch.sh",
        ROOT / "conda/cuda-targets.py",
    ]
    manifest = {
        "schema": 1,
        "snapshot_date": date.isoformat(),
        "package_version": config["version"] + ".post" + date.strftime("%Y%m%d"),
        "plumed_package_version": date.strftime("%Y.%m.%d"),
        "plumed_version": source_version,
        "plumed_commit": sha,
        "plumed_commit_date": git("show", "-s", "--format=%cI", "HEAD"),
        "repository": os.environ.get("GITHUB_REPOSITORY", "plumed/plumed2"),
        "build_number": int(os.environ.get("GITHUB_RUN_ID", "0")) * 1000
        + int(os.environ.get("GITHUB_RUN_ATTEMPT", "1")),
        "workflow_url": "https://github.com/"
        + os.environ.get("GITHUB_REPOSITORY", "plumed/plumed2")
        + "/actions/runs/"
        + os.environ.get("GITHUB_RUN_ID", "0"),
        "patch_mode": "runtime",
        "source_files": {
            str(p.relative_to(ROOT)): digest(p) for p in files if p.is_file()
        },
        "gromacs": config,
        "matrix": matrix(),
        "cuda_versions": cuda_versions(),
    }
    write(HERE / "manifest.json", manifest)
    output = os.environ.get("GITHUB_OUTPUT")
    if output:
        with open(output, "a") as stream:
            stream.write("matrix=" + json.dumps({"include": matrix()}) + "\n")
            stream.write("cuda_variants=" + json.dumps(cuda_variants()) + "\n")
            stream.write(
                "platforms="
                + json.dumps(
                    {
                        "include": [
                            {"platform": p, "os": r} for p, r in PLATFORMS.items()
                        ]
                    }
                )
                + "\n"
            )
    print(json.dumps(manifest, indent=2))


def environment():
    manifest = read(HERE / "manifest.json")
    if manifest["plumed_commit"] != git("rev-parse", "HEAD"):
        raise ValueError("Checkout differs from prepared source commit")
    for relative, expected in manifest["source_files"].items():
        if digest(ROOT / relative) != expected:
            raise ValueError("Source digest mismatch: " + relative)
    entries = {
        "GROMACS_PLUMED_SHA": manifest["plumed_commit"],
        "GROMACS_PLUMED_VERSION": manifest["plumed_package_version"],
        "GROMACS_PACKAGE_VERSION": manifest["package_version"],
        "GROMACS_BUILD_NUMBER": str(manifest["build_number"]),
        "CONDA_BLD_PATH": str(Path(os.environ["RUNNER_TEMP"]) / "gromacs-conda-bld"),
        "CONDA_ALWAYS_COPY": "true",
        "CONDA_CHANNEL_PRIORITY": "strict",
        "CPU_COUNT": "4",
    }
    gpu = os.environ.get("GROMACS_GPU", "cpu")
    if gpu != "cpu":
        version = next(
            v for v in manifest["cuda_versions"] if "cuda" + v.replace(".", "") == gpu
        )
        entries["GROMACS_CUDA_VERSION"] = version
        entries["CONDA_OVERRIDE_CUDA"] = version
    with open(os.environ["GITHUB_ENV"], "a") as stream:
        stream.writelines(f"{key}={value}\n" for key, value in entries.items())


def package_index(path):
    from conda_package_handling.api import extract

    with tempfile.TemporaryDirectory() as directory:
        extract(str(Path(path).resolve()), dest_dir=directory)
        return read(Path(directory) / "info/index.json")


def record(directory, platform, mpi, gpu):
    """Collect just the newly built variant, never the temporary PLUMED kernel."""
    manifest = read(HERE / "manifest.json")
    packages = list(
        Path(os.environ["CONDA_BLD_PATH"]).glob(
            f"{platform}/gromacs-plumed-{manifest['package_version']}-{mpi}_{gpu}_*.conda"
        )
    )
    if len(packages) != 1:
        raise ValueError(f"Expected exactly one GROMACS archive, found {packages}")
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    package = packages[0]
    index = package_index(package)
    if manifest["plumed_commit"][:12] not in index["build"]:
        raise ValueError("Package build string lost patch source identity")
    shutil.copy2(package, directory / package.name)
    write(
        directory / f"{platform}-{mpi}-{gpu}.json",
        {
            "platform": platform,
            "mpi": mpi,
            "gpu": gpu,
            "package": package.name,
            "sha256": digest(package),
            "index": index,
            "source": manifest,
            "gpu_execution_tested": False,
            "runtime_validation": runtime_validation(),
        },
    )


def runtime_validation():
    """Keep actual installed kernel package identities beside numerical results."""
    records = {}
    for name in ("installed", "channel-runtime"):
        prefix = Path(os.environ["RUNNER_TEMP"]) / name
        paths = list((prefix / "conda-meta").glob("plumed-*.json"))
        if not paths:
            continue
        if len(paths) != 1:
            raise ValueError("Ambiguous installed PLUMED runtime")
        index = read(paths[0])
        records[name] = {
            key: index.get(key)
            for key in ("name", "version", "build", "subdir", "sha256", "url")
        }
        source = prefix / "lib/plumed/conda-source.json"
        if source.exists():
            records[name]["source"] = read(source)
    return records


def verify(directory):
    directory = Path(directory)
    manifest = read(HERE / "manifest.json")
    records = []
    for item in manifest["matrix"]:
        p, mpi, gpu = (item[key] for key in ("platform", "mpi", "gpu"))
        record = read(directory / f"{p}-{mpi}-{gpu}.json")
        if record["source"] != manifest:
            raise ValueError("Mixed source manifests")
        if any(record[key] != item[key] for key in ("platform", "mpi", "gpu")):
            raise ValueError("Variant mismatch")
        filename = record["package"]
        if Path(filename).name != filename or not filename.startswith(
            "gromacs-plumed-"
        ):
            raise ValueError("Invalid package path")
        if digest(directory / filename) != record["sha256"]:
            raise ValueError("Package digest mismatch")
        index = record["index"]
        if index["subdir"] != p or index["version"] != manifest["package_version"]:
            raise ValueError("Package platform/version mismatch")
        if (
            index["name"] != "gromacs-plumed"
            or manifest["plumed_commit"][:12] not in index["build"]
        ):
            raise ValueError("Unexpected package identity")
        records.append(record)
    if {p.name for p in directory.glob("*.conda")} != {r["package"] for r in records}:
        raise ValueError("Missing or unexpected archives")
    print(f"Verified {len(records)} packages from {manifest['plumed_commit']}")
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as stream:
            stream.write(
                f"PLUMED patch: `{manifest['plumed_commit']}`; snapshot {manifest['snapshot_date']}.\n\n"
            )
            stream.write(
                "CUDA packages compiled and tested on CPU; GPU execution not tested.\n\n"
            )
            for record in records:
                stream.write(f"- `{record['package']}` — SHA256 `{record['sha256']}`\n")
    return records


def upload(directory):
    if (
        os.environ.get("GITHUB_EVENT_NAME") != "workflow_dispatch"
        or os.environ.get("GITHUB_REF") != "refs/heads/master"
        or os.environ.get("GITHUB_REPOSITORY") != "plumed/plumed2"
    ):
        raise ValueError("Uploads require a manual upstream master run")
    records = verify(directory)
    token = os.environ["CONDA_UPLOAD_TOKEN"]
    if not token:
        raise ValueError("Missing upload token")
    # Query public metadata without transmitting credentials.
    try:
        with urllib.request.urlopen(
            "https://api.anaconda.org/package/plumed/gromacs-plumed"
        ) as response:
            existing = {f["basename"]: f for f in json.load(response)["files"]}
    except urllib.error.HTTPError as error:
        if error.code != 404:
            raise
        existing = {}
    environment = os.environ.copy()
    environment["ANACONDA_API_TOKEN"] = token
    for record in records:
        basename = record["platform"] + "/" + record["package"]
        old = existing.get(basename)
        if old:
            if old.get("sha256") != record["sha256"] or "test" not in old.get(
                "labels", []
            ):
                raise ValueError("Existing package conflicts: " + basename)
            print("Already published with identical checksum: " + basename)
            continue
        subprocess.run(
            [
                "anaconda",
                "upload",
                "--user",
                "plumed",
                "--label",
                "test",
                str(Path(directory) / record["package"]),
            ],
            check=True,
            env=environment,
        )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "command", choices=("prepare", "environment", "record", "verify", "upload")
    )
    parser.add_argument("--directory", default="packages")
    parser.add_argument("--platform", choices=PLATFORMS)
    parser.add_argument("--mpi", choices=("tmpi", "openmpi"))
    parser.add_argument("--gpu")
    args = parser.parse_args()
    if args.command == "record":
        if not all((args.platform, args.mpi, args.gpu)):
            parser.error("record requires --platform, --mpi and --gpu")
        record(args.directory, args.platform, args.mpi, args.gpu)
    elif args.command in ("verify", "upload"):
        {"verify": verify, "upload": upload}[args.command](args.directory)
    else:
        {"prepare": prepare, "environment": environment}[args.command]()


if __name__ == "__main__":
    main()
