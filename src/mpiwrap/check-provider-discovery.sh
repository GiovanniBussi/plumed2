#!/usr/bin/env bash
# Exercise the real mkmpi discovery path with a deterministic mock compiler.
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/mpiw-discovery.XXXXXX")
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/scripts" "$tmp/src/config" "$tmp/src/wrapper"
cp "$root/scripts/mkmpi.sh" "$tmp/scripts/"
cp "$root/src/wrapper/Plumed.h" "$tmp/src/wrapper/"
printf 'mpi_provider_link_flags="-shared"\n' > "$tmp/src/config/compile_options.sh"
case $(uname -s) in
  Darwin) shared=-dynamiclib ;;
  *) shared=-shared ;;
esac
"${CXX:-c++}" -std=c++17 -fPIC "$shared" -DMPIW_TEST_PROVIDER \
  -I"$root/src/mpiwrap" -x c++ "$root/src/mpiwrap/test-provider-discovery.cpp.in" \
  -o "$tmp/provider.so"
"${CXX:-c++}" -std=c++17 -pthread -D__PLUMED_MPIWRAP -D__PLUMED_HAS_DLOPEN \
  -I"$root/src/mpiwrap" -x c++ "$root/src/mpiwrap/mpiwrap.cpp" \
  -x c++ "$root/src/mpiwrap/test-provider-discovery.cpp.in" -ldl -o "$tmp/test"
cat > "$tmp/mock-mpicc" <<'COMPILER'
#!/usr/bin/env bash
set -euo pipefail
printf 'compile\n' >> "$MPIW_TEST_ROOT/compiler.log"
while (($#)); do
  if [[ $1 == -o ]]; then cp "$MPIW_TEST_ROOT/provider.so" "$2"; exit; fi
  shift
done
exit 1
COMPILER
chmod +x "$tmp/mock-mpicc"
unset PLUMED_MPI_WRAPPER PLUMED_MPI_WRAPPER_FORCE
export MPIW_TEST_ROOT="$tmp" MPICC="$tmp/mock-mpicc"
"$tmp/test" success
[[ $(wc -l < "$tmp/compiler.log") -eq 1 ]]
# Explicitly failing discovery must leave the registry available for later use.
export MPICC=/usr/bin/false
"$tmp/test" failure
[[ $(wc -l < "$tmp/compiler.log") -eq 2 ]]
# An injected or explicitly selected library must bypass compiler discovery.
"$tmp/test" injection
"$tmp/test" environment
[[ $(wc -l < "$tmp/compiler.log") -eq 2 ]]
echo 'native communicator provider discovery and failure handling passed'
