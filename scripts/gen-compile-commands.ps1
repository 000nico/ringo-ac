# Genera la base de datos de compilacion (compile_commands.json) que clangd usa
# en Zed para el driver, el cliente y el server.
#
# Por que un build aparte (build-index/):
#   - El generador "Visual Studio" no puede exportar compile_commands.json.
#   - Ninja a secas toma el GCC de MSYS2 que esta en el PATH, y clangd terminaria
#     mezclando headers de MinGW con flags de MSVC. Por eso pasamos por vcvars64.bat.
#   - Es solo para el LSP: no genera binarios ni reemplaza a build/.
#
# Uso:  powershell -ExecutionPolicy Bypass -File scripts\gen-compile-commands.ps1

[CmdletBinding()]
param(
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot 'build-index'

# 1. WDK local (lo usa find_package(WDK) -> FindWDK.cmake).
if (-not $env:WDKContentRoot) {
    $contentRoot = Join-Path $repoRoot 'toolchains\wdk\c'
    if (Test-Path (Join-Path $contentRoot 'Include')) {
        $env:WDKContentRoot = $contentRoot
    } else {
        throw 'No hay WDK. Ejecuta scripts/fetch-wdk.ps1.'
    }
}
Write-Host "[index] WDKContentRoot = $env:WDKContentRoot"

# 2. Entorno MSVC (vcvars64.bat).
$vcvars = Get-ChildItem @(
    "$env:ProgramFiles\Microsoft Visual Studio\*\*\VC\Auxiliary\Build\vcvars64.bat"
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\*\*\VC\Auxiliary\Build\vcvars64.bat"
) -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
if (-not $vcvars) {
    throw 'No encontre vcvars64.bat. Instala Visual Studio con "Desktop development with C++".'
}
Write-Host "[index] vcvars = $vcvars"

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "[index] Borrando $buildDir ..."
    Remove-Item $buildDir -Recurse -Force
}

# 3. Configure con el preset "index" (Ninja + MSVC + CMAKE_EXPORT_COMPILE_COMMANDS).
#    vcvars64.bat es un .bat: hay que llamarlo desde cmd.
$configure = 'set "WDKContentRoot=' + $env:WDKContentRoot + '" && ' +
             'call "' + $vcvars + '" >nul && ' +
             'cd /d "' + $repoRoot + '" && ' +
             'cmake --preset index'
Write-Host '[index] cmake --preset index ...'
cmd /c $configure
if ($LASTEXITCODE -ne 0) { throw 'Fallo el configure de build-index.' }

# 4. Verificar.
$db = Join-Path $buildDir 'compile_commands.json'
if (-not (Test-Path $db)) { throw "No se genero $db" }
$entradas = ConvertFrom-Json (Get-Content $db -Raw)
$count = $entradas.Count
Write-Host "[index] OK: $count entradas en $db"
Write-Host '[index] clangd lo lee via .clangd (CompileFlags.CompilationDatabase: build-index).'
