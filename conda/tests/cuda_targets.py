"""Validate the common NVCC/CMake target policy without requiring a GPU."""

import importlib.util
import os
import unittest
from pathlib import Path
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    "cuda_targets", Path(__file__).resolve().parents[1] / "cuda-targets.py"
)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class Targets(unittest.TestCase):
    def test_compiler_activation_targets(self):
        flags = "-gencode=arch=compute_80,code=sm_80 -gencode=arch=compute_90,code=[sm_90,compute_90]"
        with patch.dict(os.environ, {"NVCC_GENCODE": flags}):
            self.assertEqual(module.targets("nvcc"), (["80", "90"], ["90"]))

    def test_adds_ptx_fallback_if_activation_omits_it(self):
        with patch.dict(
            os.environ, {"NVCC_GENCODE": "-gencode=arch=compute_80,code=sm_80"}
        ):
            self.assertEqual(module.targets("nvcc"), (["80"], ["80"]))

    def test_activation_preserves_hopper_and_blackwell_targets(self):
        flags = (
            "-gencode arch=compute_89,code=sm_89 "
            "-gencode arch=compute_90a,code=sm_90a "
            "-gencode arch=compute_100f,code=sm_100f "
            "-gencode arch=compute_121f,code=sm_121f "
            "-gencode arch=compute_121,code=compute_121"
        )
        with patch.dict(os.environ, {"NVCC_GENCODE": flags}):
            self.assertEqual(
                module.targets("nvcc"), (["89", "90a", "100f", "121f"], ["121"])
            )

    def test_specialized_ptx_also_gets_generic_forward_fallback(self):
        with patch.dict(
            os.environ,
            {"NVCC_GENCODE": "-gencode arch=compute_90a,code=[sm_90a,compute_90a]"},
        ):
            self.assertEqual(module.targets("nvcc"), (["90a"], ["90", "90a"]))

    def test_nvcc_capabilities_not_a_hardcoded_old_gpu(self):
        with (
            patch.dict(os.environ, {"NVCC_GENCODE": ""}),
            patch.object(
                module.subprocess,
                "check_output",
                side_effect=[
                    "sm_75\nsm_80\nsm_90\nsm_100\nsm_120\nsm_100a\n",
                    "compute_75\ncompute_80\ncompute_90\ncompute_100\ncompute_120\ncompute_100a\n",
                ],
            ),
        ):
            self.assertEqual(
                module.targets("nvcc"), (["75", "80", "90", "100", "120"], ["120"])
            )

    def test_invalid_activation_cannot_silently_narrow_coverage(self):
        with (
            patch.dict(os.environ, {"NVCC_GENCODE": "not-valid"}),
            self.assertRaises(ValueError),
        ):
            module.targets("nvcc")


if __name__ == "__main__":
    unittest.main()
