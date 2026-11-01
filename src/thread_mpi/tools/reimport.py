#!/usr/bin/env python3
"""Reimport the pinned GROMACS thread-MPI sources with PLUMED namespace isolation."""
import argparse
import pathlib
import re
import shutil
import subprocess
import tarfile
import tempfile
import hashlib
import io

PINNED = "6c69f720aa7378fddfee67f99d51aff21ccfb0a2"
SOURCE_CPP = """alltoall.cpp atomic.cpp barrier.cpp bcast.cpp collective.cpp comm.cpp
errhandler.cpp event.cpp gather.cpp group.cpp list.cpp lock.cpp numa_malloc.cpp
once.cpp p2p_protocol.cpp p2p_send_recv.cpp p2p_wait.cpp profile.cpp
pthreads.cpp reduce.cpp reduce_fast.cpp scan.cpp scatter.cpp tmpi_init.cpp
tmpi_malloc.cpp topology.cpp type.cpp""".split()


def _first_code(lines):
    in_block = False
    directive_continues = False
    for i, line in enumerate(lines):
        s = line.strip()
        if directive_continues:
            directive_continues = s.endswith("\\")
            continue
        if in_block:
            if "*/" in s:
                in_block = False
                s = s.split("*/", 1)[1].strip()
            else:
                continue
        if not s:
            continue
        if s.startswith("/*"):
            in_block = "*/" not in s[2:]
            continue
        if s.startswith("//"):
            continue
        if s.startswith("#"):
            directive_continues = s.endswith("\\")
            continue
        return i
    return len(lines)


def _guard_bounds(lines):
    first = None
    in_block = False
    guard_name = None
    for i, line in enumerate(lines):
        s = line.lstrip()
        if in_block:
            if "*/" in s:
                in_block = False
                s = s.split("*/", 1)[1].strip()
            else:
                continue
        if s.startswith("#"):
            match = re.match(r"#\s*ifndef\s+([A-Za-z_]\w*)", s)
            if match:
                first = i
                guard_name = match.group(1)
            break
        if s.startswith("/*") and "*/" not in s[2:]:
            in_block = True
        if s and not s.startswith(("/*", "*", "//")):
            break
    if first is None:
        return None
    define_line = None
    for i in range(first + 1, len(lines)):
        s = lines[i].strip()
        if not s or s.startswith(("/*", "*", "//")):
            continue
        m = re.match(r"#\s*define\s+([A-Za-z_]\w*)\b", s)
        if m and m.group(1) == guard_name:
            define_line = i
        break
    if define_line is None:
        return None
    depth = 0
    for i in range(first, len(lines)):
        s = lines[i].lstrip()
        if re.match(r"#\s*(if|ifdef|ifndef)\b", s):
            depth += 1
        elif re.match(r"#\s*endif\b", s):
            depth -= 1
            if depth == 0:
                return first, i
    return first, len(lines)


def _guard_end(lines):
    bounds = _guard_bounds(lines)
    return len(lines) if bounds is None else bounds[1]


def _header_scope(lines):
    bounds = _guard_bounds(lines)
    in_block = False
    start = 0 if bounds is None else bounds[0] + 1
    for i in range(start, len(lines)):
        line = lines[i]
        s = line.strip()
        if in_block:
            if "*/" in s:
                in_block = False
                s = s.split("*/", 1)[1].strip()
            else:
                continue
        if not s or s.startswith(("/*", "*", "//")):
            if s.startswith("/*") and "*/" not in s[2:]:
                in_block = True
            continue
        if s.startswith("#"):
            if re.match(r"#\s*(if|ifdef|ifndef)\b", s):
                return i
            continue
        return i
    return len(lines)


def namespace_file(path, is_header):
    text = path.read_text()
    if "namespace PLMD {" in text:
        return
    lines = text.splitlines()
    if is_header and _first_code(lines) == len(lines):
        return
    start = _header_scope(lines)
    if start == len(lines):
        return
    opening = ["namespace PLMD {", "namespace thread_mpi {"]
    original = lines
    # Some upstream headers include architecture-specific helpers after type
    # declarations. Leave every include at global scope; those helpers add
    # their own namespace block when transformed.
    transformed = original[:start] + opening
    for i, line in enumerate(original[start:], start):
        if i > start and re.match(r"\s*#\s*include\b", line):
            transformed.extend(["} // namespace thread_mpi", "} // namespace PLMD", line,
                                "namespace PLMD {", "namespace thread_mpi {"])
        else:
            transformed.append(line)
    lines = transformed
    if is_header:
        end = _guard_end(lines)
        closing = ["} // namespace thread_mpi", "} // namespace PLMD"]
        lines[end:end] = closing
    else:
        lines.extend(["", "} // namespace thread_mpi", "} // namespace PLMD"])
    path.write_text("\n".join(lines) + "\n")


def import_from(root, out):
    archive = subprocess.check_output(["git", "-C", str(root), "archive", PINNED,
                                       "src/external/thread_mpi"])
    with tempfile.TemporaryDirectory(prefix="plumed-thread-mpi-upstream-") as tmp:
        with tarfile.open(fileobj=io.BytesIO(archive)) as tf:
            tf.extractall(tmp)
        src = pathlib.Path(tmp) / "src/external/thread_mpi"
        out.mkdir(parents=True, exist_ok=True)
        for name in ("include", "src"):
            target = out / name
            if target.exists():
                shutil.rmtree(target)
            target.mkdir(parents=True)
        (out / "LICENSE").write_bytes((src / "LICENSE").read_bytes())
        upstream_files = [src / "LICENSE", src / "include/tmpi.h", src / "include/thread_mpi.h"]
        upstream_files += sorted((src / "include/thread_mpi").rglob("*.h"))
        upstream_files += [src / "src" / name for name in SOURCE_CPP]
        upstream_files += sorted((src / "src").glob("*.h"))
        (out / "UPSTREAM.sha256").write_text("".join(
            f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(src)}\n"
            for p in upstream_files))
        (out / "UPSTREAM.txt").write_text(
            "Source: GROMACS 2026.4 (v2026.4)\n"
            f"Commit: {PINNED}\n"
            "Upstream path: src/external/thread_mpi\n"
            "Import transform: tools/reimport.py wraps C++ declarations and definitions in "
            "PLMD::thread_mpi; include directives remain at global scope.\n"
            "No upstream function bodies were otherwise edited.\n"
        )
        shutil.copytree(src / "include", out / "include", dirs_exist_ok=True)
        for p in sorted((src / "src").glob("*.h")):
            shutil.copy2(p, out / "src" / p.name)
        for name in SOURCE_CPP:
            shutil.copy2(src / "src" / name, out / "src" / name)
        atomic = out / "src/atomic.cpp"
        atomic_text = atomic.read_text().replace(
            "int _tMPI_Atomics = 1;",
            "namespace PLMD { namespace thread_mpi {\nint _tMPI_Atomics = 1;\n} }",
        )
        atomic.write_text(atomic_text)
        for p in sorted((out / "include").rglob("*.h")):
            namespace_file(p, True)
        for p in sorted((out / "src").glob("*.h")):
            namespace_file(p, True)
        for p in sorted((out / "src").glob("*.cpp")):
            namespace_file(p, False)
        repo = out.parents[1]
        astyle = repo / "astyle/astyle"
        options = repo / ".astyle.options"
        if astyle.exists() and options.exists():
            for p in sorted(list((out / "include").rglob("*.h")) +
                            list((out / "src").glob("*.h")) +
                            list((out / "src").glob("*.cpp"))):
                formatted = subprocess.check_output(
                    [str(astyle), f"--options={options}"], input=p.read_bytes())
                p.write_bytes(formatted)


def tree_digest(path):
    return {p.relative_to(path).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in path.rglob("*") if p.is_file()}


def import_digest(path):
    files = [path / "LICENSE", path / "UPSTREAM.txt", path / "UPSTREAM.sha256"]
    files.extend(p for subdir in ("include", "src") for p in (path / subdir).rglob("*") if p.is_file())
    return {p.relative_to(path).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in files if p.exists()}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("gromacs", type=pathlib.Path, help="read-only pinned GROMACS checkout")
    ap.add_argument("--output", type=pathlib.Path,
                    default=pathlib.Path(__file__).resolve().parents[1])
    ap.add_argument("--check", action="store_true", help="verify current import is reproducible")
    args = ap.parse_args()
    root = args.gromacs.resolve()
    rev = subprocess.check_output(["git", "-C", str(root), "rev-parse", "v2026.4^{commit}"], text=True).strip()
    if rev != PINNED:
        raise SystemExit(f"expected tag v2026.4 at {PINNED}, found {rev}")
    out = args.output.resolve()
    if out == root or root in out.parents:
        raise SystemExit("output must not be inside the read-only GROMACS tree")
    if args.check:
        with tempfile.TemporaryDirectory(prefix="plumed-thread-mpi-check-") as tmp:
            generated = pathlib.Path(tmp) / "tree"
            import_from(root, generated)
            if import_digest(generated) != import_digest(out):
                raise SystemExit("reimport check failed: generated tree differs from checked-in import")
        print(f"reimport check passed for {PINNED}")
    else:
        import_from(root, out)
        print(f"imported {len(SOURCE_CPP)} C++ files and {len(list(out.rglob('*.h')))} headers from {PINNED}")


if __name__ == "__main__":
    main()
