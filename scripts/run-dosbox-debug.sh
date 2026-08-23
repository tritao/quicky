#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
game_dir="$(CDPATH= cd -- "${script_dir}/../game" && pwd)"
dosbox_bin="${DOSBOX_DEBUG_BIN:-dosbox-debug}"

if ! command -v "$dosbox_bin" >/dev/null 2>&1; then
    printf 'Debug DOSBox executable not found: %s\n' "$dosbox_bin" >&2
    printf 'Install the dosbox-debug package or set DOSBOX_DEBUG_BIN.\n' >&2
    exit 1
fi

if [[ ! -f "${game_dir}/QUIKY.EXE" ]]; then
    printf 'Missing game file: %s\n' "${game_dir}/QUIKY.EXE" >&2
    exit 1
fi

mount_command="mount c \"${game_dir}\""
exec "$dosbox_bin" \
    -c "$mount_command" \
    -c "c:" \
    -c "cycles ${QUIKY_CYCLES:-16000}" \
    -c "QUIKY.EXE"
