[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$DriverPath,
    [Parameter(Mandatory = $true)]
    [string]$CertificatePath,
    [Parameter(Mandatory = $true)]
    [string]$TimestampUrl,
    [string]$SigntoolPath = "signtool.exe"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $DriverPath)) {
    throw "No existe el driver: $DriverPath"
}
if (-not (Test-Path -LiteralPath $CertificatePath)) {
    throw "No existe el certificado: $CertificatePath"
}

& $SigntoolPath sign /fd SHA256 /f $CertificatePath /tr $TimestampUrl /td SHA256 $DriverPath
if ($LASTEXITCODE -ne 0) {
    throw "signtool falló con código $LASTEXITCODE"
}

& $SigntoolPath verify /kp /v $DriverPath
if ($LASTEXITCODE -ne 0) {
    throw "La verificación de firma falló con código $LASTEXITCODE"
}
