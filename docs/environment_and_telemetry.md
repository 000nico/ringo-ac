# Environment Integrity and Hardware Telemetry

This document describes the environment verification subsystems and the Hardware Identification (HWID) telemetry engine implemented in Ringo Anti-Cheat.

---

## 1. Environment Integrity Subsystem

The environment engine verifies that the underlying operating system is running in an authentic, untampered security configuration.

```
                    +--------------------------------------------+
                    |        Environment Integrity Checks        |
                    +---------------------+----------------------+
                                          |
        +------------------+--------------+-------------+------------------+
        |                  |                            |                  |
   [Hypervisor]      [Secure Boot]                 [Code Integrity]   [Kernel Debug]
   - CPUID vs ZW     - 3-Way Cross Check           - HVCI Status      - KdEnabled
   - Bit 31 Check    - UEFI Env Var & Registry     - TestSigning      - KdNotPresent
```

### 1.1 Hypervisor Presence and Tampering Detection
Source: `km/core/enviroment/hypervisor/hypervisor.cpp`

- **Mechanism**:
  1. Calls `ZwQuerySystemInformation(SystemHypervisorInformation, ...)` to check `shi.HypervisorPresent`.
  2. Executes raw instruction `__cpuid(cpuInfo, 1)` and reads `ECX` bit 31 (`rawBit = (cpuInfo[2] >> 31) & 1`).
  3. If `HypervisorPresent != rawBit`, flags `manipulatedInformation = TRUE` (indicating spoofing or selective hooking of system information).

### 1.2 Multi-Source Secure Boot Verification
Source: `km/core/enviroment/secure_boot/secure_boot.cpp`

- **Mechanism**: Performs a 3-way validation cross-checking different kernel subsystems:
  1. `ZwQuerySystemInformation(SystemSecureBootInformation, ...)` for `SecureBootEnabled` and `SecureBootCapable`.
  2. `ExGetFirmwareEnvironmentVariable` querying the UEFI NVRAM variable `L"SecureBoot"` under `EFI_GLOBAL_VARIABLE_GUID` (`8be4df61-93ca-11d2-aa0d-00e098032b8c`).
  3. Registry query via `ZwOpenKey` / `ZwQueryValueKey` for `\Registry\Machine\SYSTEM\CurrentControlSet\Control\SecureBoot\State\UEFISecureBootEnabled`.
  4. If results mismatch or if `isEnabled && !isSupported`, flags `consistentData = FALSE`.

### 1.3 Code Integrity, HVCI, and VBS
Sources: `hvci.cpp`, `test_signing.cpp`, `vbs.cpp`

- **HVCI**: Queries `SystemCodeIntegrityInformation` (`103`) for `CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED` (`0x400`).
- **Test Signing**: Checks `sci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_TESTSIGN` (`0x02`).
- **VBS / Isolated User Mode**: Queries `SystemIsolatedUserModeInformation` for `HvciEnabled` and `SecureKernelRunning`.

### 1.4 Kernel Debugger Detection
Source: `km/core/enviroment/kernel_debugging/kernel_debugging.cpp`

- Queries `SystemKernelDebuggerInformation` (`0x23`).
- Validates: `isKernelDebuggerPresent = skdi.KernelDebuggerEnabled && !skdi.KernelDebuggerNotPresent`.

### 1.5 TPM Hardware Probing
Source: `km/core/enviroment/tpm/tpm.cpp`

- Opens `\Device\TPM` using `ZwCreateFile`.
- Probes for TPM 2.0 command `TPM2_GetRandom` (`0x8001...0x017B`).
- Falls back to TPM 1.2 command `TPM_ORD_GetRandom` (`0x00C1...0x0046`).

---

## 2. Hardware Telemetry and HWID Generation

Source: `km/core/telemetry/hwid/hwid.cpp`

Ringo generates a deterministic hardware profile based on immutable machine primitives:

```cpp
typedef struct _HWID {
    UCHAR SmbiosUuid[16];    // Raw SMBIOS Type 1 System UUID
    ULONG CpuHash;           // Combined CPUID hash (Leaf 0, 1, and 4)
    ULONG DiskSerial;        // Hash of physical drive serial number
} HWID;
```

### 2.1 SMBIOS Type 1 UUID Parser
- Queries `ZwQuerySystemInformation(SystemFirmwareTableInformation, ...)` with provider signature `'RSMB'` (`SystemFirmwareTable_Get`).
- Locates the Raw SMBIOS Data table.
- Traverses formatted records until reaching Header Type 1 (`System Information`), extracting the 16-byte UUID at offset `0x08`.
- Validates boundaries and termination markers (`type == 127` and `length < 4`).

### 2.2 CPU Identification Hash
- Executes `__cpuid(cpuInfo, 0)` to mix Vendor String registers (`EAX ^ EBX ^ ECX ^ EDX`).
- Executes `__cpuid(cpuInfo, 1)` for Processor Signature and Feature flags (`EAX ^ EDX`).
- Executes `__cpuidex(cpuInfo, 4, 0)` for Deterministic Cache topology parameters.

### 2.3 Physical Disk Serial Number
- Opens kernel handle to `\Device\Harddisk0\DR0` via `ZwOpenFile`.
- Sends `IOCTL_STORAGE_QUERY_PROPERTY` with `StorageDeviceProperty` and `PropertyStandardQuery`.
- Extracts `SerialNumberOffset` from the returned `STORAGE_DEVICE_DESCRIPTOR`.
- Computes a polynomial hash (`hash * 31 + char`) across the null-terminated serial string.
