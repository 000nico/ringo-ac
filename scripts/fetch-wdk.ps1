[CmdletBinding()]
param(
    [string]$Version = '10.0.26100.6584'
)

$ErrorActionPreference = 'Stop'

$repoRoot      = Split-Path -Parent $PSScriptRoot
$toolchainsDir = Join-Path $repoRoot 'toolchains'
$wdkRoot       = Join-Path $toolchainsDir 'wdk'
$contentRoot   = Join-Path $wdkRoot 'c'

if (Test-Path (Join-Path $contentRoot 'Include')) {
    Write-Host "[fetch-wdk] WDK ya presente en $contentRoot"
    return $contentRoot
}

New-Item -ItemType Directory -Force -Path $toolchainsDir | Out-Null

$url   = "https://api.nuget.org/v3-flatcontainer/microsoft.windows.wdk.x64/$Version/microsoft.windows.wdk.x64.$Version.nupkg"
$nupkg = Join-Path $toolchainsDir 'wdk.nupkg'
$zip   = Join-Path $toolchainsDir 'wdk.zip'

Write-Host "[fetch-wdk] Descargando WDK $Version desde nuget.org ..."
Invoke-WebRequest -Uri $url -OutFile $nupkg -UseBasicParsing

Copy-Item $nupkg $zip -Force
if (Test-Path $wdkRoot) { Remove-Item -Recurse -Force $wdkRoot }
New-Item -ItemType Directory -Force -Path $wdkRoot | Out-Null
Expand-Archive -LiteralPath $zip -DestinationPath $wdkRoot -Force
Remove-Item $zip -Force

if (-not (Test-Path (Join-Path $contentRoot 'Include'))) {
    throw "El paquete WDK no contiene el directorio esperado 'c/Include'."
}

Write-Host "[fetch-wdk] WDK extraido en $contentRoot"
return $contentRoot
