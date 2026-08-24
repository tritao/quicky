#!/usr/bin/env bash
set -euo pipefail

script_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(CDPATH= cd -- "${script_dir}/.." && pwd)"
build_dir="${DOSBOX_AUTOMATION_BUILD_DIR:-${repo_dir}/research/build/dosbox-automation-debug}"
deps_prefix="${DOSBOX_AUTOMATION_DEPS_PREFIX:-${repo_dir}/research/build/sdl-prefix}"
dosbox_bin="${DOSBOX_AUTOMATION_BIN:-${build_dir}/dosbox_with_debugger}"
config_template="${DOSBOX_AUTOMATION_CONFIG_TEMPLATE:-${repo_dir}/research/dosbox-automation.conf}"
portable_config="${build_dir}/dosbox-automation.conf"
target="${QUIKY_AUTOMATION_TARGET-${repo_dir}/game/QUIKY.EXE}"

if [[ ! -x "$dosbox_bin" ]]; then
    printf 'DOSBox Automation debugger not found: %s\n' "$dosbox_bin" >&2
    printf 'Build it first, or set DOSBOX_AUTOMATION_BIN.\n' >&2
    exit 1
fi

if [[ ! -f "$config_template" ]]; then
    printf 'DOSBox Automation config template not found: %s\n' "$config_template" >&2
    exit 1
fi

if [[ ! -f "$portable_config" || "$config_template" -nt "$portable_config" ]]; then
    cp -- "$config_template" "$portable_config"
fi

if [[ -n "$target" && ! -e "$target" ]]; then
    printf 'Automation target not found: %s\n' "$target" >&2
    printf 'Set QUIKY_AUTOMATION_TARGET to another EXE, directory, or an empty value.\n' >&2
    exit 1
fi

if [[ -d "${deps_prefix}/lib" ]]; then
    export LD_LIBRARY_PATH="${deps_prefix}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
fi

export XDG_DATA_HOME="${DOSBOX_AUTOMATION_DATA_HOME:-${repo_dir}}"

if [[ -n "$target" ]]; then
    exec "$dosbox_bin" "$@" "$target"
else
    exec "$dosbox_bin" "$@"
fi
