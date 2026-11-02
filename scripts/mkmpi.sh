#!/usr/bin/env bash
# Build only the provider adapter: never link a second PLUMED kernel.
set -euo pipefail

MANUAL='Build an MPI provider for a serial-built PLUMED kernel.

Usage: plumed mkmpi --compiler /absolute/path/to/mpicc --out provider.so

Options:
  --compiler PATH        Absolute path to an MPI C or C++ compiler driver
  -o, --out PATH         Output shared library (required)
  --cflag ARG            Add one compiler argument; may be repeated
  --ldflag ARG           Add one linker argument; may be repeated
  -h, --help             Show this help

The adapter is C++98-compatible. The .cpp suffix selects the language; no
standard or language-selection flags are added. Configured shared-library
flags are reused. Extra flags are individual arguments without shell evaluation.
The generated provider uses the selected MPI runtime and no PLUMED library.'

compiler=
output=
cflags=()
ldflags=()
while (($#)); do
  case "$1" in
    --description) echo 'build an MPI provider using an MPI compiler'; exit 0 ;;
    --options) echo '--compiler -o --out --cflag --ldflag -h --help'; exit 0 ;;
    -h|--help) echo "$MANUAL"; exit 0 ;;
    --compiler|-o|--out|--cflag|--ldflag)
      option=$1
      if (($# < 2)); then
        echo "mkmpi: $option requires an argument" >&2
        exit 1
      fi
      case "$option" in
        --compiler) compiler=$2 ;;
        -o|--out) output=$2 ;;
        --cflag) cflags+=("$2") ;;
        --ldflag) ldflags+=("$2") ;;
      esac
      shift 2 ;;
    *) echo "mkmpi: unknown option $1 (see --help)" >&2; exit 1 ;;
  esac
done
if [[ "$compiler" != /* || ! -f "$compiler" || ! -x "$compiler" ]]; then
  echo 'mkmpi: --compiler must name an absolute executable MPI compiler path' >&2
  exit 1
fi
if [[ -z "$output" ]]; then
  echo 'mkmpi: --out is required' >&2
  exit 1
fi
plumed_mpi_root=${PLUMED_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
adapter_dir="$plumed_mpi_root/src/mpiwrap"
for source in mpiwrap_implementation.cpp mpiwrap_implementation.cpp.inc provider-abi.h provider-callbacks.h; do
  if [[ ! -r "$adapter_dir/$source" ]]; then
    echo "mkmpi: missing installed adapter source $adapter_dir/$source" >&2
    exit 1
  fi
done
# Reuse only flags checked by configure, as the mklib command does. The
# selected MPI driver replaces the configured compiler, not its MPI runtime.
options_file="$plumed_mpi_root/src/config/compile_options.sh"
if [[ ! -r "$options_file" ]]; then
  echo "mkmpi: missing configured build options $options_file" >&2
  exit 1
fi
source "$options_file"
if [[ -z "${mpi_provider_link_flags:-}" ]]; then
  echo 'mkmpi: shared-library compiler flags are unavailable; reconfigure PLUMED' >&2
  exit 1
fi
read -r -a configured_compile_flags <<< "${mpi_provider_compile_flags:-}"
read -r -a configured_link_flags <<< "$mpi_provider_link_flags"
# Use the destination filesystem so the final rename is atomic. Keeping the
# output basename also preserves the default library identity on macOS.
build_dir=$(mktemp -d "$(dirname "$output")/.plumed-mkmpi.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT
temporary_output="$build_dir/$(basename "$output")"
"$compiler" ${configured_compile_flags[@]+"${configured_compile_flags[@]}"} -DPLUMED_MPIW_NATIVE_ADAPTER \
  ${cflags[@]+"${cflags[@]}"} "$adapter_dir/mpiwrap_implementation.cpp" \
  "${configured_link_flags[@]}" ${ldflags[@]+"${ldflags[@]}"} -o "$temporary_output" || {
    echo "mkmpi: MPI adapter compilation/link failed with $compiler" >&2
    exit 1
  }
if [[ ! -s "$temporary_output" ]]; then
  echo 'mkmpi: compiler did not produce a provider library' >&2
  exit 1
fi
mv -f "$temporary_output" "$output"
