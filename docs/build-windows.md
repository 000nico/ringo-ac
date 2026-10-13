# Building on Windows (MSVC + WDK)

The project compiles natively on Windows using MSVC and the Windows Driver Kit (WDK), using [FindWDK](https://github.com/SergiusTheBest/FindWDK) (`cmake/FindWDK.cmake`) and the `wdk_add_driver` function for the kernel driver.

## Structure

```text
driver/   kernel-mode driver  -> rngdrv.sys
client/   user-mode client    -> ringo_client.exe
server/   backend HTTP + db   -> ringo-server.exe
cmake/    FindWDK.cmake, wdk-shim/warning.h
scripts/  build-all.ps1, sign-driver.ps1, fetch-wdk.ps1, gen-compile-commands.ps1
```

## Requirements

- Windows 10/11 x64
- Visual Studio 2022/2026 with the "Desktop development with C++" workload
- CMake >= 3.25 (tested with 4.4)
- Git (FetchContent is used for SQLiteCpp and nlohmann/json)
- WDK. No need to install it globally: `scripts/fetch-wdk.ps1` downloads the NuGet package `Microsoft.Windows.WDK.x64` and extracts it to `toolchains/wdk/c`. If you already have WDK installed, set `WDKContentRoot` to use that location
- `signtool.exe` is included with the Windows SDK (only required for signing)

## Build everything

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-all.ps1
```

Options:

```text
-Configuration Debug|Release   (default: Debug)
-SkipDriver | -SkipClient | -SkipServer
-SkipSign                      don't sign the driver
-EnableTestSigning             enable `bcdedit /set testsigning on` (requires admin)
```

The script fetches the WDK if missing, configures CMake, builds `driver`, `client` and `ringo-server`, and signs `rngdrv.sys`. Outputs go to `build/bin/`:

```text
build/bin/rngdrv.sys
build/bin/ringo_client.exe
build/bin/ringo-server.exe
```

## Building with CMake directly

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target driver client ringo-server
```

Or use presets: `cmake --preset windows` + `cmake --build --preset release`.

## clangd / Zed (IntelliSense)

Zed uses clangd for C/C++ IntelliSense. The root `.clangd` points clangd to the compilation database in `build-index/`:

```yaml
CompileFlags:
  CompilationDatabase: build-index
```

Regenerate it whenever you add or move `.cpp` files. Otherwise clangd will infer flags and may report false positives (unknown `PVOID`/`ULONG`/`UCHAR`, missing `ntifs.h`, etc.):

```powershell
powershell -ExecutionPolicy Bypass -File scripts\gen-compile-commands.ps1
```

Why a separate build directory (not `build/`):

- The Visual Studio generator cannot export `compile_commands.json` (`CMAKE_EXPORT_COMPILE_COMMANDS` does not apply)
- `build-index/` is configured with Ninja + MSVC (the script runs via `vcvars64.bat`). Running Ninja directly picks up GCC from MSYS2 in PATH, which would mix MinGW headers with MSVC flags
- It's only for LSP: `build-index/` produces no binaries and doesn't replace `build/`; it is listed in `.gitignore`

`.zed/settings.json` only sets clangd as the language server for C/C++. No need to configure `lsp.clangd.binary.path` — Zed uses its own bundled clangd.

## Driver signing and test signing

The driver must be signed to load on Windows. `scripts/sign-driver.ps1` creates a self-signed code-signing certificate `CN=Ringo Test Driver` if it doesn't exist, attempts to import it into `LocalMachine\Root` and `LocalMachine\TrustedPublisher` (requires admin), and signs the `.sys` file:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\sign-driver.ps1 `
  -DriverPath build\bin\rngdrv.sys -EnableTestSigning
```

For a test environment:

1. Run the script as administrator (so the certificate is imported into machine stores; without admin it only signs, but the kernel won't trust it)
2. `-EnableTestSigning` runs `bcdedit /set testsigning on`
3. Reboot Windows
4. Load the driver (the client registers and starts the service)

Check status:

```powershell
bcdedit | Select-String testsigning
```

## Notes

- The client requires administrator privileges (it opens `\\.\ringo` and manages the service)
- Test signing requires a reboot after enabling it
