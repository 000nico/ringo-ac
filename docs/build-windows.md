# Build en Windows (MSVC + WDK)

El proyecto se compila nativamente en Windows con MSVC y el WDK, usando
[FindWDK](https://github.com/SergiusTheBest/FindWDK) (`cmake/FindWDK.cmake`) y la
funcion `wdk_add_driver` para el driver.

## Estructura

```text
driver/   kernel-mode driver  -> rngdrv.sys
client/   user-mode client    -> ringo_client.exe
server/   backend HTTP + db   -> ringo-server.exe
cmake/    FindWDK.cmake, wdk-shim/warning.h
scripts/  build-all.ps1, sign-driver.ps1, fetch-wdk.ps1, gen-compile-commands.ps1
```

## Requisitos

- Windows 10/11 x64.
- Visual Studio 2022/2026 con la workload "Desktop development with C++".
- CMake >= 3.25 (probado con 4.4).
- Git (FetchContent lo usa para SQLiteCpp y nlohmann/json).
- El WDK. No hace falta instalarlo: `scripts/fetch-wdk.ps1` baja el paquete
  NuGet `Microsoft.Windows.WDK.x64` y lo extrae en `toolchains/wdk/c`.
  Si ya tenes el WDK instalado, definí `WDKContentRoot` y se usara ese.
- `signtool.exe` viene con el Windows SDK (solo para firmar).

## Compilar todo

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-all.ps1
```

Opciones:

```text
-Configuration Debug|Release   (default: Debug)
-SkipDriver | -SkipClient | -SkipServer
-SkipSign                      no firma el driver
-EnableTestSigning             habilita `bcdedit /set testsigning on` (admin)
```

El script baja el WDK si falta, configura CMake, compila `driver`, `client` y
`ringo-server`, y firma `rngdrv.sys`. Salidas en `build/bin`:

```text
build/bin/rngdrv.sys
build/bin/ringo_client.exe
build/bin/ringo-server.exe
```

## CMake a mano

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target driver client ringo-server
```

O con los presets: `cmake --preset windows` + `cmake --build --preset release`.

## clangd / Zed (IntelliSense)

Zed resuelve C/C++ con clangd. El `.clangd` de la raiz le indica usar la base de
datos de compilacion de `build-index/`:

```yaml
CompileFlags:
  CompilationDatabase: build-index
```

Regenerala cada vez que agregues o muevas archivos `.cpp`. Si no, clangd inventa
los flags y marca falsos errores (`PVOID`/`ULONG`/`UCHAR` desconocidos,
`ntifs.h` no encontrado, etc.):

```powershell
powershell -ExecutionPolicy Bypass -File scripts\gen-compile-commands.ps1
```

Por que un build aparte y no `build/`:

- El generador "Visual Studio" no sabe exportar `compile_commands.json`
  (`CMAKE_EXPORT_COMPILE_COMMANDS` no aplica).
- `build-index/` se configura con Ninja + MSVC (el script pasa por
  `vcvars64.bat`). Ninja a secas agarra el GCC de MSYS2 que esta en el PATH, y
  entonces clangd mezcla headers de MinGW con flags de MSVC.
- Es solo para el LSP: `build-index/` no genera binarios ni reemplaza a `build/`,
  y esta en `.gitignore`.

`.zed/settings.json` solo fija clangd como language server para C y C++. No hace
falta configurar `lsp.clangd.binary.path`: Zed usa su clangd propio y lo
actualiza solo.

## Firma y test signing

El driver debe firmarse para cargarse. `scripts/sign-driver.ps1` crea (si no
existe) un certificado autofirmado de code signing `CN=Ringo Test Driver`,
intenta importarlo en `LocalMachine\Root` y `LocalMachine\TrustedPublisher`
(requiere admin), y firma el `.sys`:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\sign-driver.ps1 `
  -DriverPath build\bin\rngdrv.sys -EnableTestSigning
```

Para un entorno de pruebas:

1. Ejecuta el script como administrador (asi importa el certificado en los
   stores de la maquina; sin admin solo firma, pero el kernel no confiara en el).
2. `-EnableTestSigning` corre `bcdedit /set testsigning on`.
3. Reinicia Windows.
4. Carga el driver (el cliente lo registra e inicia el servicio solo).

Comprobar el estado:

```powershell
bcdedit | Select-String testsigning
```

## Notas

- El driver se enlaza con `wdk_add_driver` (`/driver /subsystem:native`,
  entry `GsDriverEntry`, `WDK::NTOSKRNL`, `WDK::HAL`, `WDK::WMILIB`,
  `WDK::BUFFEROVERFLOWFASTFAILK`, mas `WDK::CNG` y `WDK::WDMSEC`).
- `cmake/wdk-shim/warning.h` es un shim vacio: FindWDK fuerza `/FIwarning.h` y
  las versiones nuevas del WDK ya no traen ese header.
- `client/CMakeLists.txt` desactiva las dependencias opcionales de httplib
  (`HTTPLIB_USE_OPENSSL/ZLIB/BROTLI/ZSTD_IF_AVAILABLE`). Si no, el
  `find_package()` de httplib encuentra las libs de MSYS2 que estan en el PATH y
  arrastra `C:/msys64/ucrt64/include` a la linea de comandos de MSVC. MSVC
  entonces incluye los headers de MinGW (`winnt.h`, `corecrt.h`) y falla con
  `#error No supported target architecture`.
- Cada `.cpp` nuevo bajo `driver/core` hay que sumarlo a `DRIVER_SOURCES` en
  `driver/CMakeLists.txt` (y regenerar el DB de clangd), o el link falla con
  `LNK2019: unresolved external symbol`.
