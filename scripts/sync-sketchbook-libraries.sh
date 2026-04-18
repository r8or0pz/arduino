#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preferences_path="${ARDUINO_PREFERENCES_PATH:-${HOME}/.arduino15/preferences.txt}"
sketchbook_root="${ARDUINO_SKETCHBOOK_PATH:-}"

if [[ -z "${sketchbook_root}" && -f "${preferences_path}" ]]; then
    sketchbook_root="$(sed -n 's/^sketchbook\.path=//p' "${preferences_path}" | tail -n 1)"
fi

sketchbook_root="${sketchbook_root:-${HOME}/Arduino}"
libraries_root="${sketchbook_root}/libraries"

mkdir -p "${libraries_root}"

for lib_dir in Backends Connectivity Logger Sensors WeatherStation; do
    ln -sfn "${repo_root}/lib/${lib_dir}" "${libraries_root}/${lib_dir}"
done