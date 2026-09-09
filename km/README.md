# Ringo — Kernel-Mode Anti-Cheat Driver

Ringo is a Windows kernel-mode driver written in C++ using the Windows Driver Kit (WDK). It forms the privileged half of a game anti-cheat system, running at Ring 0 to enforce protections and collect telemetry that user-mode code cannot reliably provide or tamper with. A companion user-mode service communicates with the driver through a private IOCTL interface.

---

## Overview

The driver serves two broad purposes: **active protection** and **passive telemetry**. On the protection side it intercepts handle operations against the protected game process and its threads, stripping dangerous access rights before they are granted. On the telemetry side it monitors driver loads, thread creation, and DLL injection events, records anomalies in shared in-kernel report structures, and queries the platform security state (Secure Boot, HVCI, VBS, TPM, hypervisor presence, kernel debugger) so the user-mode service can make trust decisions without relying on data it cannot independently verify.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     User-Mode Service                       │
│              DeviceIoControl  (\\.\\ringo)                   │
└────────────────────────┬────────────────────────────────────┘
                         │  IOCTL (METHOD_BUFFERED)
┌────────────────────────▼────────────────────────────────────┐
│                  Ringo Kernel Driver  (km.sys)               │
│                                                             │
│  ┌──────────────┐  ┌───────────────┐  ┌─────────────────┐  │
│  │  Callbacks   │  │  Detection    │  │  Environment    │  │
│  │  ─────────── │  │  ─────────── │  │  ─────────────  │  │
│  │  ObCallbacks │  │  BYOVD scan  │  │  Secure Boot    │  │
│  │  ImageNotify │  │  BigPool scan│  │  HVCI / VBS     │  │
│  │  ProcNotify  │  │  KWalk (WIP) │  │  Hypervisor     │  │
│  │  ThreadNotify│  │               │  │  TPM            │  │
│  └──────────────┘  └───────────────┘  │  Kernel Debug   │  │
│                                       │  Test Signing   │  │
│  ┌──────────────────────────────┐     └─────────────────┘  │
│  │  RINGO_CONTEXT (global state)│                           │
│  │  Game info · Reports · Cbs   │                           │
│  └──────────────────────────────┘                           │
└─────────────────────────────────────────────────────────────┘
```

All mutable shared state lives in a single `RINGO_CONTEXT` global. Reports accumulate in fixed-size arrays inside that struct; the user-mode service reads them out via IOCTL on demand.

---

## Components

### Callbacks (`core/detection/callbacks/`)

**ObCallbacks** (`ob_callbacks/`) registers pre-operation handlers on `PsProcessType`, `PsThreadType`, and `IoDeviceObjectType`. For the protected game process it strips `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, and `PROCESS_VM_OPERATION` from any handle opened by a third party, and does the same for the most dangerous thread rights (`THREAD_TERMINATE`, `THREAD_GET_CONTEXT`, `THREAD_SET_CONTEXT`, etc.). For BYOVD-flagged device objects it zeroes the `DesiredAccess` entirely, preventing tools from interacting with a known-vulnerable driver through its exposed interface.

**Image Load Notify** (`image_notify/`) fires on every kernel image and user-mode image load. When `ProcessId` is `NULL` or `4` (the System process), the image is a kernel driver; the callback computes its hashes and checks against the BYOVD blocklist. When the image belongs to the protected game process, it records unsigned DLL loads in the `FLAGGED_DLL_REPORT`.

**Create Process Notify** (`create_process_notify/`) tracks process creation and destruction to keep `ringo_context.Game` current. Rather than requiring the user-mode service to hand over a PID, the driver matches on the process image name, so it becomes aware of game start and stop events autonomously.

**Create Thread Notify** (`create_thread_notify/`) records every new thread created in the game process, capturing the start address and whether the thread was created from a different process (`isCreatedRemotely`). This is the primary signal for detecting remote thread injection without relying on user-mode hooks.

### Detection (`core/detection/`)

**BYOVD Detection** (`vulnerable_drivers/`) identifies Bring-Your-Own-Vulnerable-Driver attacks. A hash blocklist of several thousand known-vulnerable drivers ships in `blocked_drivers.hpp` as a compile-time array of `{SHA256, Authentihash}` pairs. Detection can happen at two points: via the image-load callback when a driver is loaded (real-time), and via a full scan of all currently loaded modules at startup (`DetectVulnerableDriversAtRuntime`). When a match is found, a `BYOVD_DETECTION_ENTRY` is written and the device object is referenced so that the ObCallback layer can enforce zero-access on any subsequent handle to it.

**Manual-Map Driver Scan** (`manual_map_driver_scan/`) looks for kernel code that was injected without going through the normal driver loading path. The Big Pool tracker (`big_pool_track/`) queries `SystemBigPoolInformation`, compares every large non-paged allocation against the list of legitimately loaded modules, and flags anything executable that is not accounted for. A curated whitelist of known-legitimate pool tags filters out OS and driver allocations that would otherwise produce false positives. A kernel-space walker (`kernel_space_walk/`) is planned as a second method.

### Environment Probes (`core/enviroment/`)

Each subdirectory is a self-contained probe that the IOCTL dispatcher calls on demand. They report on HVCI, VBS, Secure Boot, test-signing mode, hypervisor presence, kernel debugger attachment, and TPM version. The results are returned as fixed-size structs over the IOCTL interface; the user-mode service decides what to do with the information.

### IOCTL Interface (`core/comunication/ioctl/`)

The driver exposes a named device `\Device\ringo` and symbolic link `\\.\ringo`. All communication is `METHOD_BUFFERED`. The IOCTL table is split between getters (environment probes, game state, HWID, callback status) and setters (`IOCTL_SET_GAME_NAME` being the primary one). The codes file (`codes.hpp`) is shared verbatim with the user-mode project to avoid duplication.

### Crypto (`sdk/crypto/`)

`ComputeDriverHashes` reads a driver file from disk into a non-paged pool buffer and computes two digests: a raw SHA-256 over the full file, and an Authenticode hash (Authentihash). The latter is computed by the driver itself rather than delegating to CI.dll, following the PE Authenticode specification: hashing everything except the PE checksum field, the Security Directory entry in the data directory, and the embedded signature blob itself. Section data is sorted by `PointerToRawData` before hashing, matching the Windows loader's behaviour.

### Telemetry (`core/telemetry/hwid/`)

Collects hardware identifiers to build a per-machine fingerprint that the user-mode service uses for session binding and ban enforcement.

---

## Security Model

The driver is intended to run only when a WHQL-signed or EV-signed driver certificate is in use. Test-signing mode is detected and reported as a degraded trust state. The following properties hold by design:

- Kernel handles are excluded from the ObCallback handler so legitimate kernel-mode drivers and system services are never affected by access stripping.
- All detection reports are write-once and accumulate in kernel memory. They are not modified from user mode; the service only reads them. This prevents a compromised user-mode component from erasing evidence.
- The BYOVD device object is `ObReferenceObject`'d when a match is detected, ensuring the object cannot be freed from under the ObCallback even if the driver attempts to unload.
- The IOCTL dispatcher validates buffer sizes before reading or writing any output, and rejects requests where the system buffer pointer is null despite a non-zero length claim.

---

## Technical Decisions

**Why `ObRegisterCallbacks` on `IoDeviceObjectType`, not just process/thread types?**
Standard game protection uses ObCallbacks on `PsProcessType` to strip memory access. Adding `IoDeviceObjectType` extends this to BYOVD scenarios: once a vulnerable driver is identified, any attempt by a cheat to open its device object and issue control codes through it is blocked before the handle is even returned. This is more reliable than trying to unload the vulnerable driver, which may fail due to reference counts or cause a crash if the driver is already in use.

**Why Authentihash alongside SHA-256?**
SHA-256 of the raw file byte-for-byte is unstable across repacked or modified binaries that preserve the same certificate. Authenticode hash covers the semantically-signed regions only, which means it matches what Windows code integrity checks — and more importantly it matches what the blocklist maintainers computed when cataloguing a vulnerable driver. Checking both maximises coverage: raw SHA-256 catches unmodified drops, Authentihash catches repacked variants that keep a valid signature.

**Why `PsSetLoadImageNotifyRoutineEx` instead of `PsSetLoadImageNotifyRoutine`?**
`PsSetLoadImageNotifyRoutineEx` with flag `0` filters out image-load notifications for images that are already mapped (e.g. shared sections), reducing unnecessary callbacks. More importantly it is the modern API available from Windows 10 1703 onwards and aligns with the expected calling convention; the legacy variant has subtly different behaviour around when `FullImageName` is guaranteed to be populated.

**Why `PsSetCreateThreadNotifyRoutineEx` with `PsCreateThreadNotifyNonSystem`?**
System threads created by the kernel itself generate significant noise and are never relevant to cheat detection within a user-mode game process. Filtering them at registration rather than inside the callback avoids the overhead of entering the callback routine thousands of times per second on a busy system.

**Why `InterlockedIncrement` for detection report indices?**
Kernel callbacks fire concurrently on any available processor. A plain `count++` compiled to a non-atomic increment-and-store sequence can produce the same index on two cores simultaneously, causing one entry to overwrite another or the count to skip a slot. `InterlockedIncrement` returns the new value atomically, so each callback gets a unique index. The bounds check happens immediately after, before any write.

**Why is the game PID tracked in kernel state rather than sent from user mode via IOCTL?**
Receiving a PID from user mode and trusting it creates a TOCTOU window: a cheat could race the IOCTL to send a different PID. Tracking it through `PsSetCreateProcessNotifyRoutineEx` by matching on the process image name means the kernel is always the authoritative source of `ProtectedGamePid`, and the match happens at process-creation time before any user-mode code in that process has run.

---

## Build & Requirements

- Windows Driver Kit (WDK), matching the Windows 10/11 SDK in use.
- Visual Studio 2022 with the "Desktop development with C++" and "Windows Driver Kit" workloads.
- Target platform: x64 only. 32-bit is not supported (the Authentihash implementation uses `IMAGE_NT_HEADERS64` exclusively).
- Driver signing: a valid EV code-signing certificate is required for deployment. Test-signing (`bcdedit /set testsigning on`) works for development but is flagged by the environment probes.
- Minimum supported OS: Windows 10 1703 (`PsSetLoadImageNotifyRoutineEx` availability).

The solution file is `km.slnx`. Build configuration and driver INF are in `km.vcxproj` and `km.inf` respectively.
