"""Exercise the public shell-tool dispatcher with literal paths and arguments."""

import os
from pathlib import Path
import subprocess
import sys

command = sys.argv[1:]
installed = subprocess.run(command + ["--is-installed"], check=False).returncode == 0
root = Path("root ' $(touch injected_root)").resolve()
(root / "scripts").mkdir(parents=True)
(root / "patches").mkdir()
(root / "scripts/probe.sh").write_text(
    """#!/usr/bin/env bash
case "$1" in
  --fail) exit 7 ;;
  --signal) kill -TERM $$ ;;
esac
printf '%s\\0' "$@"
printf '%s\\0' "$PLUMED_ROOT" "$PLUMED_IS_INSTALLED" "$PLUMED_TEST_INHERITED"
printf diagnostic >&2
"""
)
arguments = [
    "two words", "", "single'quote", 'double"quote', "$(touch injected_arg)",
    "; touch injected_semicolon", "back\\slash", "line1\nline2", "*",
]
environment = os.environ.copy()
environment.update(
    PLUMED_ROOT=str(root),
    PLUMED_IS_INSTALLED="parent sentinel",
    PLUMED_TEST_INHERITED="inherited value",
)
result = subprocess.run(
    command + ["probe"] + arguments, env=environment,
    stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False, timeout=30,
)
expected = arguments + [str(root), "yes" if installed else "no", "inherited value"]
assert result.returncode == 0, (result.returncode, result.stderr)
assert result.stdout == b"\0".join(value.encode() for value in expected) + b"\0", result.stdout
assert result.stderr == b"diagnostic", result.stderr
for option in ["--fail", "--signal"]:
    result = subprocess.run(command + ["probe", option], env=environment, check=False, timeout=30)
    assert result.returncode == 1, (option, result.returncode)
assert not any(Path(name).exists() for name in ["injected_root", "injected_arg", "injected_semicolon"])
Path("result").write_text("OK\n")
