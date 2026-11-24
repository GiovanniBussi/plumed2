#!/usr/bin/env python3
"""Check real configure selection for hyphenated names and legacy expressions."""

import shutil
import subprocess
import tempfile
from pathlib import Path

source = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="plumed-configure-modules-") as scratch:
    root = Path(scratch)
    shutil.copy2(source / "configure", root / "configure")
    (root / "src/maketools").mkdir(parents=True)
    for name in (
        "module-header",
        "module-header.awk",
        "resolve-modules",
        "resolve-modules.awk",
    ):
        shutil.copy2(source / "src/maketools" / name, root / "src/maketools" / name)
    for declaration in (source / "src").glob("*/module.yml"):
        target = root / declaration.relative_to(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(declaration, target)
    for expression, enabled, disabled in (
        ("reset:+isdb-pytorch", {"isdb-pytorch"}, set()),
        ("all:-isdb-pytorch", set(), {"isdb-pytorch"}),
        ("reset+ves-isdb", {"ves"}, {"isdb"}),
        ("none:isdb-pytorch:+pytorch", {"isdb-pytorch", "pytorch"}, {"isdb"}),
    ):
        # Fail deliberately after selection, before any compiler/dependency probes.
        result = subprocess.run(
            ["bash", "configure", "--enable-modules=" + expression + ":missing_probe"],
            cwd=root,
            text=True,
            capture_output=True,
            check=False,
            timeout=30,
        )
        assert result.returncode != 0
        assert "module missing_probe which does not exist" in result.stderr, (
            result.stderr
        )
        for name in enabled:
            assert (root / "src" / (name + ".on")).exists(), (expression, name)
            assert not (root / "src" / (name + ".off")).exists(), (expression, name)
        for name in disabled:
            assert (root / "src" / (name + ".off")).exists(), (expression, name)
            assert not (root / "src" / (name + ".on")).exists(), (expression, name)

    # Stop at the compiler probe after real selection/resolution. This exercises
    # configure without requiring libraries, a build, or a full source copy.
    def resolve(expression, *options):
        return subprocess.run(
            [
                "bash",
                "configure",
                "--enable-modules=" + expression,
                *options,
                "CC=false",
                "CXX=false",
            ],
            cwd=root,
            text=True,
            capture_output=True,
            timeout=30,
            check=False,
        )

    for expression, additions in (
        ("none:ves", {"bias", "cltools"}),
        ("none:dimred", {"gridtools", "mapping", "function", "colvar"}),
        ("none:colvar", set()),
        ("none:isdb-pytorch", {"colvar"}),
        ("none:-bias:ves:+bias", {"cltools"}),
        ("none:-bias:-bias:ves:+bias", {"cltools"}),
        ("none:-bias:ves:bias", {"cltools"}),
        ("none:-bias:reset", set()),
        ("-bias:none:ves", {"bias", "cltools"}),
        ("none:ves:-ves:-bias", set()),
        ("none:ves:reset", set()),
        ("none:ves:none", set()),
        ("none:ves:all", set()),
    ):
        result = resolve(expression)
        assert result.returncode != 0
        assert "compiler cannot create executables" in result.stderr, result.stderr
        warnings = [
            line
            for line in result.stderr.splitlines()
            if "WARNING: enabling module " in line
        ]
        actual = {line.split("enabling module ")[1].split()[0] for line in warnings}
        assert actual == additions, (expression, result.stderr)
        assert len(warnings) == len(additions), (expression, warnings)
        for name in additions:
            assert (root / "src" / (name + ".on")).exists()
            assert not (root / "src" / (name + ".off")).exists()

    for expression, chain in (
        ("none:ves:-bias", "ves -> bias"),
        ("none:-bias:ves", "ves -> bias"),
        ("none:dimred:-colvar", "dimred -> mapping -> colvar"),
    ):
        result = resolve(expression)
        assert result.returncode != 0
        assert chain in result.stderr and "explicitly disabled" in result.stderr, (
            result.stderr
        )
        assert "WARNING: enabling module" not in result.stderr, result.stderr
        # No partial automatic additions on a conflict.
        assert not (root / "src/cltools.on").exists()

    result = resolve("none", "--enable-kenref")
    assert "compiler cannot create executables" in result.stderr, result.stderr
    assert (root / "src/kenref.on").exists() and (root / "src/bias.on").exists()
    assert "enabling module bias required by kenref -> bias" in result.stderr
    result = resolve("none:-bias", "--enable-kenref")
    assert "kenref -> bias" in result.stderr and "explicitly disabled" in result.stderr
    result = resolve("none:-kenref", "--enable-kenref")
    assert "kenref is explicitly disabled" in result.stderr

    # Unknown dependencies must not silently disappear as config support does.
    declaration = root / "src/colvar/module.yml"
    original = declaration.read_text()
    declaration.write_text(
        original.replace("use: [core, tools]", "use: [missing_module]")
    )
    result = resolve("none:colvar")
    assert "colvar -> missing_module: unknown module" in result.stderr, result.stderr
    declaration.write_text(original)

    # A feature probe dropping a dependency must fail the final guard, rather
    # than re-enabling an unavailable module. The reduced fixture needs no build.
    resolve("none:ves")
    (root / "src/bias.on").unlink()
    (root / "src/bias.off").touch()
    result = subprocess.run(
        ["src/maketools/resolve-modules", "src", "", "check"],
        cwd=root,
        text=True,
        capture_output=True,
        timeout=30,
        check=False,
    )
    assert result.returncode != 0
    assert (
        "ves -> bias" in result.stderr
        and "unavailable after feature checks" in result.stderr
    )
    assert not (root / "src/bias.on").exists()

    # The Make links guard must propagate a dependency-check failure.
    (root / "src/maketools/make.module").write_text(
        (source / "src/maketools/make.module").read_text()
    )
    (root / "src/maketools/make.rules").touch()
    for name in ("check_module.sh", "makelinks.sh"):
        shutil.copy2(source / "src/maketools" / name, root / "src/maketools" / name)
    (root / "src/ves/Makefile").write_text("include ../maketools/make.module\n")
    result = subprocess.run(
        ["make", "links"],
        cwd=root / "src/ves",
        text=True,
        capture_output=True,
        timeout=30,
        check=False,
    )
    assert result.returncode != 0, result.stdout + result.stderr
    assert "depends on bias" in result.stdout
    assert not (root / "src/ves/bias").exists()

    obsolete = root / "src/legacy-module/module.type"
    obsolete.parent.mkdir()
    obsolete.write_text("default-on\n")
    result = subprocess.run(
        ["bash", "configure", "--enable-modules=reset"],
        cwd=root,
        text=True,
        capture_output=True,
        timeout=30,
        check=False,
    )
    assert result.returncode != 0
    assert "module.type is obsolete, use module.yml" in result.stderr, result.stderr
print("Hyphenated/legacy selection and obsolete-declaration diagnostics passed")
