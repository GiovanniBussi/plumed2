"""Exercise publication gates against missing, mixed and corrupted artifacts."""

import copy
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import package


class ManifestChecks(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.manifest = {
            "plumed_commit": "a" * 40,
            "snapshot_date": "2026-11-24",
            "package_version": "2026.4.post20261124",
            "matrix": package.matrix(),
        }
        package.write(self.root / "manifest.json", self.manifest)
        self.records = {}
        for item in self.manifest["matrix"]:
            platform, mpi, gpu = (item[k] for k in ("platform", "mpi", "gpu"))
            filename = f"gromacs-plumed-test-{platform}-{mpi}-{gpu}.conda"
            (self.root / filename).write_bytes(b"package fixture")
            record = {
                "platform": platform,
                "mpi": mpi,
                "gpu": gpu,
                "source": copy.deepcopy(self.manifest),
                "package": filename,
                "sha256": package.digest(self.root / filename),
                "index": {
                    "name": "gromacs-plumed",
                    "version": self.manifest["package_version"],
                    "subdir": platform,
                    "build": mpi + "_" + gpu + "_plumed_g" + "a" * 12,
                },
            }
            record_path = self.root / f"{platform}-{mpi}-{gpu}.json"
            package.write(record_path, record)
            self.records[record_path] = record
        self.patch = patch.object(package, "HERE", self.root)
        self.patch.start()
        self.addCleanup(self.patch.stop)

    def test_complete_matrix(self):
        self.assertEqual(len(package.verify(self.root)), 10)
        self.assertEqual(len(package.PLATFORMS), 4)
        self.assertTrue(
            any(
                x["platform"] == "osx-arm64" and x["mpi"] == "openmpi"
                for x in package.matrix()
            )
        )

    def test_missing_variant(self):
        next(iter(self.records)).unlink()
        with self.assertRaises(FileNotFoundError):
            package.verify(self.root)

    def test_corrupt_package(self):
        filename = next(iter(self.records.values()))["package"]
        (self.root / filename).write_bytes(b"corrupt")
        with self.assertRaisesRegex(ValueError, "digest mismatch"):
            package.verify(self.root)

    def test_mixed_source(self):
        path, record = next(iter(self.records.items()))
        record["source"]["plumed_commit"] = "b" * 40
        package.write(path, record)
        with self.assertRaisesRegex(ValueError, "Mixed source"):
            package.verify(self.root)

    def test_unexpected_temporary_kernel(self):
        (self.root / "plumed-temporary.conda").write_bytes(b"never upload")
        with self.assertRaisesRegex(ValueError, "unexpected archives"):
            package.verify(self.root)

    def test_invalid_package_name(self):
        path, record = next(iter(self.records.items()))
        record["package"] = "../escape.conda"
        package.write(path, record)
        with self.assertRaisesRegex(ValueError, "Invalid package path"):
            package.verify(self.root)

    def test_fork_upload_rejected_without_using_token(self):
        with (
            patch.dict(
                os.environ,
                {
                    "GITHUB_EVENT_NAME": "push",
                    "GITHUB_REPOSITORY": "giovannibussi/plumed2",
                },
            ),
            self.assertRaisesRegex(ValueError, "manual upstream master"),
        ):
            package.upload(self.root)


if __name__ == "__main__":
    unittest.main()
