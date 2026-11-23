"""Translate one shared CUDA fat-binary policy to NVCC or CMake arguments."""

import argparse
import os
import re
import subprocess


def targets(nvcc):
    configured = os.environ.get("NVCC_GENCODE", "")
    real, virtual = set(), set()
    if configured:
        for code in re.findall(r"\bcode=(\[[^\]]+\]|[^\s]+)", configured):
            real.update(re.findall(r"\bsm_(\d+[af]?)\b", code))
            virtual.update(re.findall(r"\bcompute_(\d+[af]?)\b", code))
        if not real:
            raise ValueError("NVCC_GENCODE contains no GPU targets")
    else:
        output = subprocess.check_output([nvcc, "--list-gpu-code"], text=True)
        real.update(re.findall(r"^sm_(\d+)$", output, re.MULTILINE))
        output = subprocess.check_output([nvcc, "--list-gpu-arch"], text=True)
        available = re.findall(r"^compute_(\d+)$", output, re.MULTILINE)
        if not real or not available:
            raise ValueError("NVCC reported no supported numeric GPU architectures")
        virtual.add(max(available, key=int))
    # Keep a PTX fallback even with activation packages listing only native code.
    if not any(v.isdecimal() for v in virtual):
        # Generic PTX is forward compatible; a/f-specific PTX is restricted.
        virtual.add(str(max(int(re.match(r"\d+", v)[0]) for v in real)))

    def order(value):
        return int(re.match(r"\d+", value)[0]), value

    return sorted(real, key=order), sorted(virtual, key=order)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--format", choices=("nvcc", "cmake"), required=True)
    parser.add_argument("--nvcc", default=os.environ.get("CUDACXX", "nvcc"))
    args = parser.parse_args()
    real, virtual = targets(args.nvcc)
    if args.format == "cmake":
        print(";".join([v + "-real" for v in real] + [v + "-virtual" for v in virtual]))
    else:
        print(
            " ".join(
                [f"-gencode=arch=compute_{v},code=sm_{v}" for v in real]
                + [f"-gencode=arch=compute_{v},code=compute_{v}" for v in virtual]
            )
        )


if __name__ == "__main__":
    main()
