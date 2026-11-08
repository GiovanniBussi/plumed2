"""Regression tests for the source tree staged by the Python build backend."""

import importlib.util
import shutil
import subprocess
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest import mock

PYTHON_DIR = Path(__file__).resolve().parents[1]
SOURCE_ROOT = (
    PYTHON_DIR / "plumed_source"
    if (PYTHON_DIR / "plumed_source").is_dir()
    else PYTHON_DIR.parent
)

# Staging helpers do not need setuptools. Keep this test usable in the Python
# test environment, which need not include the PEP 517 build dependency.
setuptools_stub = types.ModuleType("setuptools")
build_meta_stub = types.ModuleType("setuptools.build_meta")
setuptools_stub.build_meta = build_meta_stub
with mock.patch.dict(
    sys.modules,
    {"setuptools": setuptools_stub, "setuptools.build_meta": build_meta_stub},
):
    spec = importlib.util.spec_from_file_location(
        "plumed_test_build_backend", PYTHON_DIR / "build_backend.py"
    )
    build_backend = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(build_backend)


@unittest.skipUnless(shutil.which("make"), "make is unavailable")
class TestBuildInputs(unittest.TestCase):
    def make_source_tree(self, root):
        (root / "fortran").mkdir(parents=True, exist_ok=True)
        for filename in build_backend.BUILD_INPUT_FILES:
            (root / filename).parent.mkdir(parents=True, exist_ok=True)
            (root / filename).write_text("fixture\n")

        for dirname in build_backend.BUILD_INPUT_DIRS:
            (root / dirname).mkdir(parents=True, exist_ok=True)
            (root / dirname / "fixture.txt").write_text("fixture\n")

        # Use the real Makefile rule which consumes the staged Fortran helper.
        source_lib = root / "src" / "lib"
        source_lib.mkdir(parents=True, exist_ok=True)
        shutil.copy2(SOURCE_ROOT / "src" / "lib" / "Makefile", source_lib)
        (source_lib / "Plumed.inc").write_text("# fixture\n")
        source_maketools = root / "src" / "maketools"
        source_maketools.mkdir(parents=True, exist_ok=True)
        shutil.copy2(SOURCE_ROOT / "src" / "maketools" / "make.rules", source_maketools)
        find_modules = source_maketools / "find_modules.sh"
        find_modules.write_text("#!/bin/sh\nexit 0\n")
        find_modules.chmod(0o755)
        install_dir = source_lib / "install"
        install_dir.mkdir()
        (install_dir / "Plumed.inc").write_text("# fixture\n")

        for name in (
            "PlumedFortran.cmake",
            "plumed.f90",
            "plumed.f90.template",
            "plumed_f08.f90",
            "plumed_f08.f90.template",
        ):
            shutil.copy2(SOURCE_ROOT / "fortran" / name, root / "fortran" / name)
        # Compiler module files are local build products, never portable source.
        (root / "fortran" / "fixture.mod").write_text("compiler output")
        (root / "fortran" / "fixture.smod").write_text("compiler output")
        return root

    def assert_fortran_inputs(self, staged):
        for name in (
            "PlumedFortran.cmake",
            "plumed.f90",
            "plumed_f08.f90",
            "plumed.f90.template",
            "plumed_f08.f90.template",
        ):
            expected = SOURCE_ROOT / "fortran" / name
            self.assertEqual(
                (staged / "fortran" / name).read_bytes(), expected.read_bytes()
            )
        self.assertFalse((staged / "fortran" / "fixture.mod").exists())
        self.assertFalse((staged / "fortran" / "fixture.smod").exists())
        self.assertTrue((staged / "src" / "lib" / "Makefile").is_file())

        # Check the actual src/lib rule can resolve ../../fortran from the staged
        # source layout. `make -q` returns 1 for an out-of-date target, but reports
        # an error (2) if the helper prerequisite is missing.
        result = subprocess.run(
            [
                "make",
                "-q",
                "-f",
                "Makefile",
                "PLUMED_INSTALL=Install",
                "GCCDEP=1",
                "install/Plumed.cmake",
            ],
            check=False,
            cwd=staged / "src" / "lib",
            capture_output=True,
            text=True,
        )
        self.assertNotEqual(result.returncode, 2, result.stdout + result.stderr)
        self.assertNotIn("No rule to make target", result.stderr)

    def test_copy_build_inputs_stages_fortran_sources_and_helper(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = self.make_source_tree(Path(temporary) / "source")
            staged = Path(temporary) / "staged"
            build_backend._copy_build_inputs(root, staged)
            self.assert_fortran_inputs(staged)

    def test_copy_sdist_source_stages_fortran_sources_and_helper(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = self.make_source_tree(Path(temporary) / "source")
            python_root = root / "python"
            python_root.mkdir()
            staged = Path(temporary) / "sdist-source"
            with mock.patch.object(build_backend, "PYTHON_ROOT", python_root):
                build_backend._copy_sdist_source(staged)
            self.assert_fortran_inputs(staged)
            self.assertFalse((staged / "src" / "include" / "plumed").exists())


if __name__ == "__main__":
    unittest.main()
