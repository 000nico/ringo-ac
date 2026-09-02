<#
.SYNOPSIS
    Builds the kernel-mode (.sys) and user-mode (.exe) parts of a driver project
    and copies the binaries to a VMware Workstation/Player VM.
#>

[CmdletBinding()]
param(
    [string]$ConfigPath = (Join-Path $PSScriptRoot "deploy-config.json")
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

# ---------------------------------------------------------------------------
# 0. Load configuration from JSON
# ---------------------------------------------------------------------------
Write-Step "Loading configuration from $ConfigPath"

if (-not (Test-Path $ConfigPath)) {
    throw "Config file not found: $ConfigPath"
}

$config = Get-Content -Path $ConfigPath -Raw | ConvertFrom-Json

# Build settings
$KernelSolutionPath = $config.build.kernelSolutionPath
$UserSolutionPath   = $config.build.userSolutionPath
$Configuration      = $config.build.configuration
$Platform           = $config.build.platform
$MSBuildPath        = $config.build.msbuildPath

# VMware settings
$VmrunPath          = $config.vmware.vmrunPath
$VmType             = $config.vmware.vmType
$VmxPath            = $config.vmware.vmxPath
$GuestUser          = $config.vmware.guestUser
$GuestPasswordPlain = $config.vmware.guestPassword
$VmEncryptionPassword = $config.vmware.encryptionPassword

# Driver settings
$SysFileName      = $config.driver.sysFileName
$ExeFileName      = $config.driver.exeFileName
$RemoteDeployPath = $config.driver.remoteDeployPath

if (-not (Test-Path $VmrunPath)) {
    throw "vmrun.exe not found at '$VmrunPath'. Install VMware Workstation/Player or fix 'vmware.vmrunPath' in the config."
}

$VmEncryptionArgs = @()
if ($VmEncryptionPassword -and $VmEncryptionPassword -ne "CHANGE_ME_IF_VM_IS_ENCRYPTED") {
    $VmEncryptionArgs = @("-vp", $VmEncryptionPassword)
}

function Invoke-VmRun {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [switch]$IgnoreError
    )
    $fullArgs = @("-T", $VmType) + $VmEncryptionArgs + @("-gu", $GuestUser, "-gp", $GuestPasswordPlain) + $Arguments
    $output = & $VmrunPath @fullArgs 2>&1
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0 -and -not $IgnoreError) {
        throw "vmrun failed (exit $exitCode): $output"
    }
    return $output
}

# ---------------------------------------------------------------------------
# Helper: build one solution with MSBuild (compilacion estatica agregada)
# ---------------------------------------------------------------------------
function Build-Solution {
    param([string]$SolutionPath)

    Write-Step "Building solution: $SolutionPath ($Configuration|$Platform)"

    $runtimeLib = if ($Configuration -eq "Debug") { "MultiThreadedDebug" } else { "MultiThreaded" }

    $buildArgs = @(
        "`"$SolutionPath`"",
        "/p:Configuration=$Configuration",
        "/p:Platform=$Platform",
        "/p:RuntimeLibrary=$runtimeLib",
        "/m",
        "/nologo",
        "/v:minimal"
    )

    $proc = Start-Process -FilePath $MSBuildPath -ArgumentList $buildArgs -NoNewWindow -Wait -PassThru
    if ($proc.ExitCode -ne 0) {
        throw "MSBuild failed with exit code $($proc.ExitCode) for $SolutionPath."
    }
    Write-Host "Build OK: $SolutionPath" -ForegroundColor Green
}

# ---------------------------------------------------------------------------
# Helper: locate a built binary under a given solution's folder only
# ---------------------------------------------------------------------------
function Find-BuiltBinary {
    param(
        [string]$SolutionPath,
        [string]$FileName
    )

    $solutionDir = Split-Path -Parent $SolutionPath

    $found = Get-ChildItem -Path $solutionDir -Recurse -Filter $FileName -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match [regex]::Escape($Platform) -and $_.FullName -match [regex]::Escape($Configuration) } |
        Select-Object -First 1

    if (-not $found) {
        $found = Get-ChildItem -Path $solutionDir -Recurse -Filter $FileName -ErrorAction SilentlyContinue | Select-Object -First 1
    }

    if (-not $found) {
        throw "Could not find $FileName under $solutionDir."
    }

    return $found
}

# ---------------------------------------------------------------------------
# 1. Build both solutions
# ---------------------------------------------------------------------------
Build-Solution -SolutionPath $KernelSolutionPath
Build-Solution -SolutionPath $UserSolutionPath

# ---------------------------------------------------------------------------
# 2. Locate the generated binaries
# ---------------------------------------------------------------------------
Write-Step "Locating generated binaries"

$sysFile = Find-BuiltBinary -SolutionPath $KernelSolutionPath -FileName $SysFileName
$exeFile = Find-BuiltBinary -SolutionPath $UserSolutionPath -FileName $ExeFileName

Write-Host "SYS: $($sysFile.FullName)"
Write-Host "EXE: $($exeFile.FullName)"

# ---------------------------------------------------------------------------
# 3. Make sure the VM is running and VMware Tools is ready
# ---------------------------------------------------------------------------
Write-Step "Checking VM state"

$runningVms = & $VmrunPath -T $VmType @VmEncryptionArgs list
$vmAlreadyRunning = ($runningVms | Where-Object { $_ -match [regex]::Escape($VmxPath) }).Count -gt 0

if (-not $vmAlreadyRunning) {
    Write-Host "VM is not running. Starting it..." -ForegroundColor Yellow
    & $VmrunPath -T $VmType @VmEncryptionArgs start $VmxPath nogui
    Start-Sleep -Seconds 10
} else {
    Write-Host "VM is already running." -ForegroundColor Green
}

Write-Host "Waiting for VMware Tools to be ready inside the guest..."
$maxRetries = 30
$toolsReady = $false
for ($i = 0; $i -lt $maxRetries; $i++) {
    $state = & $VmrunPath -T $VmType @VmEncryptionArgs checkToolsState $VmxPath 2>&1
    if ($state -match "running") {
        $toolsReady = $true
        break
    }
    Start-Sleep -Seconds 5
}
if (-not $toolsReady) { throw "VMware Tools did not respond in time inside the guest." }
Write-Host "VMware Tools ready." -ForegroundColor Green

# Sanity check that the guest credentials actually work
Invoke-VmRun -Arguments @("runProgramInGuest", $VmxPath, "-noWait", "C:\Windows\System32\cmd.exe", "/c", "exit")
Write-Host "Guest credentials validated." -ForegroundColor Green

# ---------------------------------------------------------------------------
# 4. Copy binaries to the VM
# ---------------------------------------------------------------------------
Write-Step "Copying binaries to the VM ($RemoteDeployPath)"

# Create destination folder in the guest (ignore error if it already exists)
Invoke-VmRun -IgnoreError -Arguments @(
    "runProgramInGuest", $VmxPath, "-activeWindow",
    "C:\Windows\System32\cmd.exe", "/c", "mkdir `"$RemoteDeployPath`""
)

$remoteSysPath = Join-Path $RemoteDeployPath $SysFileName
$remoteExePath = Join-Path $RemoteDeployPath $ExeFileName

Invoke-VmRun -Arguments @("copyFileFromHostToGuest", $VmxPath, $sysFile.FullName, $remoteSysPath)
Invoke-VmRun -Arguments @("copyFileFromHostToGuest", $VmxPath, $exeFile.FullName, $remoteExePath)

Write-Host "Copy completed." -ForegroundColor Green