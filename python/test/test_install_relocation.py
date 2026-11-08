"""Exercise installed configuration in shared libraries and static MD hosts."""

import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest


PACKAGE = Path(__file__).resolve().parents[1]
SOURCE = (
    PACKAGE / "plumed_source"
    if (PACKAGE / "plumed_source").is_dir()
    else PACKAGE.parent
)


class TestDirectoryOffsets(unittest.TestCase):
    def test_compiler_abi(self):
        """Installed plugin flags preserve either libstdc++ string ABI."""
        compiler = shlex.split(os.environ.get("CXX", "c++"))
        if shutil.which(compiler[0]) is None:
            self.skipTest("C++ compiler unavailable")
        for abi in ("0", "1"):
            flag = "-D_GLIBCXX_USE_CXX11_ABI=" + abi
            result = subprocess.check_output(
                ["sh", str(SOURCE / "src/maketools/cxx-abi"), *compiler, flag],
                text=True,
            )
            self.assertEqual(result.strip(), flag)

    def test_custom_directories(self):
        """Configure directories are lexical paths, not existing build directories."""
        for source, target, expected in (
            ("/p/lib64", "/p/sdk/include", "../sdk/include"),
            ("/p/tools/bin", "/p/lib/arch", "../../lib/arch"),
            ("/p/lib/plumed", "/p/lib/plumed", "."),
            ("/x/../p//lib/", "/p/include/./", "../include"),
            ("/p with spaces/lib", "/p with spaces/include", "../include"),
            ("/p/lib", "/elsewhere/include", "../../elsewhere/include"),
        ):
            with self.subTest(source=source, target=target):
                result = subprocess.check_output(
                    ["sh", str(SOURCE / "src/maketools/relative-path"), source, target],
                    text=True,
                )
                self.assertEqual(result.strip(), expected)


class TestMetadataOffsets(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="plumed metadata ")
        self.addCleanup(self.temporary.cleanup)
        # Repeated spaces, a quote and percent sequences exercise Make's
        # unquoted MAKEFILE_LIST and the escaping of pkg-config arguments.
        self.prefix = Path(self.temporary.name) / "prefix's two  spaces%20"
        self.root = self.prefix / "lib64/plumed-test"
        self.metadata = self.root / "src/lib"
        self.metadata.mkdir(parents=True)

    def makefile(self):
        path = self.metadata / "Plumed.inc.shared"
        path.write_text(
            (SOURCE / "src/maketools/install-make-head").read_text()
            + 'PLUMED_LOAD= "$(PLUMED_LIBRARY_DIR)/libplumed-test.so"\n'
        )
        return path

    def test_make_include(self):
        if not shutil.which("make"):
            self.skipTest("make unavailable")
        include = str(self.makefile()).replace(" ", "\\ ")
        makefile = Path(self.temporary.name) / "consumer.mk"
        makefile.write_text(
            "include " + include + '\nall:\n\t@printf "%s\\n" "$(PLUMED_LIBRARY_DIR)"\n'
        )
        result = subprocess.check_output(["make", "-s", "-f", str(makefile)], text=True)
        self.assertEqual(Path(result.strip()), self.prefix / "lib64")

    def test_cmake_include(self):
        if not shutil.which("cmake"):
            self.skipTest("CMake unavailable")
        makefile = self.makefile()
        cmakefile = makefile.with_name("Plumed.cmake.shared")
        result = subprocess.run(
            ["sh", str(SOURCE / "src/maketools/inc2cmake")],
            input=makefile.read_text(),
            check=True,
            capture_output=True,
            text=True,
        )
        cmakefile.write_text(result.stdout)
        script = Path(self.temporary.name) / "consumer.cmake"
        script.write_text(
            'include("' + str(cmakefile) + '")\n'
            'message(STATUS "RESULT=${PLUMED_LIBRARY_DIR}")\n'
        )
        output = subprocess.check_output(["cmake", "-P", str(script)], text=True)
        self.assertIn("RESULT=" + str(self.prefix / "lib64"), output)

    def test_modulefile_symlink(self):
        if not shutil.which("tclsh"):
            self.skipTest("Tcl unavailable")
        module = self.root / "modulefile"
        text = (SOURCE / "src/lib/modulefile.in").read_text()
        for name, value in {
            "root_to_bin": "../../tools/bin",
            "root_to_include": "../../sdk/include",
            "SOEXT": "so",
            "program_name": "plumed-test",
            "use_absolute_soname": "no",
        }.items():
            text = text.replace("@_" + name + "_@", value)
        module.write_text(text)
        link = Path(self.temporary.name) / "external-modules/plumed"
        link.parent.mkdir()
        link.symlink_to(module)
        script = Path(self.temporary.name) / "consumer.tcl"
        script.write_text(
            "proc module-whatis {args} {}\nproc prepend-path {args} {}\n"
            "proc module-info {args} {return 0}\nproc setenv {args} {}\n"
            "source $env(PLUMED_TEST_MODULE)\nputs $libdir\nputs $bindir\nputs $includedir\n"
        )
        output = subprocess.check_output(
            ["tclsh", str(script)],
            env={**os.environ, "PLUMED_TEST_MODULE": str(link)},
            text=True,
        )
        self.assertEqual(
            [Path(line) for line in output.splitlines()],
            [
                self.prefix.resolve() / "lib64",
                self.prefix.resolve() / "tools/bin",
                self.prefix.resolve() / "sdk/include",
            ],
        )

    def test_pkgconfig_arguments(self):
        if not shutil.which("pkg-config"):
            self.skipTest("pkg-config unavailable")
        directory = self.prefix / "lib64/pkgconfig"
        directory.mkdir(parents=True)
        text = (SOURCE / "src/lib/pkgconfig.in").read_text()
        for name, value in {
            "pc_to_prefix": "../..",
            "pc_to_exec_prefix": "../..",
            "pc_to_include": "../../sdk/include",
            "program_name": "plumed-test",
            "VERSION": "1.0",
            "extra": "",
            "libs": "",
            "private_libs": "",
            "cppflags": "",
            "conflicts": "",
        }.items():
            text = text.replace("@_" + name + "_@", value)
        (directory / "plumed-test.pc").write_text(text)
        output = subprocess.check_output(
            ["pkg-config", "--cflags", "--libs", "plumed-test"],
            env={**os.environ, "PKG_CONFIG_PATH": str(directory)},
            text=True,
        )
        flags = shlex.split(output)
        includes = [Path(flag[2:]).resolve() for flag in flags if flag.startswith("-I")]
        libraries = [
            Path(flag[2:]).resolve() for flag in flags if flag.startswith("-L")
        ]
        self.assertEqual(includes, [self.prefix.resolve() / "sdk/include"])
        self.assertEqual(libraries, [self.prefix.resolve() / "lib64"])

    def test_shell_launcher_symlink(self):
        binary = self.prefix / "tools/bin/plumed-test-config"
        binary.parent.mkdir(parents=True)
        (self.root / "installation.id").write_text("fixture-build\n")
        text = (SOURCE / "src/maketools/install-script-head").read_text()
        for name, value in {
            "BIN_TO_ROOT": "../../lib64/plumed-test",
            "ROOT": "/old/missing/root",
            "ROOT_TO_INCLUDE": "../../sdk/include",
            "ROOT_TO_HTML": "../../share/manual",
            "NAME": "plumed-test",
            "VERSION": "1.0",
        }.items():
            text = text.replace("@" + name + "@", value)
        binary.write_text(
            text + '\nprintf "%s\\n" "$PLUMED_ROOT" "$PLUMED_INCLUDEDIR"\n'
        )
        link = Path(self.temporary.name) / "launcher"
        link.symlink_to(binary)
        env = {
            key: value
            for key, value in os.environ.items()
            if not key.startswith("PLUMED_")
        }
        output = subprocess.check_output(["bash", str(link)], env=env, text=True)
        self.assertEqual(
            [Path(line).resolve() for line in output.splitlines()],
            [self.root.resolve(), self.prefix.resolve() / "sdk/include"],
        )


class TestInstalledDiscovery(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="plumed config relocation ")
        cls.directory = Path(cls.temporary.name).resolve()
        cls.original = cls.directory / "configured prefix"
        cls.relative_root = Path("lib64/architecture/plumed-test")
        cls.relative_include = Path("sdk/include")
        cls.relative_html = Path("share/manual")
        compiler = shlex.split(os.environ.get("CXX", "c++"))
        if shutil.which(compiler[0]) is None:
            cls.temporary.cleanup()
            raise unittest.SkipTest("C++ compiler unavailable")
        cls.addClassCleanup(cls.temporary.cleanup)
        build = cls.directory / "build"
        build.mkdir()
        (build / "version.h").write_text(
            '#define PLUMED_VERSION_SHORT "test"\n'
            '#define PLUMED_VERSION_LONG "test"\n'
            '#define PLUMED_VERSION_GIT "test"\n'
        )
        (build / "Makefile.conf.xxd").write_text("0x23")
        shutil.copy2(SOURCE / "src/config/Config.h", build / "Config.h")
        root = cls.original / cls.relative_root
        fields = {
            "ISINSTALLED": "true",
            "INSTALL_ID": "fixture-build",
            "PLUMED_ROOT": str(root),
            "SOEXT": "dylib" if sys.platform == "darwin" else "so",
            "htmldir": str(cls.original / cls.relative_html),
            "includedir": str(cls.original / cls.relative_include),
            "program_name": "plumed-test",
            "LIB_TO_ROOT": "plumed-test",
            "PREFIX_TO_ROOT": str(cls.relative_root),
            "ROOT_TO_INCLUDE": os.path.relpath(
                cls.original / cls.relative_include, root
            ),
            "ROOT_TO_HTML": os.path.relpath(cls.original / cls.relative_html, root),
        }
        template = (SOURCE / "src/config/Config.inc.in").read_text()
        for name, value in fields.items():
            template = template.replace("@" + name + "@", value)
        (build / "config.cpp").write_text(template)
        (build / "probe.cpp").write_text(
            '#include "Config.h"\n#include <iostream>\n'
            'int main() { std::cout << PLMD::config::getPlumedRoot() << "\\n"'
            ' << PLMD::config::getPlumedIncludedir() << "\\n"'
            ' << PLMD::config::getPlumedHtmldir() << "\\n"; }\n'
        )
        (build / "export.cpp").write_text(
            '#include "Config.h"\n'
            'extern "C" const char* probe() { static const auto root='
            "PLMD::config::getPlumedRoot(); return root.c_str(); }\n"
        )
        flags = ["-std=c++17", "-fPIC", "-D_GNU_SOURCE", "-D__PLUMED_HAS_DLADDR=1"]
        subprocess.run(
            [*compiler, *flags, "-c", "config.cpp", "-o", "config.o"],
            cwd=build,
            check=True,
            capture_output=True,
            text=True,
        )
        cls.host = build / "static-host"
        subprocess.run(
            [
                *compiler,
                "-std=c++17",
                "probe.cpp",
                "config.o",
                "-ldl",
                "-o",
                str(cls.host),
            ],
            cwd=build,
            check=True,
            capture_output=True,
            text=True,
        )
        cls.library = build / (
            "libprobe.dylib" if sys.platform == "darwin" else "libprobe.so"
        )
        subprocess.run(
            [
                *compiler,
                "-std=c++17",
                "-dynamiclib" if sys.platform == "darwin" else "-shared",
                "export.cpp",
                "config.o",
                "-ldl",
                "-o",
                str(cls.library),
            ],
            cwd=build,
            check=True,
            capture_output=True,
            text=True,
        )

    def setUp(self):
        shutil.rmtree(self.original, ignore_errors=True)
        self.case = tempfile.TemporaryDirectory(dir=self.directory)
        self.addCleanup(self.case.cleanup)
        self.addCleanup(shutil.rmtree, self.original, True)
        root = self.original / self.relative_root
        root.mkdir(parents=True)
        (root / "installation.id").write_text("fixture-build\n")
        binary = self.original / "tools/bin/md-host"
        binary.parent.mkdir(parents=True)
        shutil.copy2(self.host, binary)
        shutil.copy2(self.library, root.parent / self.library.name)
        self.env = {
            key: value
            for key, value in os.environ.items()
            if not key.startswith(("PLUMED_", "LD_", "DYLD_"))
        }

    def query(self, host, overrides=None):
        result = subprocess.run(
            [str(host)],
            env={**self.env, **(overrides or {})},
            cwd=self.case.name,
            check=True,
            capture_output=True,
            text=True,
        )
        return [Path(line) for line in result.stdout.splitlines()]

    def test_static_host_follows_moved_prefix(self):
        moved = Path(self.case.name) / "moved prefix"
        self.original.rename(moved)
        self.assertEqual(
            self.query(moved / "tools/bin/md-host"),
            [
                moved / self.relative_root,
                moved / self.relative_include,
                moved / self.relative_html,
            ],
        )

    def test_shared_kernel_and_changed_working_directory(self):
        moved = Path(self.case.name) / "moved prefix"
        self.original.rename(moved)
        relative_library = self.relative_root.parent / self.library.name
        script = """
import ctypes, json, os, sys
library=ctypes.CDLL('./'+sys.argv[1])
os.chdir('/')
library.probe.restype=ctypes.c_char_p
print(json.dumps(library.probe().decode()))
"""
        result = subprocess.run(
            [sys.executable, "-c", script, str(relative_library)],
            cwd=moved,
            env=self.env,
            check=True,
            capture_output=True,
            text=True,
        )
        self.assertEqual(Path(json.loads(result.stdout)), moved / self.relative_root)

    def test_separate_static_host_uses_path_after_installation_moves(self):
        host = Path(self.case.name) / "independent-md"
        shutil.copy2(self.host, host)
        moved = Path(self.case.name) / "moved prefix"
        self.original.rename(moved)
        self.assertEqual(
            self.query(host, {"PATH": str(moved / "tools/bin")})[0],
            moved / self.relative_root,
        )

    def test_moved_host_retains_configured_fallback(self):
        host = Path(self.case.name) / "independent-md"
        shutil.copy2(self.host, host)
        self.assertEqual(self.query(host)[0], self.original / self.relative_root)

    def test_rejects_another_build(self):
        wrong = Path(self.case.name) / "other installation"
        root = wrong / self.relative_root
        root.mkdir(parents=True)
        (root / "installation.id").write_text("another-build\n")
        host = wrong / "bin/md-host"
        host.parent.mkdir()
        shutil.copy2(self.host, host)
        self.assertEqual(self.query(host)[0], self.original / self.relative_root)

    def test_environment_overrides_remain_independent(self):
        host = self.original / "tools/bin/md-host"
        overrides = {"PLUMED_ROOT": "/custom/resources"}
        self.assertEqual(
            self.query(host, overrides),
            [
                Path("/custom/resources"),
                self.original / self.relative_include,
                self.original / self.relative_html,
            ],
        )


if __name__ == "__main__":
    unittest.main()
