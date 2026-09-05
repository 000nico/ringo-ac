<#
.SYNOPSIS
    Builds the kernel-mode (.sys) and user-mode (.exe) projects and copies
    both binaries to the Desktop of a VMware Workstation/Player VM.
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
$SysFileName = $config.driver.sysFileName
$ExeFileName = $config.driver.exeFileName

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

    # NOTE: Windows PowerShell (5.1) silently drops empty-string ("") arguments
    # when calling a native exe via '&' with array splatting. Since an empty
    # guest password is a valid/common case (no-password guest account), that
    # bug shifts every argument after it and vmrun ends up misreading the
    # .vmx path as a command name. Building the command line manually and
    # running it through cmd.exe forces every argument -- including the
    # empty one -- to be quoted and passed through correctly.
    $quotedArgs = ($fullArgs | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $commandLine = "`"$VmrunPath`" $quotedArgs"

    $output = cmd /c $commandLine 2>&1
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0 -and -not $IgnoreError) {
        throw "vmrun failed (exit $exitCode): $output"
    }
    return $output
}

# ---------------------------------------------------------------------------
# Helper: generate a .props file that FORCES static CRT linking (/MT or /MTd)
# ---------------------------------------------------------------------------
# Passing /p:RuntimeLibrary=... on the MSBuild command line often does NOT
# override a value the .vcxproj already hardcodes in its own ItemDefinitionGroup.
# The reliable fix is to import a .props file AFTER the project's own C++
# targets via ForceImportAfterCppTargets -- item metadata defined later wins,
# so this guarantees the static runtime (no vcruntime140.dll / msvcp140.dll
# dependency at runtime) regardless of what the project file says.
function New-StaticCrtPropsFile {
    $propsPath = Join-Path $env:TEMP "force_static_crt.props"
    $propsContent = @'
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemDefinitionGroup>
    <ClCompile>
      <RuntimeLibrary Condition="'$(Configuration)'=='Debug'">MultiThreadedDebug</RuntimeLibrary>
      <RuntimeLibrary Condition="'$(Configuration)'!='Debug'">MultiThreaded</RuntimeLibrary>
    </ClCompile>
  </ItemDefinitionGroup>
</Project>
'@
    Set-Content -Path $propsPath -Value $propsContent -Encoding UTF8
    return $propsPath
}

# ---------------------------------------------------------------------------
# Helper: build one solution with MSBuild
# ---------------------------------------------------------------------------
function Build-Solution {
    param(
        [string]$SolutionPath,
        [switch]$ForceStaticCrt
    )

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

    if ($ForceStaticCrt) {
        $propsPath = New-StaticCrtPropsFile
        $buildArgs += "/p:ForceImportAfterCppTargets=`"$propsPath`""
        Write-Host "Forcing static CRT (/MT or /MTd) via $propsPath" -ForegroundColor Yellow
    }

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
# 1. Build both solutions (KM + UM)
# ---------------------------------------------------------------------------
Build-Solution -SolutionPath $KernelSolutionPath
Build-Solution -SolutionPath $UserSolutionPath -ForceStaticCrt

# ---------------------------------------------------------------------------
# 2. Locate the generated binaries
# ---------------------------------------------------------------------------
Write-Step "Locating generated binaries"

$sysFile = Find-BuiltBinary -SolutionPath $KernelSolutionPath -FileName $SysFileName
$exeFile = Find-BuiltBinary -SolutionPath $UserSolutionPath -FileName $ExeFileName

Write-Host "SYS: $($sysFile.FullName)"
Write-Host "EXE: $($exeFile.FullName)"

# ---------------------------------------------------------------------------
# 3. Copy both binaries to the Desktop of the VM
# ---------------------------------------------------------------------------
$RemoteDesktopPath = "C:\Users\$GuestUser\Desktop"

Write-Step "Copying binaries to VM Desktop ($RemoteDesktopPath)"

$remoteSysPath = Join-Path $RemoteDesktopPath $SysFileName
$remoteExePath = Join-Path $RemoteDesktopPath $ExeFileName

Invoke-VmRun -Arguments @("copyFileFromHostToGuest", $VmxPath, $sysFile.FullName, $remoteSysPath)
Invoke-VmRun -Arguments @("copyFileFromHostToGuest", $VmxPath, $exeFile.FullName, $remoteExePath)

Write-Host "Copy completed." -ForegroundColor Green
