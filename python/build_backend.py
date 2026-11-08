"""PEP 517 backend that builds and stages a small Unix PLUMED runtime."""

from __future__ import annotations

import contextlib
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import time

from setuptools import build_meta as _setuptools_backend


PYTHON_ROOT = Path(__file__).resolve().parent
RUNTIME_PACKAGE = PYTHON_ROOT / "plumed" / "runtime"
# Python helpers are source files; every other runtime entry is generated.
# Do not list installed directories: make install may add new ones in the future.
RUNTIME_HELPERS = {"__init__.py", "cli.py", "__pycache__"}

# files and directories of the PLUMED source tree needed to build the kernel
BUILD_INPUT_FILES = (
    "configure",
    "configure.ac",
    "Makefile",
    "Makefile.conf.in",
    "VERSION.txt",
    "COPYING.LESSER",
    "PEOPLE",
    "sourceme.sh.in",
    "stamp-h.in",
)
BUILD_INPUT_DIRS = ("src", "json", "scripts", "patches", "vim", "fortran")
# build products that might be present in a developer tree
BUILD_PRODUCTS = (
    "*.o",
    "*.d",
    "*.on",
    "*.off",
    "*.a",
    "*.so",
    "*.dylib",
    "*.dll",
    "*.exe",
    "installation.id",
)


def _verbose() -> bool:
    return os.environ.get("PLUMED_BUILD_VERBOSE", "").lower() in {"1", "yes", "true"}


def _show_failure(log: Path) -> None:
    """Print the relevant part of the log of a failed step."""
    lines = log.read_text(errors="replace").splitlines()
    error = re.compile(r"error[: ]|\*\*\*|Traceback|not found", re.IGNORECASE)
    errors = [line for line in lines if error.search(line)]
    if errors:
        print("PLUMED: lines that look like errors:", file=sys.stderr)
        for line in errors[:40]:
            print("  " + line, file=sys.stderr)
    print(f"PLUMED: last lines of {log}:", file=sys.stderr)
    for line in lines[-150:]:
        print("  " + line, file=sys.stderr)
    sys.stderr.flush()


@contextlib.contextmanager
def _captured_output(label: str):
    """Send the output of a build step to a log file and show it only on failure.

    The output of compilers and setuptools (thousands of lines, one for each
    file compiled, copied or added) makes CI logs hard to read. Set
    PLUMED_BUILD_VERBOSE=1 to see everything.
    """
    if _verbose():
        yield
        return
    handle, name = tempfile.mkstemp(prefix="plumed-build-", suffix=".log")
    log = Path(name)
    print(f"PLUMED: {label} (log in {log})", flush=True)
    sys.stderr.flush()
    saved = os.dup(1), os.dup(2)
    os.dup2(handle, 1)
    os.dup2(handle, 2)
    start = time.monotonic()
    succeeded = False
    try:
        yield
        succeeded = True
    finally:
        sys.stdout.flush()
        sys.stderr.flush()
        os.dup2(saved[0], 1)
        os.dup2(saved[1], 2)
        for fd in (*saved, handle):
            os.close(fd)
        if succeeded:
            print(
                f"PLUMED: {label}: done in {time.monotonic() - start:.0f} s", flush=True
            )
            log.unlink(missing_ok=True)
        else:
            print(f"PLUMED: {label}: FAILED", file=sys.stderr)
            _show_failure(log)


def _source_root() -> Path:
    staged = PYTHON_ROOT / "plumed_source"
    return staged if staged.is_dir() else PYTHON_ROOT.parent


def _ignore_build_inputs(directory, names):
    """Exclude native build products from source archives and cache inputs."""
    ignored = set(shutil.ignore_patterns(*BUILD_PRODUCTS)(directory, names))
    if Path(directory).parts[-2:] == ("src", "lib"):
        ignored.update(
            name
            for name in names
            if name in {"install", "plumed"}
            or (name.startswith("plumed-") and not Path(name).suffix)
        )
    return ignored


def _copy_build_inputs(source: Path, destination: Path) -> None:
    destination.mkdir(parents=True)
    for filename in BUILD_INPUT_FILES:
        shutil.copy2(source / filename, destination / filename)
    for dirname in BUILD_INPUT_DIRS:
        shutil.copytree(
            source / dirname,
            destination / dirname,
            symlinks=True,
            ignore=_ignore_build_inputs,
        )
    # the source distribution does not contain this link (see _copy_sdist_source)
    include_link = destination / "src" / "include" / "plumed"
    if not include_link.is_symlink() and not include_link.exists():
        include_link.parent.mkdir(parents=True, exist_ok=True)
        include_link.symlink_to("../")


def _runtime_cache_key(source: Path, modules: list[str]) -> str:
    """Identify a runtime build: sources, build recipe, platform and compiler settings.

    The kernel does not depend on the Python version, so cibuildwheel can build it
    once and reuse it for all the Python versions (see PLUMED_RUNTIME_CACHE).
    """
    digest = hashlib.sha256()
    settings = [sys.platform, platform.machine(), " ".join(modules)]
    for name in (
        "MACOSX_DEPLOYMENT_TARGET",
        "CC",
        "CXX",
        "CFLAGS",
        "CXXFLAGS",
        "LDFLAGS",
    ):
        settings.append(f"{name}={os.environ.get(name, '')}")
    for item in settings:
        digest.update(item.encode() + b"\0")
    digest.update(Path(__file__).read_bytes())
    paths = [source / name for name in BUILD_INPUT_FILES]
    for dirname in BUILD_INPUT_DIRS:
        for directory, directories, files in os.walk(source / dirname):
            ignored = _ignore_build_inputs(directory, [*directories, *files])
            directories[:] = [name for name in directories if name not in ignored]
            paths.extend(
                Path(directory) / name
                for name in [*directories, *files]
                if name not in ignored
            )
    for path in sorted(paths):
        digest.update(str(path.relative_to(source)).encode() + b"\0")
        if path.is_symlink():
            digest.update(os.readlink(path).encode() + b"\0")
        elif path.is_file():
            digest.update(path.read_bytes())
    return digest.hexdigest()[:24]


def _runtime_entries(directory: Path):
    """Select generated native entries without assuming an installation layout."""
    return [path for path in directory.iterdir() if path.name not in RUNTIME_HELPERS]


def _clear_runtime() -> None:
    for path in _runtime_entries(RUNTIME_PACKAGE):
        if path.is_dir() and not path.is_symlink():
            shutil.rmtree(path)
        else:
            path.unlink()


def _copy_entries(source: Path, destination: Path) -> None:
    """Preserve all installed files, permissions and directories."""
    for path in _runtime_entries(source):
        target = destination / path.name
        if path.is_dir():
            shutil.copytree(path, target, symlinks=True)
        else:
            shutil.copy2(path, target)


def _restore_runtime(cached: Path) -> None:
    _clear_runtime()
    _copy_entries(cached / "runtime", RUNTIME_PACKAGE)


def _store_runtime(cached: Path) -> None:
    staging = cached.with_name(cached.name + f".tmp{os.getpid()}")
    shutil.rmtree(staging, ignore_errors=True)
    (staging / "runtime").mkdir(parents=True)
    _copy_entries(RUNTIME_PACKAGE, staging / "runtime")
    (staging / ".complete").touch()
    try:
        staging.rename(cached)
    except OSError:
        # another build stored it in the meantime
        shutil.rmtree(staging, ignore_errors=True)


def _default_modules(source: Path) -> list[str]:
    modules = []
    for module_type in sorted((source / "src").glob("*/module.type")):
        kind = module_type.read_text().strip()
        if kind in {"always", "default-on"}:
            modules.append(module_type.parent.name)
    return modules


def _run(command: list[str], *, cwd: Path, env: dict[str, str] | None = None) -> None:
    print("+", " ".join(command), flush=True)
    subprocess.run(command, cwd=cwd, env=env, check=True)


def _bundle_kernel_enabled() -> bool:
    setting = os.environ.get("PLUMED_BUILD_KERNEL")
    if setting is None and os.environ.get("plumed_default_kernel"):
        return False
    return (setting or "1").lower() not in {"0", "no", "false"}


def _plumed_version(source: Path) -> str:
    # VERSION.txt starts with comment lines
    for line in (source / "VERSION.txt").read_text().splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            return line
    return "unknown"


def _install_runtime(source: Path, modules: list[str]) -> None:
    """Stage the complete normal installation, then add wheel-only metadata.

    Headers, scripts, Fortran sources, editor files and build exports are all
    selected and transformed by make install. A manifest captures that tree
    before packaging, so tests detect any files lost by setuptools or repair.
    """
    with tempfile.TemporaryDirectory(prefix="plumed-wheel-install-") as temporary:
        staging = Path(temporary)
        _run(["make", "install", "DESTDIR=" + str(staging)], cwd=source)
        installed = staging / "plumed_runtime"
        if not installed.is_dir():
            raise RuntimeError("make install did not produce the configured prefix")
        conflicts = RUNTIME_HELPERS.intersection(
            path.name for path in installed.iterdir()
        )
        if conflicts:
            raise RuntimeError(
                f"installed files conflict with Python helpers: {conflicts}"
            )
        # All configure directories must stay inside the private wheel prefix.
        outside = [
            path
            for path in staging.rglob("*")
            if path.is_file() and not path.is_relative_to(installed)
        ]
        if outside:
            raise RuntimeError(
                f"make install placed files outside its prefix: {outside[:5]}"
            )
        links = [path for path in installed.rglob("*") if path.is_symlink()]
        if links:
            raise RuntimeError(
                f"unexpected symbolic links in the installation: {links[:5]}"
            )
        manifest = {}
        for path in sorted(installed.rglob("*")):
            if path.is_file():
                contents = path.read_bytes()
                # auditwheel/delocate can rewrite ELF and Mach-O load commands.
                # Require these files and permissions, and hash all other data.
                image = contents[:4] in {
                    b"\x7fELF",
                    b"\xfe\xed\xfa\xce",
                    b"\xce\xfa\xed\xfe",
                    b"\xfe\xed\xfa\xcf",
                    b"\xcf\xfa\xed\xfe",
                    b"\xca\xfe\xba\xbe",
                    b"\xbe\xba\xfe\xca",
                    b"\xca\xfe\xba\xbf",
                    b"\xbf\xba\xfe\xca",
                }
                manifest[path.relative_to(installed).as_posix()] = {
                    "sha256": None if image else hashlib.sha256(contents).hexdigest(),
                    "executable": bool(path.stat().st_mode & 0o111),
                }
        _clear_runtime()
        _copy_entries(installed, RUNTIME_PACKAGE)

    runtime_lib = RUNTIME_PACKAGE / "lib"
    runtime_bin = RUNTIME_PACKAGE / "bin"
    runtime_root = runtime_lib / "plumed"
    (runtime_root / "install-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n"
    )
    license_dir = RUNTIME_PACKAGE / "licenses"
    license_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source / "COPYING.LESSER", license_dir / "COPYING.LESSER")
    build_info = [
        f"PLUMED version: {_plumed_version(source)}",
        "Build system: configure/make install",
        "Shared kernel: enabled",
        "MPI: disabled",
        "OpenMP: disabled",
        "External BLAS/LAPACK: disabled",
        "External FFTW: disabled",
        "Enabled modules: " + ", ".join(modules),
    ]
    (runtime_root / "build-info.txt").write_text("\n".join(build_info) + "\n")
    if sys.platform == "linux":
        _check_linux_run_paths(runtime_lib, runtime_bin)


def _check_linux_run_paths(runtime_lib: Path, runtime_bin: Path) -> None:
    # libplumed and the executable need libplumedKernel: without a relative run
    # path neither the loader nor auditwheel can find it inside the wheel.
    for binary in (runtime_lib / "libplumed.so", runtime_bin / "plumed"):
        dynamic = subprocess.check_output(["readelf", "-d", str(binary)], text=True)
        expected = "$ORIGIN/../lib" if binary.parent == runtime_bin else "$ORIGIN"
        if expected not in dynamic:
            raise RuntimeError(
                f"{binary.name} was linked without the {expected} run path:\n" + dynamic
            )


def _build_runtime() -> None:
    if sys.platform not in {"linux", "darwin"}:
        raise RuntimeError(
            "Bundled PLUMED kernels are currently supported on Linux and macOS only"
        )
    source_root = _source_root()
    modules = _default_modules(source_root)
    if not modules:
        raise RuntimeError("No always or default-on PLUMED modules were found")

    cache_root = os.environ.get("PLUMED_RUNTIME_CACHE")
    cached = None
    if cache_root:
        cached = Path(cache_root) / _runtime_cache_key(source_root, modules)
        if (cached / ".complete").is_file():
            print(f"PLUMED: reusing the kernel built in {cached}", flush=True)
            _restore_runtime(cached)
            return

    with _captured_output("building the PLUMED kernel"):
        _compile_runtime(source_root, modules)
    if cached:
        _store_runtime(cached)


def _compile_runtime(source_root: Path, modules: list[str]) -> None:
    with tempfile.TemporaryDirectory(prefix="plumed-wheel-build-") as temporary:
        source = Path(temporary) / "plumed"
        _copy_build_inputs(source_root, source)
        prefix = "/plumed_runtime"
        module_option = "--enable-modules=none:" + ":".join(
            "+" + name for name in modules
        )
        configure = [
            "./configure",
            f"--prefix={prefix}",
            module_option,
            "--enable-shared",
            "--disable-static-archive",
            "--disable-static-patch",
            "--disable-python",
            "--disable-doc",
            "--disable-mpi",
            "--disable-openmp",
            "--disable-external-blas",
            "--disable-external-lapack",
            "--disable-fftw",
            "--disable-molfile-plugins",
            "--disable-external-molfile-plugins",
            "--disable-zlib",
            "--disable-rpath",
            "CXXFLAGS=" + (os.environ.get("CXXFLAGS", "") + " -O2").strip(),
            "CFLAGS=" + (os.environ.get("CFLAGS", "") + " -O2").strip(),
        ]
        if sys.platform == "darwin":
            configure.append("--disable-absolute-install-name")
        try:
            _run(configure, cwd=source)
        except subprocess.CalledProcessError:
            # config.log is in the temporary tree: show it before it is removed
            config_log = source / "config.log"
            if config_log.is_file():
                print("+ tail of config.log")
                lines = config_log.read_text(errors="replace").splitlines()
                print("\n".join(lines[-80:]), flush=True)
            raise
        jobs = str(max(1, min(4, os.cpu_count() or 1)))
        _run(["make", "-C", "src/lib", "-j" + jobs], cwd=source)
        _install_runtime(source, modules)


def _copy_sdist_source(destination: Path) -> None:
    _copy_build_inputs(PYTHON_ROOT.parent, destination)
    # src/include/plumed is a link to ../: setuptools would follow it recursively
    # (a 120 MB sdist, with tens of thousands of copies of the sources).
    # _copy_build_inputs creates it again when the kernel is built.
    (destination / "src" / "include" / "plumed").unlink()


def build_sdist(sdist_directory, config_settings=None):
    staged = PYTHON_ROOT / "plumed_source"
    created = not staged.exists()
    try:
        if created:
            _copy_sdist_source(staged)
        os.environ["plumed_force_cython"] = "yes"
        with _captured_output("building the source distribution"):
            return _setuptools_backend.build_sdist(sdist_directory, config_settings)
    finally:
        if created:
            shutil.rmtree(staged, ignore_errors=True)


def build_wheel(wheel_directory, config_settings=None, metadata_directory=None):
    kernel_setting = os.environ.get("PLUMED_BUILD_KERNEL")
    force_cython_setting = os.environ.get("plumed_force_cython")
    # PLUMED's in-tree install passes plumed_default_kernel when building the
    # traditional wrapper-only interface from a staged python/ directory.
    bundle_kernel = _bundle_kernel_enabled()
    if kernel_setting is None and not bundle_kernel:
        os.environ["PLUMED_BUILD_KERNEL"] = "0"
    if bundle_kernel and force_cython_setting is None:
        os.environ["plumed_force_cython"] = "yes"
    try:
        # A wrapper-only rebuild must not pick up resources from an interrupted
        # bundled build in the same source tree.
        _clear_runtime()
        if bundle_kernel:
            _build_runtime()
        with _captured_output("building the Python module and the wheel"):
            return _setuptools_backend.build_wheel(
                wheel_directory, config_settings, metadata_directory
            )
    finally:
        if kernel_setting is None:
            os.environ.pop("PLUMED_BUILD_KERNEL", None)
        if force_cython_setting is None:
            os.environ.pop("plumed_force_cython", None)
        _clear_runtime()


def get_requires_for_build_sdist(config_settings=None):
    return _setuptools_backend.get_requires_for_build_sdist(config_settings)


def get_requires_for_build_wheel(config_settings=None):
    with _captured_output("checking the build requirements"):
        return _setuptools_backend.get_requires_for_build_wheel(config_settings)


def prepare_metadata_for_build_wheel(metadata_directory, config_settings=None):
    with _captured_output("preparing the package metadata"):
        return _setuptools_backend.prepare_metadata_for_build_wheel(
            metadata_directory, config_settings
        )
