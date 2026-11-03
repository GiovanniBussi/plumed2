#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TASK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/plumed-fixzeroes.XXXXXX")
trap 'rm -rf "$TASK_DIR"' EXIT

cat > "$TASK_DIR/input" <<'EOF'
# sign normalization must preserve delimiters and all nonzero values
1 -0.000000 2 -0.0e+10 -0e-3 -0.25 0.00 -nan
  -0.000000    3.000000
two  -0.000000  spaces -0.000000
end -0.000
EOF
cat > "$TASK_DIR/expected" <<'EOF'
# sign normalization must preserve delimiters and all nonzero values
1 0.000000 2 0.0e+10 0e-3 -0.25 0.00 nan
0.000000    3.000000
two 0.000000  spaces 0.000000
end 0.000
EOF

check_sed() {
  local sed_bin=$1 name=$2
  mkdir "$TASK_DIR/$name"
  ln -s "$sed_bin" "$TASK_DIR/$name/sed"
  cat > "$TASK_DIR/input" <<'EOF'
# sign normalization must preserve delimiters and all nonzero values
1 -0.000000 2 -0.0e+10 -0e-3 -0.25 0.00 -nan
  -0.000000    3.000000
two  -0.000000  spaces -0.000000
end -0.000
EOF
  PATH="$TASK_DIR/$name:$PATH" "$ROOT/regtest/scripts/fixzeroes.sh" "$TASK_DIR/input"
  diff -u "$TASK_DIR/expected" "$TASK_DIR/input"

  printf '%6.3f %8.3f %g %g\n' -0.0 -0.0 -0.0 -0.0 > "$TASK_DIR/negative"
  printf '%6.3f %8.3f %g %g\n' 0.0 0.0 0.0 0.0 > "$TASK_DIR/positive"
  PATH="$TASK_DIR/$name:$PATH" "$ROOT/regtest/scripts/fixzeroes.sh" "$TASK_DIR/negative"
  PATH="$TASK_DIR/$name:$PATH" "$ROOT/regtest/scripts/fixzeroes.sh" "$TASK_DIR/positive"
  diff -u "$TASK_DIR/positive" "$TASK_DIR/negative"

}

check_sed "$(command -v sed)" bsd-sed
if command -v gsed >/dev/null 2>&1; then
  check_sed "$(command -v gsed)" gnu-sed
fi
echo 'fixzeroes canonicalizes zero fields and preserves other values under BSD and GNU sed environments'
