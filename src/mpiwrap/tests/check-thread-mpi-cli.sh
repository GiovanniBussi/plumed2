#!/usr/bin/env bash
set -euo pipefail

if [[ $# != 1 || ! -x $1 ]]; then
  echo "usage: $0 /path/to/plumed-cli" >&2
  exit 2
fi

plumed=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
tmp=$(mktemp -d "${TMPDIR:-/private/tmp}/mpiwrap-cli.XXXXXX")
trap 'rm -rf "$tmp"' EXIT

cat > "$tmp/plumed.dat" <<'EOF'
UNITS LENGTH=A ENERGY=kcal/mol TIME=ps
EOF

"$plumed" --has-thread-mpi
for ranks in 1 2 4; do
  (cd "$tmp" && "$plumed" --tmpi-threads "$ranks" driver \
    --plumed plumed.dat --noatoms --multi "$ranks" --timestep 1 \
    --trajectory-stride 1 >"driver.$ranks.out" 2>"driver.$ranks.err")
done

if (cd "$tmp" && "$plumed" --tmpi-threads 1 driver \
    --plumed plumed.dat --noatoms --multi 2 >mismatch.out 2>mismatch.err); then
  echo "launcher accepted a rank count inconsistent with driver --multi" >&2
  exit 1
fi
grep -q "invalid number of processes for multi environment" "$tmp/mismatch.err"

if (cd "$tmp" && "$plumed" --tmpi-threads 2 --no-mpi driver \
    >"conflict.out" 2>"conflict.err"); then
  echo "launcher accepted a contradictory --no-mpi option" >&2
  exit 1
fi
grep -q -- "--tmpi-threads and --no-mpi cannot be requested together" "$tmp/conflict.err"

cat > "$tmp/chdir.dat" <<'EOF'
UNITS LENGTH=A ENERGY=kcal/mol TIME=ps
PLUMED FILE=guest.dat CHDIR=thread-mpi-directory
EOF
cat > "$tmp/guest.dat" <<'EOF'
UNITS LENGTH=A ENERGY=kcal/mol TIME=ps
EOF
if (cd "$tmp" && "$plumed" --tmpi-threads 2 driver \
    --plumed chdir.dat --noatoms --multi 2 --timestep 1 \
    --trajectory-stride 1 >chdir.out 2>chdir.err); then
  echo "thread-MPI unexpectedly accepted PLUMED CHDIR" >&2
  exit 1
fi
grep -q "CHDIR is not supported while a thread-MPI runtime is active" "$tmp/chdir.err"
test ! -e "$tmp/thread-mpi-directory"

echo "thread-MPI CLI launch, rank handoff, and CHDIR guard passed"
