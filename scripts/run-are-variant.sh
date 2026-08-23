#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(CDPATH= cd -- "${script_dir}/.." && pwd)"
dosbox_bin="${DOSBOX_BIN:-dosbox}"
variant_archive="${1:-}"
mode="${2:-run}"

if [[ -z "${variant_archive}" || ( "${mode}" != "run" && "${mode}" != "setup" ) ]]; then
    printf 'Usage: %s <variant-NESTLE.DAT> [run|setup]\n' "$0" >&2
    exit 2
fi
if ! command -v "$dosbox_bin" >/dev/null 2>&1; then
    printf 'DOSBox executable not found: %s\n' "$dosbox_bin" >&2
    exit 1
fi
if [[ ! -f "${variant_archive}" ]]; then
    printf 'Variant archive not found: %s\n' "${variant_archive}" >&2
    exit 1
fi

runtime_dir="$(mktemp -d "${TMPDIR:-/tmp}/quiky-are.XXXXXX")"
cleanup() {
    rm -rf -- "${runtime_dir}"
}
trap cleanup EXIT

for runtime_file in DPMI16BI.OVL NESTLE.INI QUIKY.EXE RTM.EXE SCORE.DAT SETUP.EXE; do
    if [[ ! -f "${repo_dir}/game/${runtime_file}" ]]; then
        printf 'Missing bundled runtime file: %s\n' "${repo_dir}/game/${runtime_file}" >&2
        exit 1
    fi
    cp -- "${repo_dir}/game/${runtime_file}" "${runtime_dir}/${runtime_file}"
done
cp -- "${variant_archive}" "${runtime_dir}/NESTLE.DAT"

if [[ "${mode}" == "setup" ]]; then
    program="SETUP.EXE"
else
    program="QUIKY.EXE"
fi
mount_command="mount c \"${runtime_dir}\""
exec "$dosbox_bin" \
    -c "$mount_command" \
    -c "c:" \
    -c "cycles 16000" \
    -c "$program" \
    -c "exit"
