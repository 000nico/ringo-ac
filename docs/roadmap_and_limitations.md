# Project Status, Limitations and Roadmap

This document outlines what Ringo Anti-Cheat currently implements, what is currently not implemented, known technical limitations, and the future development roadmap.

---

## 1. What Ringo-AC Does (Current Capabilities)

- **Handle Protection (ObRegisterCallbacks)**:
  - Restricts unauthorized process handles against the target game, stripping `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, and `PROCESS_VM_OPERATION`.
  - Restricts unauthorized thread handles, stripping context modification, termination, and suspension rights (`THREAD_SET_CONTEXT`, `THREAD_SUSPEND_RESUME`, `THREAD_TERMINATE`, etc.).
  - Distinguishes kernel-mode handles from user-mode handles.
- **Process and Thread Lifecycle Monitoring**:
  - Automatically identifies when the protected game starts or terminates via `PsSetCreateProcessNotifyRoutineEx`.
  - Intercepts thread creation via `PsSetCreateThreadNotifyRoutineEx` and captures thread start addresses (`ThreadQuerySetWin32StartAddress`).
  - Detects cross-process (remote) thread injections (`PsGetCurrentProcessId() != ProcessId`).
- **Module Load Tracking**:
  - Logs all DLLs loaded into the game address space via `PsSetLoadImageNotifyRoutineEx` with signature level checks.
- **Deep Environment Integrity Verification**:
  - Detects hypervisors and information spoofing by comparing CPUID leaf 1 against `SystemHypervisorInformation`.
  - Performs 3-way validation of Secure Boot (System Info, UEFI NVRAM variable, Registry).
  - Queries HVCI, Virtualization-Based Security (VBS), and Test Signing modes.
  - Detects attached kernel debuggers.
- **Hardware Identification (HWID)**:
  - Extracts immutable hardware IDs (SMBIOS Type 1 UUID, CPUID hashes, physical hard disk serial).
- **Communication Layer**:
  - Secure buffered IOCTL dispatcher connecting kernel detections to user-mode client.

---

## 2. What Ringo-AC Does NOT Do (Current Gaps and Security Caveats)

- **Does NOT Prevent Kernel-Mode Cheats (DKOM / Direct Kernel Memory Manipulation)**:
  - If a cheat runs as a signed/vulnerable driver or is mapped into kernel space, it can bypass `ObRegisterCallbacks` by directly manipulating `EPROCESS` or mapping physical memory via `\Device\PhysicalMemory` or page table manipulation (`CR3`).
- **Does NOT Perform Memory Signature / Pattern Scanning**:
  - The driver does not currently scan game virtual memory for known cheat byte signatures or injected shellcode.
- **Does NOT Validate Page Table Integrity in Usermode**:
  - Usermode memory hooks, inline detours, or manual mapped DLLs inside the game that do not trigger `LoadImageNotify` (e.g. manual mapped without calling `NtMapViewOfSection` / `LdrLoadDll`) are not actively scanned yet.
- **Does NOT Block Direct Kernel Object Handle Duplication from Kernel**:
  - Kernel-mode cheats can duplicate handles or directly call `MmCopyVirtualMemory` without passing through object manager hooks.
- **Does NOT Implement Communication Encryption / HMAC**:
  - The IOCTL protocol currently transmits plaintext structures. A malicious usermode application running with administrative privileges could theoretically issue IOCTLs to `\\.\ringo` if access control lists (DACLs) are not strictly enforced on `\Device\ringo`.

---

## 3. What is In Progress / Incomplete / Stubs

| Component | Path | Current State | Missing Functionality |
| :--- | :--- | :--- | :--- |
| **Big Pool Scanner** | `km/core/detection/manual_map_driver_scan/big_pool_track/bpt.cpp` | **Basic Query Only** | Currently only checks non-paged size > 4KB outside known modules. Needs to inspect whether memory contains executable code (`MZ`/`PE` headers, `PAGE_EXECUTE_READWRITE` / `PAGE_EXECUTE_READ`, or valid code pointers) to eliminate false positives. |
| **Kernel Space Walk** | `km/core/detection/manual_map_driver_scan/kernel_space_walk/kwalk.cpp` | **Empty Stub (0 bytes)** | Needs implementation of PML4/Page Table walker to traverse kernel page tables and detect unbacked executable kernel allocations. |
| **Vulnerable Drivers** | `km/core/detection/vulnerable_drivers/` | **Empty Directory** | Intended for hash/certificate blocklist of known vulnerable drivers (e.g., `gdrv.sys`, `mhyprot2.sys`, `RTCore64.sys`) frequently abused in BYOVD attacks. |
| **Suspicious Path Filter**| `km/core/detection/callbacks/image_notify/image_notify.hpp` | **Commented Out (`TODO`)** | The function `EsRutaSospechosa` (checking for `\Temp\`, `\AppData\Local\Temp\`, `\Downloads\`) is commented out and not linked into the detection pipeline. |
| **TPM Direct Probing** | `km/core/enviroment/tpm/tpm.cpp` | **Experimental** | Standard `ZwWriteFile`/`ZwReadFile` on `\Device\TPM` may return `STATUS_INVALID_DEVICE_REQUEST` on newer Windows builds without sending via TPM Base Services (TBS) IOCTL. |

---

## 4. Development Roadmap

### Phase 1: Robust Usermode Memory Scanning
- Implement usermode VAD (Virtual Address Descriptor) scanner to detect `PAGE_EXECUTE_READWRITE` / unbacked allocated memory in the game process.
- Implement thread call stack walking for threads created inside the game to verify return addresses point within legitimate module `.text` sections.

### Phase 2: Kernel Cheat and BYOVD Defense
- Implement PML4 page table walk in `kwalk.cpp` to discover unbacked executable kernel memory.
- Implement BYOVD blocklist monitor filtering driver loads (`PsSetLoadImageNotifyRoutine` on `ProcessId == 0` for kernel drivers) matching known vulnerable driver hashes and certificates.
- Implement PE header and import table validation on non-paged Big Pool allocations in `bpt.cpp`.

### Phase 3: Communication Hardening
- Apply restrictive Security Descriptor (SDDL) on `\Device\ringo` allowing access only to the game / anticheat client process token.
- Implement heartbeat ping and encrypted challenge-response handshake over IOCTL to prevent replay and spoofing attacks.
