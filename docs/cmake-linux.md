# Build Linux -> Windows

El build es experimental: usa `clang-cl` y `lld-link` en Linux, headers/libs
del WDK NuGet y el Windows SDK/CRT obtenido por `xwin`. No reemplaza todavía
una validación del driver en Windows.

## Dependencias de Arch

```bash
sudo pacman -S --needed clang lld llvm cmake ninja
```

El WDK usado durante esta migración es `Microsoft.Windows.WDK.x64`
`10.0.28000.2526`. Sus headers y libraries se esperan en
`$HOME/toolchains/wdk/c`. El SDK/CRT de xwin se espera en
`$HOME/toolchains/xwin-splat`, y se obtiene con:

```bash
$HOME/toolchains/xwin/xwin --arch x86_64 \
  --cache-dir $HOME/toolchains/xwin-cache \
  --accept-license splat --include-debug-libs \
  --output $HOME/toolchains/xwin-splat
```

Ese comando acepta una licencia Microsoft: ejecutalo explícitamente en la
máquina donde corresponda. `scripts/bootstrap-linux.sh` no acepta licencias.

Las rutas se pueden cambiar con `RINGO_TOOLCHAIN_ROOT` o con las variables
`RINGO_WDK_ROOT`, `RINGO_SDK_ROOT` y `RINGO_CRT_ROOT` de CMake.

## Compilar

```bash
cmake --preset debug
cmake --build --preset debug
cmake --preset release
cmake --build --preset release
```

Los targets separados son `debug-km` y `debug-um`. Las salidas son:

```text
build/debug/bin/rngdrv.sys
build/debug/bin/ringo_client.exe
build/release/bin/rngdrv.sys
build/release/bin/ringo_client.exe
```

El driver se enlaza como WDM con `/driver`, `/subsystem:native`,
`/entry:DriverEntry`, `/integritycheck` y las libraries
`ntoskrnl.lib`, `hal.lib`, `bufferoverflowfastfailk.lib` y `cng.lib`.
El ejecutable usa `/subsystem:console` y runtime estático (`/MT` o `/MTd`).

## Verificación PE

```bash
llvm-readobj --file-headers build/release/bin/rngdrv.sys
llvm-readobj --file-headers build/release/bin/ringo_client.exe
```

El `.sys` debe mostrar `Subsystem: IMAGE_SUBSYSTEM_NATIVE`. El entry point
se fija explícitamente a `DriverEntry` en el comando de link de CMake.

## INF, catálogo y firma

El `km.inf` actual es un esqueleto y no contiene todavía un modelo PnP/servicio
completo. `stampinf` actualiza metadatos como `DriverVer`; `Inf2Cat` genera el
catálogo `.cat` a partir del INF y los archivos del paquete. Esas herramientas
son de Windows/WDK y quedan como paso manual en la VM o en un host Windows.

La firma no forma parte del build Linux. En Windows, con WDK instalado:

```powershell
.\scripts\sign-driver.ps1 `
  -DriverPath .\rngdrv.sys `
  -CertificatePath .\test-driver.pfx `
  -TimestampUrl https://timestamp.digicert.com
```

El certificado y su contraseña no se guardan en el repositorio.

## Despliegue existente

`deploy.ps1` sigue construyendo con MSBuild y buscando los binarios debajo de
`km/` y `um/`; no se modificó. Los `.bat` originales esperan `km.sys` junto al
propio script (`%~dp0km.sys`). Para usar el artefacto Linux sin modificar esos
scripts, copiá `build/<preset>/bin/rngdrv.sys` al nombre `km.sys` en la VM.
El cliente queda en `build/<preset>/bin/ringo_client.exe`; copiá ese archivo al
Desktop de la VM o al path que uses para la prueba.
