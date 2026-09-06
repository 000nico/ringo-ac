# Ringo Anti-Cheat (Ringo-AC)

A lightweight, hybrid Kernel-Mode (KM) and User-Mode (UM) Anti-Cheat system designed for Windows x64. Ringo-AC focuses on proactive process protection, telemetry collection, environment integrity validation, and kernel-level cheat detection (manual mapped drivers and unbacked memory).

```
   +-------------------------------------------------------------+
   |                        User Mode (UM)                       |
   |   - AC Client / Service Agent                               |
   |   - IOCTL Interface Wrapper                                 |
   |   - Environment & Telemetry Verification Consumer           |
   +------------------------------+------------------------------+
                                  |
                           IOCTL (DeviceIoControl)
                                  |
   +------------------------------v------------------------------+
   |                       Kernel Mode (KM)                      |
   |  [ObCallbacks]          [Kernel Callbacks]    [Environment] |
   |  - Handle Stripping     - Process Lifecycle   - Secure Boot |
   |  - Process/Thread Ob    - Remote Threads      - HVCI / VBS  |
   |                         - DLL / Image Loads   - TPM / Dbg   |
   |                                                             |
   |  [Driver Scanner]       [Telemetry / HWID]    [Core Engine] |
   |  - Big Pool Tracking    - SMBIOS UUID         - Ringo Ctx   |
   |  - Module Base Scanner  - Disk Serial / CPUID - Dispatcher  |
   +-------------------------------------------------------------+
```

---

## 📑 Documentation Index

Detailed technical specifications are divided across the following dedicated documents:

- 🏗️ **[System Architecture & IOCTL Interface](docs/architecture.md)** — Architectural design, Driver Entry/Unload lifecycle, memory model, and IOCTL communication protocol.
- 🛡️ **[Detection Engine & Callbacks](docs/detection_and_callbacks.md)** — In-depth breakdown of `ObRegisterCallbacks`, Process/Thread/Image notify routines, and Big Pool scanning.
- 🔒 **[Environment Integrity & Hardware Telemetry](docs/environment_and_telemetry.md)** — Multi-source Secure Boot verification, HVCI/VBS query internals, TPM probing, and HWID generation.
- 🚧 **[Project Status, Limitations & Roadmap](docs/roadmap_and_limitations.md)** — Explicit analysis of what the AC does **now**, what is **WIP / incomplete**, and current security limitations.

---

## ⚡ Current Capability Matrix

| Component | Subsystem | Status | Implementation Details |
| :--- | :--- | :--- | :--- |
| **Object Protection** | `ObRegisterCallbacks` | ✅ **Functional** | Strips `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION` and thread context access rights. |
| **Process Tracking** | `PsSetCreateProcessNotifyRoutineEx` | ✅ **Functional** | Tracks target game process creation and destruction to update global context state. |
| **Thread Tracking** | `PsSetCreateThreadNotifyRoutineEx` | ✅ **Functional** | Logs thread creation, detects remote threads, and extracts Win32 start address. |
| **Module Tracking** | `PsSetLoadImageNotifyRoutineEx` | ✅ **Functional** | Intercepts DLL loads in the target game, tracking base addresses, sizes, and signing levels. |
| **HWID Engine** | Telemetry | ✅ **Functional** | Generates hardware identity using SMBIOS Type 1 UUID, CPUID hashes, and physical disk serials. |
| **Environment Check** | Hypervisor Detection | ✅ **Functional** | Compares `ZwQuerySystemInformation` against CPUID Leaf 1 bit 31 to flag information manipulation. |
| **Environment Check** | Secure Boot Check | ✅ **Functional** | 3-way check (System Information, UEFI variable `Secure Boot`, and Registry key) with consistency validation. |
| **Environment Check** | HVCI / VBS / TestSigning | ✅ **Functional** | Validates code integrity options and Isolated User Mode / Secure Kernel status. |
| **Environment Check** | Kernel Debugger Check | ✅ **Functional** | Inspects `KernelDebuggerEnabled` and `KernelDebuggerNotPresent` state. |
| **Environment Check** | TPM Version Probe | ⚠️ **Experimental** | Queries `\Device\TPM` with TPM 2.0 / 1.2 commands. |
| **Driver Detection** | Big Pool Tracking | ⚠️ **Partial** | Enumerates non-paged big pool allocations and compares against legitimate module ranges. |
| **Driver Detection** | Kernel Space Walk | ❌ **Stub** | Empty placeholder (`kwalk.cpp`). |
| **Driver Detection** | Vulnerable Driver Blocklist | ❌ **Stub** | Unimplemented directory placeholder. |

---

## 🛠️ Repository Layout

```
ringo-ac/
├── km/                                  # Kernel-Mode Driver (WDM / C++20)
│   ├── main.cpp                         # DriverEntry & DriverUnload definitions
│   ├── km.inf / km.vcxproj              # Driver build definition & package configuration
│   ├── sdk/imports/                     # Undocumented NT structures and NTOS API imports
│   └── core/
│       ├── global.hpp / global.cpp      # Global runtime context (RINGO_CONTEXT)
│       ├── comunication/ioctl/          # IOCTL dispatcher, codes, and device setup
│       ├── detection/
│       │   ├── callbacks/               # Object, Process, Thread, and Image notify callbacks
│       │   │   ├── ob_callbacks/        # ObRegisterCallbacks (Process & Thread protection)
│       │   │   ├── create_process_notify/# Process creation/exit lifecycle hook
│       │   │   ├── create_thread_notify/# Thread creation & remote thread detector
│       │   │   └── image_notify/        # DLL load monitor
│       │   ├── manual_map_driver_scan/  # Kernel memory scanners (BigPool & Module walk)
│       │   └── vulnerable_drivers/      # Known vulnerable driver blocklist (Stub)
│       ├── enviroment/                  # System security & hypervisor integrity checks
│       └── telemetry/hwid/              # Hardware ID extraction (SMBIOS, CPUID, Disk)
├── um/                                  # User-Mode Console Client (C++20)
│   ├── main.cpp                         # UM entry point & verification output
│   └── core/                            # Driver handle manager & IOCTL query wrappers
└── docs/                                # Technical documentation
    ├── architecture.md
    ├── detection_and_callbacks.md
    ├── environment_and_telemetry.md
    └── roadmap_and_limitations.md
```

---

## 🚀 Building and Deployment

### Prerequisites
- **Visual Studio 2022** with **C++ Desktop Development** and **C++20** standard enabled.
- **Windows Driver Kit (WDK) 10 / 11** matching your installed Windows SDK.
- Target machine with **Test Signing enabled** or configured with a Kernel Debugging connection (KD/WinDbg).

### Compiling
1. Open the solution in Visual Studio 2022.
2. Select target configuration `Debug|x64` or `Release|x64`.
3. Build the solution (both `km` and `um` projects).
   > **Note:** The `km` project includes `/INTEGRITYCHECK` linker flag required for `ObRegisterCallbacks` to succeed without `STATUS_ACCESS_DENIED`.

### Deploying Driver to Test VM
Scripts are included in the repository root for automated testing:
- [`deploy-config.json`](file:///C:/dev/ringo-ac/deploy-config.json): Configuration file for target VM name and paths.
- [`deploy.ps1`](file:///C:/dev/ringo-ac/deploy.ps1): PowerShell deployment automation.
- [`debug_deploy_driver_vm.bat`](file:///C:/dev/ringo-ac/debug_deploy_driver_vm.bat): Installs and starts the service on the test environment.
- [`debug_delete_driver_vm.bat`](file:///C:/dev/ringo-ac/debug_delete_driver_vm.bat): Stops and uninstalls the driver service.
