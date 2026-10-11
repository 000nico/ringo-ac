[CmdletBinding()]
param(
    [string]$DriverPath,
    [string]$CertSubject = 'CN=Ringo Test Driver',
    [switch]$SkipTrust,
    [switch]$EnableTestSigning
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $DriverPath) {
    $DriverPath = Join-Path $repoRoot 'build\bin\rngdrv.sys'
}
if (-not (Test-Path $DriverPath)) {
    throw "No existe el driver a firmar: $DriverPath"
}

function Get-SignTool {
    $kitsBin = 'C:\Program Files (x86)\Windows Kits\10\bin'
    $candidates = Get-ChildItem $kitsBin -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^\d+\.\d+' } |
        Sort-Object { [version]$_.Name } -Descending
    foreach ($dir in $candidates) {
        $exe = Join-Path $dir.FullName 'x64\signtool.exe'
        if (Test-Path $exe) { return $exe }
    }
    throw 'No se encontro signtool.exe. Instala el Windows SDK.'
}

function Add-CertToStore {
    param(
        [System.Security.Cryptography.X509Certificates.X509Certificate2]$Cert,
        [string]$Name,
        [string]$Location
    )
    $store = New-Object System.Security.Cryptography.X509Certificates.X509Store($Name, $Location)
    $store.Open([System.Security.Cryptography.X509Certificates.OpenFlags]::ReadWrite)
    $store.Add($Cert)
    $store.Close()
}

$signtool = Get-SignTool

# Certificado de prueba autofirmado (code signing EKU)
$cert = Get-ChildItem Cert:\CurrentUser\My |
    Where-Object { $_.Subject -eq $CertSubject } |
    Sort-Object NotAfter -Descending |
    Select-Object -First 1

if (-not $cert) {
    Write-Host "[sign] Creando certificado autofirmado de prueba '$CertSubject' ..."
    $cert = New-SelfSignedCertificate `
        -Type CodeSigningCert `
        -Subject $CertSubject `
        -CertStoreLocation 'Cert:\CurrentUser\My' `
        -KeyAlgorithm RSA `
        -KeyLength 2048 `
        -HashAlgorithm SHA256 `
        -NotAfter (Get-Date).AddYears(5)
}

Write-Host "[sign] Certificado: $($cert.Subject) [$($cert.Thumbprint)]"

# Para test signing el certificado debe ser de confianza en la maquina.
if (-not $SkipTrust) {
    foreach ($storeName in @('Root', 'TrustedPublisher')) {
        try {
            Add-CertToStore -Cert $cert -Name $storeName -Location 'LocalMachine'
            Write-Host "[sign] Certificado importado en LocalMachine\$storeName"
        } catch {
            Write-Warning "[sign] No se pudo importar en LocalMachine\$storeName (requiere admin)."
        }
    }
}

Write-Host "[sign] Firmando $DriverPath ..."
& $signtool sign /v /fd SHA256 /s My /sha1 $cert.Thumbprint $DriverPath
if ($LASTEXITCODE -ne 0) {
    throw "signtool fallo al firmar (codigo $LASTEXITCODE)."
}

cmd /c "`"$signtool`" verify /v /pa `"$DriverPath`" 2>&1" | Out-Host
if ($LASTEXITCODE -ne 0) {
    Write-Warning '[sign] La verificacion reporto que el certificado no es de confianza (normal sin admin: importalo en LocalMachine\Root / TrustedPublisher ejecutando como administrador).'
}

if ($EnableTestSigning) {
    Write-Host "[sign] Habilitando test signing (bcdedit /set testsigning on) ..."
    & bcdedit /set testsigning on
    if ($LASTEXITCODE -ne 0) {
        Write-Warning '[sign] No se pudo habilitar test signing (requiere admin).'
    } else {
        Write-Host '[sign] Test signing habilitado. Reinicia Windows para aplicarlo.'
    }
}

Write-Host '[sign] Firmado.'
exit 0
