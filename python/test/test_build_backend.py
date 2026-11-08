"""Check complete native staging and cache reuse without compiling a kernel."""

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


PACKAGE = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "plumed_test_backend", PACKAGE / "build_backend.py"
)
backend = None
if importlib.util.find_spec("setuptools") is not None:
    backend = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(backend)


@unittest.skipIf(backend is None, "setuptools is needed to test the build backend")
class TestNativeStaging(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="plumed native staging ")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.runtime = self.directory / "runtime"
        self.runtime.mkdir()
        for name in ("__init__.py", "cli.py"):
            (self.runtime / name).write_text("# Python helper\n")
        self.source = self.directory / "source"
        self.source.mkdir()
        (self.source / "VERSION.txt").write_text("# version\n2.11-test\n")
        (self.source / "COPYING.LESSER").write_text("test license\n")
        self.files = {
            "bin/plumed": b"\x7fELFtest executable",
            "lib/libplumedKernel.so": b"\x7fELFtest kernel",
            "lib/plumed/src/main/main.cpp": b"// main source\n",
            "include/plumed/tools/Header.h": b"// transformed header\n",
            # A new installer directory must survive staging AND cache reuse.
            "future/resources/new-file": b"future installed data\n",
        }
        self.addCleanup(patch.stopall)
        patch.object(backend, "RUNTIME_PACKAGE", self.runtime).start()
        patch.object(backend, "_check_linux_run_paths").start()
        self.run = patch.object(backend, "_run", side_effect=self.install).start()

    def install(self, command, *, cwd):
        self.assertEqual(command[:2], ["make", "install"])
        self.assertEqual(cwd, self.source)
        self.staging = Path(command[2].removeprefix("DESTDIR="))
        for relative, contents in self.files.items():
            path = self.staging / "plumed_runtime" / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(contents)
            path.chmod(0o755 if relative.startswith("bin/") else 0o644)

    def test_complete_installation_and_manifest(self):
        backend._install_runtime(self.source, ["core"])
        self.run.assert_called_once()
        manifest = json.loads(
            (self.runtime / "lib/plumed/install-manifest.json").read_text()
        )
        self.assertEqual(set(manifest), set(self.files))
        for relative, contents in self.files.items():
            self.assertEqual((self.runtime / relative).read_bytes(), contents)
            self.assertEqual(
                manifest[relative]["executable"], relative.startswith("bin/")
            )
            self.assertEqual(
                manifest[relative]["sha256"],
                None
                if contents.startswith(b"\x7fELF")
                else hashlib.sha256(contents).hexdigest(),
            )

    def test_cache_preserves_future_installed_directories(self):
        backend._install_runtime(self.source, ["core"])
        cache = self.directory / "cache/key"
        backend._store_runtime(cache)
        backend._clear_runtime()
        self.assertEqual(
            {path.name for path in self.runtime.iterdir()}, {"__init__.py", "cli.py"}
        )
        (self.runtime / "stale-resource").write_text("old layout")
        backend._restore_runtime(cache)
        self.assertFalse((self.runtime / "stale-resource").exists())
        self.assertEqual(
            (self.runtime / "future/resources/new-file").read_bytes(),
            self.files["future/resources/new-file"],
        )
        self.assertEqual((self.runtime / "cli.py").read_text(), "# Python helper\n")

    def test_rejects_installer_files_outside_prefix(self):
        install = self.install

        def outside(command, *, cwd):
            install(command, cwd=cwd)
            (self.staging / "outside-file").touch()

        self.run.side_effect = outside
        with self.assertRaisesRegex(RuntimeError, "outside its prefix"):
            backend._install_runtime(self.source, ["core"])

    def test_rejects_installer_conflicts_with_python_helpers(self):
        self.files["cli.py"] = b"conflicting native file"
        with self.assertRaisesRegex(RuntimeError, "conflict with Python helpers"):
            backend._install_runtime(self.source, ["core"])

    def test_wrapper_only_build_removes_stale_native_files(self):
        (self.runtime / "future").mkdir()
        (self.runtime / "future/stale").touch()

        def build(*args):
            self.assertFalse((self.runtime / "future").exists())
            self.assertTrue((self.runtime / "cli.py").is_file())
            return "wrapper.whl"

        with patch.dict(os.environ, {"PLUMED_BUILD_KERNEL": "0"}):
            with patch.object(
                backend._setuptools_backend, "build_wheel", side_effect=build
            ):
                self.assertEqual(
                    backend.build_wheel(str(self.directory)), "wrapper.whl"
                )


if __name__ == "__main__":
    unittest.main()
