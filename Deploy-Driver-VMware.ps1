<#
.SYNOPSIS
    Builds the kernel-mode (.sys) and user-mode (.exe) parts of a driver project,
    copies the binaries to a VMware Workstation/Player VM, enables test signing,
    and reinstalls the driver service (sc delete / sc stop / sc create / sc start).

.DESCRIPTION
    All settings (build paths, VM info, guest credentials, driver/service names)
    are read from a JSON config file sitting next to this script (deploy-config.json
    by default), so you just run:

        .\Deploy-Driver-VMware.ps1

    without passing any parameters.

    Uses vmrun.exe (VIX API, shipped with VMware Workstation/Player) to:
      - copy files from host to guest (copyFileFromHostToGuest)
      - run commands inside the guest (runProgramInGuest)
      - reset the VM and wait for VMware Tools to come back up

    No network/WinRM setup is required inside the VM, just VMware Tools installed
    and running (needed for runProgramInGuest / copyFileFromHostToGuest).

    Prerequisites:
      - Host: Visual Studio + WDK installed (MSBuild available).
      - Host: VMware Workstation/Player installed, with vmrun.exe available
        (typically "C:\Program Files (x86)\VMware\VMware VIX\vmrun.exe").
      - VM: VMware Tools installed and running, plus a valid guest user/password.

.PARAMETER ConfigPath
    Path to the JSON config file. Defaults to "deploy-config.json" next to this script.

.EXAMPLE
    .\Deploy-Driver-VMware.ps1

.EXAMPLE
    .\Deploy-Driver-VMware.ps1 -ConfigPath "C:\configs\my-deploy.json"

.NOTES
    SECURITY: the JSON config stores the guest password in plain text. This is meant
    for a local/lab test VM only. Do not reuse real credentials, do not commit this
    file to source control, and restrict its file permissions (icacls) if needed.
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
$SolutionPath  = $config.build.solutionPath
$Configuration = $config.build.configuration
$Platform      = $config.build.platform
$MSBuildPath   = $config.build.msbuildPath

# VMware settings
$VmrunPath          = $config.vmware.vmrunPath
$VmType             = $config.vmware.vmType
$VmxPath            = $config.vmware.vmxPath
$GuestUser          = $config.vmware.guestUser
$GuestPasswordPlain = $config.vmware.guestPassword

# Driver settings
$DriverName       = $config.driver.driverName
$SysFileName      = $config.driver.sysFileName
$ExeFileName      = $config.driver.exeFileName
$RemoteDeployPath = $config.driver.remoteDeployPath

# Options
$RebootIfNeeded = [bool]$config.options.rebootIfNeeded

if (-not (Test-Path $VmrunPath)) {
    throw "vmrun.exe not found at '$VmrunPath'. Install VMware Workstation/Player or fix 'vmware.vmrunPath' in the config."
}

# Helper to call vmrun with guest credentials already baked in
function Invoke-VmRun {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [switch]$IgnoreError
    )
    $fullArgs = @("-T", $VmType, "-gu", $GuestUser, "-gp", $GuestPasswordPlain) + $Arguments
    $output = & $VmrunPath @fullArgs 2>&1
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0 -and -not $IgnoreError) {
        throw "vmrun failed (exit $exitCode): $output"
    }
    return $output
}

# ---------------------------------------------------------------------------
# 1. Build the solution (kernel-mode .sys + user-mode .exe)
# ---------------------------------------------------------------------------
Write-Step "Building solution: $SolutionPath ($Configuration|$Platform)"

$buildArgs = @(
    "`"$SolutionPath`"",
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform",
    "/m",
    "/nologo",
    "/v:minimal"
)

$proc = Start-Process -FilePath $MSBuildPath -ArgumentList $buildArgs -NoNewWindow -Wait -PassThru
if ($proc.ExitCode -ne 0) {
    throw "MSBuild failed with exit code $($proc.ExitCode)."
}
Write-Host "Build OK." -ForegroundColor Green

# ---------------------------------------------------------------------------
# 2. Locate the generated binaries
# ---------------------------------------------------------------------------
Write-Step "Locating generated binaries"

$solutionDir = Split-Path -Parent $SolutionPath

$sysFile = Get-ChildItem -Path $solutionDir -Recurse -Filter $SysFileName -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match [regex]::Escape($Platform) -and $_.FullName -match [regex]::Escape($Configuration) } |
    Select-Object -First 1
if (-not $sysFile) {
    $sysFile = Get-ChildItem -Path $solutionDir -Recurse -Filter $SysFileName -ErrorAction SilentlyContinue | Select-Object -First 1
}

$exeFile = Get-ChildItem -Path $solutionDir -Recurse -Filter $ExeFileName -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match [regex]::Escape($Platform) -and $_.FullName -match [regex]::Escape($Configuration) } |
    Select-Object -First 1
if (-not $exeFile) {
    $exeFile = Get-ChildItem -Path $solutionDir -Recurse -Filter $ExeFileName -ErrorAction SilentlyContinue | Select-Object -First 1
}

if (-not $sysFile) { throw "Could not find $SysFileName under $solutionDir." }
if (-not $exeFile) { throw "Could not find $ExeFileName under $solutionDir." }

Write-Host "SYS: $($sysFile.FullName)"
Write-Host "EXE: $($exeFile.FullName)"

# ---------------------------------------------------------------------------
# 3. Make sure the VM is running and VMware Tools is ready
# ---------------------------------------------------------------------------
Write-Step "Checking VM state"

$runningVms = & $VmrunPath -T $VmType list
if ($runningVms -notmatch [regex]::Escape($VmxPath)) {
    Write-Host "VM is not running. Starting it..." -ForegroundColor Yellow
    & $VmrunPath -T $VmType start $VmxPath nogui
    Start-Sleep -Seconds 10
}

Write-Host "Waiting for VMware Tools to be ready inside the guest..."
$maxRetries = 30
$toolsReady = $false
for ($i = 0; $i -lt $maxRetries; $i++) {
    $state = & $VmrunPath -T $VmType checkToolsState $VmxPath 2>&1
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

# ---------------------------------------------------------------------------
# 5. Enable test signing on the VM (if not already enabled)
# ---------------------------------------------------------------------------
Write-Step "Checking/enabling test signing on the VM"

# Dump 'bcdedit /enum' output to a temp file inside the guest, then pull it back
$remoteCheckFile = Join-Path $RemoteDeployPath "bcdcheck.txt"
Invoke-VmRun -Arguments @(
    "runProgramInGuest", $VmxPath, "-activeWindow",
    "C:\Windows\System32\cmd.exe", "/c", "bcdedit /enum > `"$remoteCheckFile`""
)

$localCheckFile = Join-Path $env:TEMP "bcdcheck.txt"
Invoke-VmRun -Arguments @("copyFileFromGuestToHost", $VmxPath, $remoteCheckFile, $localCheckFile)
$bcdOutput = Get-Content $localCheckFile -Raw
Remove-Item $localCheckFile -ErrorAction SilentlyContinue

$testSigningOn = $bcdOutput -match "testsigning\s+Yes"

if ($testSigningOn) {
    Write-Host "Test signing is already enabled." -ForegroundColor Green
} else {
    Write-Host "Enabling test signing..." -ForegroundColor Yellow
    Invoke-VmRun -Arguments @(
        "runProgramInGuest", $VmxPath, "-activeWindow",
        "C:\Windows\System32\cmd.exe", "/c", "bcdedit /set testsigning ON"
    )
    Write-Host "Test signing enabled. A VM reboot is required for it to take effect." -ForegroundColor Yellow

    if ($RebootIfNeeded) {
        Write-Step "Rebooting VM"
        & $VmrunPath -T $VmType reset $VmxPath

        Write-Host "Waiting for VMware Tools to come back up after reboot..."
        $toolsReady = $false
        for ($i = 0; $i -lt $maxRetries; $i++) {
            Start-Sleep -Seconds 5
            $state = & $VmrunPath -T $VmType checkToolsState $VmxPath 2>&1
            if ($state -match "running") {
                $toolsReady = $true
                break
            }
        }
        if (-not $toolsReady) { throw "VMware Tools did not respond in time after reboot." }
        Write-Host "VM rebooted and ready." -ForegroundColor Green
    } else {
        Write-Host "Set 'options.rebootIfNeeded' to true in the config, or reboot the VM manually before loading the driver." -ForegroundColor Yellow
    }
}

# ---------------------------------------------------------------------------
# 6. Stop/remove the old driver and install/start the new one
# ---------------------------------------------------------------------------
Write-Step "Reinstalling driver service '$DriverName' on the VM"

# Stop it (ignore error if it doesn't exist or is already stopped)
Invoke-VmRun -IgnoreError -Arguments @(
    "runProgramInGuest", $VmxPath, "-activeWindow",
    "C:\Windows\System32\sc.exe", "stop", $DriverName
)
Start-Sleep -Seconds 2

# Delete the old service (ignore error if it doesn't exist)
Invoke-VmRun -IgnoreError -Arguments @(
    "runProgramInGuest", $VmxPath, "-activeWindow",
    "C:\Windows\System32\sc.exe", "delete", $DriverName
)
Start-Sleep -Seconds 1

# Create the service pointing at the new .sys
Invoke-VmRun -Arguments @(
    "runProgramInGuest", $VmxPath, "-activeWindow",
    "C:\Windows\System32\sc.exe", "create", $DriverName,
    "type=", "kernel", "binPath=", $remoteSysPath, "start=", "demand"
)

# Start the new driver
Invoke-VmRun -Arguments @(
    "runProgramInGuest", $VmxPath, "-activeWindow",
    "C:\Windows\System32\sc.exe", "start", $DriverName
)

# ---------------------------------------------------------------------------
# 7. Query final status
# ---------------------------------------------------------------------------
Write-Step "Querying final service status"

$remoteQueryFile = Join-Path $RemoteDeployPath "scquery.txt"
Invoke-VmRun -Arguments @(
    "runProgramInGuest", $VmxPath, "-activeWindow",
    "C:\Windows\System32\cmd.exe", "/c", "sc query $DriverName > `"$remoteQueryFile`""
)
$localQueryFile = Join-Path $env:TEMP "scquery.txt"
Invoke-VmRun -Arguments @("copyFileFromGuestToHost", $VmxPath, $remoteQueryFile, $localQueryFile)
Get-Content $localQueryFile
Remove-Item $localQueryFile -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "Deploy complete." -ForegroundColor Green
