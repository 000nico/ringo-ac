# Environment Integrity and Hardware Telemetry

This document describes the environment verification probes and the hardware identification system in the Ringo kernel driver. These components characterize the security state of the machine the driver is running on, and provide the data that a backend service would use to assess trustworthiness.

---

## Environment Probes

The `km/core/system/` directory contains seven independent probes, each in its own subdirectory. They share no state with each other and are invoked by the IOCTL dispatcher on demand. All probes return typed structs that fit in small buffers — the largest is three bytes for the Secure Boot result.

The architecture is intentional: probes are isolated so that a failure in one does not cascade, and they are on-demand rather than polled so that the driver does not consume kernel resources between IOCTL calls.

### Hypervisor Detection

Source: `km/core/system/hypervisor/`

The hypervisor check does two things. It calls `ZwQuerySystemInformation(SystemHypervisorInformation)` to read the kernel's view of hypervisor presence. It then executes a raw `__cpuid(cpuInfo, 1)` instruction and reads bit 31 of ECX, which the hardware hypervisor sets directly in the CPU response.

If these two answers disagree — the kernel says no hypervisor, but CPUID says yes, or vice versa — the `manipulatedInformation` field in the result struct is set. This indicates that something is selectively hooking or forging system information calls, which is a strong signal of a hypervisor-based cheat (e.g., an external memory reader running from a Type-1 hypervisor that intercepts virtualization-related kernel queries while leaving CPUID honest).

### Secure Boot

Source: `km/core/system/secure_boot/`

Secure Boot is verified through three independent sources and the results are cross-compared:

1. `ZwQuerySystemInformation(SystemSecureBootInformation)` — the kernel's official answer.
2. `ExGetFirmwareEnvironmentVariable` querying the UEFI NVRAM variable `SecureBoot` under the EFI global variable GUID `{8be4df61-93ca-11d2-aa0d-00e098032b8c}`.
3. A registry read from `HKLM\SYSTEM\CurrentControlSet\Control\SecureBoot\State\UEFISecureBootEnabled`.

If any two sources disagree, or if the result claims Secure Boot is enabled but the platform is not capable of it, `consistentData` is set to false. Inconsistency is itself a signal — it suggests that one data source is being spoofed or that the environment is in an unexpected state.

### HVCI, VBS, and Test Signing

Sources: `hvci/`, `vbs/`, `test_signing/`

These three probes all query `SystemCodeIntegrityInformation` and inspect specific bit flags in the returned `CodeIntegrityOptions` field. HVCI checks for `CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED` (0x400). Test-signing mode checks for `CODEINTEGRITY_OPTION_TESTSIGN` (0x02). VBS additionally queries `SystemIsolatedUserModeInformation` to check whether the Secure Kernel is running alongside the normal kernel.

Test-signing mode is particularly significant for anti-cheat purposes: a system running with test-signing enabled accepts unsigned or self-signed drivers, which is the development configuration used by most kernel cheat developers. Detecting it does not prove cheating, but it changes the trust level of any kernel-mode assertions the driver makes about the system.

### Kernel Debugger Detection

Source: `km/core/system/kernel_debugging/`

`ZwQuerySystemInformation(SystemKernelDebuggerInformation)` returns two flags: `KernelDebuggerEnabled` and `KernelDebuggerNotPresent`. The probe considers a debugger present only when both conditions hold: enabled is true and not-present is false. Using both avoids false positives from configurations where kernel debugging infrastructure is loaded but not actively connected.

### TPM Probing

Source: `km/core/system/tpm/`

The TPM probe opens `\Device\TPM` via `ZwCreateFile` and attempts a test command. It first tries a TPM 2.0 command (`TPM2_GetRandom`); if that fails it falls back to TPM 1.2 (`TPM_ORD_GetRandom`). The returned `TPM` struct reports the detected version as an enum (`None`, `1.2`, or `2.0`).

This approach works on many systems but is experimental: newer Windows builds may require TPM commands to go through the TPM Base Services (TBS) IOCTL layer rather than the raw device. The probe may return `None` on up-to-date systems even when a TPM is physically present.

---

## Hardware Telemetry

Source: `km/core/identity/hwid/hwid.cpp`

The HWID subsystem builds a deterministic 24-byte machine fingerprint from three hardware primitives:

```cpp
typedef struct _HWID {
    UCHAR SmbiosUuid[16];  // SMBIOS Type 1 System UUID
    ULONG CpuHash;         // Combined CPUID hash (leaves 0, 1, and 4)
    ULONG DiskSerial;      // Polynomial hash of physical drive serial
} HWID;
```

These sources are chosen for stability and independence. They read from firmware, CPU microcode, and storage hardware respectively, making them resistant to OS-level spoofing attempts that manipulate registry values or WMI results.

**SMBIOS UUID** — `ZwQuerySystemInformation(SystemFirmwareTableInformation)` with the `'RSMB'` provider retrieves the raw SMBIOS blob. The code traverses formatted SMBIOS records until it finds a Type 1 (System Information) entry and extracts the 16-byte UUID at offset 0x08. Boundary checks and terminator markers (type 127, length < 4) prevent reads past the end of the table.

**CPU Hash** — Three CPUID leaves are combined. Leaf 0 gives the vendor string as register values (mixed with XOR). Leaf 1 gives the processor signature and feature flags. Leaf 4 with subleaf 0 gives deterministic cache parameters. The combination produces a hash that identifies the CPU family and model without depending on any OS-provided data.

**Disk Serial** — A kernel handle to `\Device\Harddisk0\DR0` is opened via `ZwOpenFile`, and `IOCTL_STORAGE_QUERY_PROPERTY` with `StorageDeviceProperty` retrieves a `STORAGE_DEVICE_DESCRIPTOR`. The serial number string at `SerialNumberOffset` is hashed with a polynomial accumulator (`hash = hash * 31 + char`). Reading the serial at the kernel level bypasses user-mode WMI and registry paths that cheats commonly spoof.

The fingerprint is returned to the user-mode service via `IOCTL_GET_HWID` and is intended for session binding and ban enforcement by a backend service.
