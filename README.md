<div align="center">
  
  *ringo*
  
  <img src="docs/banner.jpg" alt="Banner" width="600">


[![license](https://img.shields.io/badge/license-MIT-blue?style=flat-square)](LICENSE)
[![language](https://img.shields.io/badge/language-C%2B%2B-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![platform](https://img.shields.io/badge/platform-Windows-0078D6?style=flat-square&logo=windows&logoColor=white)](https://www.microsoft.com)

  
  *a lightweight hybrid kernel-mode and user-mode Anti-Cheat for windows x64.*
</div


### Kernel Mode (`km`)
- **Process & Thread Protection (`ObRegisterCallbacks`)**: Strips unauthorized `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION`, and dangerous thread manipulation rights (`THREAD_SET_CONTEXT`, `THREAD_TERMINATE`).
- **Kernel Callbacks**:
  - `PsSetCreateProcessNotifyRoutineEx`: Detects target game launch and termination to track its PID.
  - `PsSetCreateThreadNotifyRoutineEx`: Logs thread creation, resolves Win32 `StartAddress`, and detects cross-process injections (`isCreatedRemotely`).
  - `PsSetLoadImageNotifyRoutineEx`: Filters and reports unsigned/suspicious DLLs mapped into the game.
- **Environment Integrity**:
  - **Secure Boot**: 3-way cross-check (System Information, UEFI NVRAM variable `SecureBoot`, Registry).
  - **Code Integrity**: HVCI, VBS, and Test-Signing status.
  - **Hypervisor**: CPUID leaf 1 vs `SystemHypervisorInformation` spoofing check.
  - **Kernel Debugging**: `KernelDebuggerEnabled` and `KernelDebuggerNotPresent` verification.
  - **TPM**: Version query (1.2 / 2.0).
- **Hardware Telemetry (HWID)**: Generates hardware identifier from SMBIOS Type 1 UUID, CPUID hash, and physical disk serial.
- **Manual Map Detection**: Big Pool allocator analysis (`BigPool_Detect`) to flag unbacked non-paged kernel code outside known drivers.

### User Mode (`um`)
- Console client communicating with the driver via `DeviceIoControl` (`\\.\ringo`).
- Configures target process name and queries environment telemetry.

---

## Project Structure

```
ringo-ac/
├── km/                                  # Kernel-Mode Driver (WDM / C++20)
│   ├── main.cpp                         # DriverEntry & DriverUnload
│   ├── sdk/imports/                     # NT structures & definitions
│   └── core/
│       ├── global.hpp / global.cpp      # Global state (RINGO_CONTEXT)
│       ├── comunication/ioctl/          # IOCTL dispatcher & codes
│       ├── detection/
│       │   ├── callbacks/               # ObCallbacks, Process, Thread, Image notify
│       │   └── manual_map_driver_scan/  # BigPool & driver scanners
│       ├── enviroment/                  # Secure Boot, HVCI, VBS, Hypervisor checks
│       └── telemetry/hwid/              # HWID extraction (SMBIOS, CPUID, Disk)
├── um/                                  # User-Mode Client (C++20)
│   ├── main.cpp                         # Client entry point
│   └── core/                            # IOCTL wrapper
└── docs/                                # Technical documentation
    ├── architecture.md                  # Context model & IOCTL protocol
    ├── detection_and_callbacks.md       # Callbacks & detection logic
    └── environment_and_telemetry.md     # Environment checks & HWID telemetry
```

---

## Build & Usage

### Requirements
- Visual Studio 2022 (C++20 standard)
- Windows Driver Kit (WDK) 10 / 11
- Windows x64 with Test Signing enabled or Kernel Debugger attached

### Build
1. Open the solution in Visual Studio 2022.
2. Select `Release | x64` or `Debug | x64`.
3. Build the solution (`km` and `um`).

### Run
```cmd
sc create ringo type= kernel binPath= "C:\path\to\km.sys"
sc start ringo
um.exe
```
