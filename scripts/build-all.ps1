[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [string]$Generator = 'Visual Studio 18 2026',
    [switch]$SkipDriver,
    [switch]$SkipClient,
    [switch]$SkipServer,
    [switch]$SkipSign,
    [switch]$EnableTestSigning
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot 'build'

$buildDriver = -not $SkipDriver
$buildClient = -not $SkipClient
$buildServer = -not $SkipServer

function To-CMakeBool([bool]$Value) { if ($Value) { 'ON' } else { 'OFF' } }

# 1. Asegurar el WDK (usado por find_package(WDK) -> FindWDK.cmake).
$contentRoot = Join-Path $repoRoot 'toolchains\wdk\c'
if (-not $env:WDKContentRoot -and -not (Test-Path (Join-Path $contentRoot 'Include'))) {
    Write-Host '[build-all] No se encontro WDK; descargando ...'
    & (Join-Path $PSScriptRoot 'fetch-wdk.ps1') | Out-Null
}
if (Test-Path (Join-Path $contentRoot 'Include')) {
    $env:WDKContentRoot = $contentRoot
}
if ($buildDriver -and -not $env:WDKContentRoot) {
    throw 'No hay WDK disponible. Ejecuta scripts/fetch-wdk.ps1.'
}
if ($env:WDKContentRoot) {
    Write-Host "[build-all] WDKContentRoot = $env:WDKContentRoot"
}

# 2. Configurar.
Write-Host "[build-all] Configurando CMake ($Generator, x64) ..."
cmake -S $repoRoot -B $buildDir -G $Generator -A x64 `
    "-DRINGO_BUILD_DRIVER=$(To-CMakeBool $buildDriver)" `
    "-DRINGO_BUILD_CLIENT=$(To-CMakeBool $buildClient)" `
    "-DRINGO_BUILD_SERVER=$(To-CMakeBool $buildServer)"
if ($LASTEXITCODE -ne 0) { throw 'CMake configure fallo.' }

# 3. Compilar.
$targets = @()
if ($buildDriver) { $targets += 'driver' }
if ($buildClient) { $targets += 'client' }
if ($buildServer) { $targets += 'ringo-server' }

foreach ($target in $targets) {
    Write-Host "[build-all] Compilando '$target' ($Configuration) ..."
    cmake --build $buildDir --config $Configuration --target $target
    if ($LASTEXITCODE -ne 0) { throw "Fallo la compilacion de '$target'." }
}

# 4. Firmar (test signing).
if ($buildDriver -and -not $SkipSign) {
    $driver = Join-Path $buildDir 'bin\rngdrv.sys'
    $signArgs = @{ DriverPath = $driver }
    if ($EnableTestSigning) { $signArgs['EnableTestSigning'] = $true }
    & (Join-Path $PSScriptRoot 'sign-driver.ps1') @signArgs
    if ($LASTEXITCODE -ne 0) { throw 'La firma del driver fallo.' }
}

Write-Host "[build-all] Listo. Binarios en $(Join-Path $buildDir 'bin')"
