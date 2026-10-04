#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
toolchain_root="${RINGO_TOOLCHAIN_ROOT:-"$HOME/toolchains"}"
xwin="${toolchain_root}/xwin/xwin"
xwin_output="${toolchain_root}/xwin-splat"

command -v clang-cl >/dev/null || { echo "Falta clang-cl; instalá los paquetes de Arch indicados en docs/cmake-linux.md." >&2; exit 1; }
command -v lld-link >/dev/null || { echo "Falta lld-link; instalá los paquetes de Arch indicados en docs/cmake-linux.md." >&2; exit 1; }
command -v cmake >/dev/null || { echo "Falta cmake; instalá los paquetes de Arch indicados en docs/cmake-linux.md." >&2; exit 1; }
command -v ninja >/dev/null || { echo "Falta ninja; instalá los paquetes de Arch indicados en docs/cmake-linux.md." >&2; exit 1; }

if [[ ! -x "$xwin" ]]; then
    echo "Falta xwin en $xwin." >&2
    echo "Descargalo desde https://github.com/Jake-Shadle/xwin/releases y extraelo dentro de $toolchain_root/xwin." >&2
    exit 1
fi

if [[ ! -d "$xwin_output" ]]; then
    echo "No existe $xwin_output."
    echo "La extracción requiere aceptar explícitamente la licencia Microsoft." >&2
    echo "$xwin --arch x86_64 --cache-dir ${toolchain_root}/xwin-cache --accept-license splat --include-debug-libs --output $xwin_output"
    exit 2
fi

if [[ ! -d "${toolchain_root}/wdk/c" ]]; then
    echo "Falta el WDK extraído en ${toolchain_root}/wdk/c." >&2
    echo "Descargá Microsoft.Windows.WDK.x64 y extraelo allí; no requiere sudo." >&2
    exit 1
fi

echo "Toolchain disponible en $toolchain_root"
echo "Configurá con: cmake --preset debug"
