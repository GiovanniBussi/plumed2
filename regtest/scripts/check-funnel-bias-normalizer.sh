#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TASK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/plumed-funnel-bias.XXXXXX")
trap 'rm -rf "$TASK_DIR"' EXIT

write_reference() {
  cat > "$TASK_DIR/reference" <<'EOF'
#! FIELDS fps.lp fps.ld funnel.bias der_fps.lp der_fps.ld
     1.828000      0.246400     34.802821  -1549.051512   -208.799941
     2.036000      0.336000   1218.894020  -9126.761106  -1506.184544
     2.400000      0.700000   8599.500000      0.000000  -6879.600000
     2.764000      1.064000  23685.409160      0.000000 -14648.949008
     3.128000      1.428000  47118.563189      0.000000 -23884.629353
     3.492000      1.792000  79246.396789 -66358.579183 -34053.428951
EOF
  cat > "$TASK_DIR/output" <<'EOF'
#! FIELDS fps.lp fps.ld funnel.bias der_fps.lp der_fps.ld
     1.828000      0.246400     34.802821  -1549.051511   -208.799941
     2.036000      0.336000   1218.894020  -9126.761106  -1506.184544
     2.400000      0.700000   8599.500000 -23587.200000  -6879.600000
     2.764000      1.064000  23685.409160 -38054.224680 -14648.949008
     3.128000      1.428000  47118.563189      0.000000 -23884.629353
     3.492000      1.792000  79246.396789 -66358.579183 -34053.428951
EOF
}

normalize_and_compare() {
  cp "$TASK_DIR/reference" "$TASK_DIR/reference.normalized"
  cp "$TASK_DIR/output" "$TASK_DIR/output.normalized"
  "$ROOT/regtest/scripts/normalize-funnel-bias.sh" "$TASK_DIR/reference.normalized" 2> "$TASK_DIR/normalizer.err" || return 1
  "$ROOT/regtest/scripts/normalize-funnel-bias.sh" "$TASK_DIR/output.normalized" 2> "$TASK_DIR/normalizer.err" || return 1
  diff -q "$TASK_DIR/reference.normalized" "$TASK_DIR/output.normalized" >/dev/null
}

write_reference
cp "$TASK_DIR/output" "$TASK_DIR/output.raw"
normalize_and_compare
if ! grep -q -- '-23587.200000' "$TASK_DIR/output.raw" || grep -q UNDEFINED "$TASK_DIR/output.raw"; then
  echo 'normalizer changed or failed to preserve the raw output copy' >&2
  exit 1
fi

expect_difference() {
  local description=$1
  local expression=$2
  write_reference
  sed "$expression" "$TASK_DIR/output" > "$TASK_DIR/output.changed"
  mv "$TASK_DIR/output.changed" "$TASK_DIR/output"
  if normalize_and_compare; then
    echo "normalizer failed to detect $description" >&2
    exit 1
  fi
}

expect_difference 'off-kink FS differences above 5-decimal resolution' \
  's/-1549.051511/-1549.051500/'
expect_difference 'a bias change at a kink' \
  's/8599.500000/8599.500001/'
expect_difference 'an LD derivative change at a kink' \
  's/-6879.600000/-6879.600001/'
expect_difference 'coordinate changes at a kink' \
  's/2.400000/2.400001/'
expect_difference 'header changes' \
  's/der_fps.ld/other/'
expect_difference 'missing rows' \
  '$d'
expect_difference 'extra rows' \
  '$a\
     3.856000      2.156000  112000.000000 -80000.000000 -45000.000000'

echo 'funnel BIAS normalization masks only kink FS and 5-decimal FS roundoff'
