#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
game_dir="$(CDPATH= cd -- "${script_dir}/../game" && pwd)"
dosbox_bin="${DOSBOX_BIN:-dosbox}"
mode="${1:-run}"

case "$mode" in
    run)
        program="QUIKY.EXE"
        ;;
    setup)
        program="SETUP.EXE"
        ;;
    *)
        printf 'Usage: %s [setup]\n' "$0" >&2
        exit 2
        ;;
esac

if ! command -v "$dosbox_bin" >/dev/null 2>&1; then
    printf 'DOSBox executable not found: %s\n' "$dosbox_bin" >&2
    exit 1
fi

if [[ ! -f "${game_dir}/${program}" ]]; then
    printf 'Missing game file: %s\n' "${game_dir}/${program}" >&2
    exit 1
fi

mount_command="mount c \"${game_dir}\""
exec "$dosbox_bin" \
    -c "$mount_command" \
    -c "c:" \
    -c "cycles 16000" \
    -c "$program" \
    -c "exit"
