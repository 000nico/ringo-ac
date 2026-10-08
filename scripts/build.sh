#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
jobs="${RINGO_JOBS:-$(nproc 2>/dev/null || echo 4)}"

usage() {
    cat <<'EOF'
Uso: scripts/build.sh [target] [config]

Targets:
  km       compila solo el driver kernel-mode  (rngdrv.sys)
  um       compila solo el cliente user-mode   (ringo_client.exe)
  server   compila solo el server nativo       (ringo-server, ELF linux)
  all      compila km + um + server

Config (solo afecta a km/um; el server siempre se compila nativo):
  debug    (default)
  release

Ejemplos:
  scripts/build.sh               # = scripts/build.sh all
  scripts/build.sh km
  scripts/build.sh um --release
  scripts/build.sh server
EOF
}

target="${1:-all}"
config="debug"

case "$target" in
    -h|--help)
        usage
        exit 0
        ;;
    debug|release)
        # primer argumento = config
        config="$target"
        target="${2:-all}"
        ;;
    *)
        case "${2:-}" in
            release|-r) config="release" ;;
            debug|-d)   config="debug" ;;
        esac
        ;;
esac

case "$target" in
    km|um|server|all) ;;
    *)
        echo "target inválido: '$target'" >&2
        usage
        exit 2
        ;;
esac

command -v cmake >/dev/null || { echo "Falta cmake; instalá los paquetes de Arch indicados en docs/cmake-linux.md." >&2; exit 1; }
command -v ninja >/dev/null || { echo "Falta ninja; instalá los paquetes de Arch indicados en docs/cmake-linux.md." >&2; exit 1; }

# apunta clangd a los compile_commands.json correctos de cada parte
refresh_cc_db() {
    ln -sfn build/debug/compile_commands.json "$repo_root/compile_commands.json"
    ln -sfn ../build/compile_commands.json "$repo_root/server/compile_commands.json"
}

do_km() {
    cmake --preset "$config"
    cmake --build --preset "$config" --target km_driver -j "$jobs"
}

do_um() {
    cmake --preset "$config"
    cmake --build --preset "$config" --target um_app -j "$jobs"
}

do_server() {
    cmake -S "$repo_root" -B "$repo_root/build" \
        -DRINGO_BUILD_KM=OFF \
        -DRINGO_BUILD_UM=OFF \
        -DRINGO_BUILD_SERVER=ON
    cmake --build "$repo_root/build" --target ringo-server -j "$jobs"
}

case "$target" in
    km)     do_km ;;
    um)     do_um ;;
    server) do_server ;;
    all)
        do_km
        do_um
        do_server
        ;;
esac

refresh_cc_db
echo "build '$target' (config=$config) terminado."