# Detection Engine and Callbacks

This document covers each detection mechanism in the Ringo kernel driver: the object callback layer that enforces handle access restrictions, the kernel notify routines that monitor process, thread, and module activity, and the memory scanners that look for injected kernel code.

---

## Object Callbacks

Source: `km/core/detection/callbacks/ob_callbacks/obcallback.cpp`

Object callbacks are the most directly protective mechanism in the driver. They intercept handle operations inside the kernel's Object Manager before a handle is returned to any caller, allowing the driver to modify the access rights that will be granted.

The driver registers a single pre-operation callback (`OpenProcessCallback`) against three object types: `PsProcessType`, `PsThreadType`, and `IoDeviceObjectType`. Registration is at altitude `"321000"` and covers both `OB_OPERATION_HANDLE_CREATE` and `OB_OPERATION_HANDLE_DUPLICATE`.

The callback logic is layered. First, kernel-mode handles are passed through unconditionally — the filter is not meant to affect other drivers or system services, only user-mode processes. Second, if the game is not open or has no PID, there is nothing to protect and the callback returns immediately. For surviving requests, the target PID determines which restriction applies:

For handles targeting the game process from a different process, `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, and `PROCESS_VM_OPERATION` are stripped. These are the three rights required by a cheat to read or write game memory. The remaining process rights — including `PROCESS_QUERY_INFORMATION` — are preserved, because stripping them breaks legitimate tooling like task managers and debuggers attached for valid reasons.

For handles targeting threads inside the game process, a broader set of rights is stripped: terminate, suspend/resume, get context, set context, set information, and direct impersonation. These cover the primary mechanisms used to inject code into or hijack threads in the target.

For handles targeting device objects in the BYOVD detection list, `DesiredAccess` is set to zero entirely. The intent is to make the vulnerable driver's interface completely inaccessible through normal handle operations. This is more robust than trying to unload the driver itself, which can fail silently or cause a bugcheck if the driver holds references.

The restriction applies to both creation and duplication. Duplicating an existing handle with elevated rights is the classic bypass for creation-time filtering.

---

## Process Lifecycle Monitoring

Source: `km/core/detection/callbacks/create_process_notify/create_process_notify.cpp`

The process-creation callback uses `PsSetCreateProcessNotifyRoutineEx`, which provides a `PS_CREATE_NOTIFY_INFO` pointer on process creation (non-null) and a null pointer on process exit. The callback extracts the short image name with `PsGetProcessImageFileName` and compares it case-insensitively against the configured `ProtectedGameProcessName`.

On game start, `RINGO_CONTEXT.Game.ProtectedGamePid` is set from the `ProcessId` parameter and `isGameOpen` is set to true. On game exit, if the exiting PID matches, both fields are cleared. This makes the driver's view of the protected process completely self-maintaining — it does not depend on the user-mode service to report process state.

The choice to track by image name rather than accepting a PID from user mode eliminates a race condition. A cheat tool that can issue IOCTLs to the driver before the game starts could supply a spoofed PID. Tracking at process-creation time means the kernel makes the association before the process's own user-mode code has run.

---

## Thread Creation Monitoring

Source: `km/core/detection/callbacks/create_thread_notify/create_thread_notify.cpp`

Registered via `PsSetCreateThreadNotifyRoutineEx` with the `PsCreateThreadNotifyNonSystem` flag, which suppresses callbacks for kernel system threads. System threads are created frequently by the kernel itself and are never relevant to game-process detection; filtering them at registration avoids the overhead of entering the callback to immediately discard the event.

For each thread created in the game process, the callback captures the Win32 start address by opening a temporary kernel handle with `ObOpenObjectByPointer` and querying `ZwQueryInformationThread` with `ThreadQuerySetWin32StartAddress`. It also records whether the creating process differs from the target process (`PsGetCurrentProcessId() != ProcessId`), which is the direct signal for remote thread creation — the primary user-mode mechanism for code injection.

Reports are written into `RINGO_CONTEXT.Detections.ThreadDetectionReport` using `InterlockedIncrement` to obtain a unique slot index safely across concurrent callbacks.

---

## Image Load Monitoring

Source: `km/core/detection/callbacks/image_notify/image_notify.cpp`

The image-load callback fires on every module mapped into any address space. The callback disambiguates between two cases based on `ProcessId`.

When `ProcessId` is `NULL` or `4`, the image is being loaded as a kernel driver (into the System process). The driver reads the file from disk, computes its SHA-256 and Authentihash digests, and checks both against the BYOVD blocklist. If a match is found, the driver resolves the driver's `DRIVER_OBJECT` by path, takes a reference to its `DeviceObject`, and records it in the BYOVD detection report so the object callback layer can enforce zero-access on subsequent handles to that device.

When `ProcessId` matches the game's PID, the callback is looking at a module being loaded into the game's address space. The `ImageSignatureLevel` field in `PIMAGE_INFO` is checked; any module with a signing level of `SE_SIGNING_LEVEL_UNSIGNED` or below is recorded in the DLL detection report for the user-mode service to evaluate.

---

## BYOVD Detection

Source: `km/core/detection/vulnerable_drivers/byovd_scan.cpp`  
Blocklist: `km/core/detection/vulnerable_drivers/blocked_drivers.hpp`

Bring-Your-Own-Vulnerable-Driver attacks use legitimate but exploitable drivers to gain kernel-level code execution without needing a signed malicious driver. Ringo defends against this by maintaining a blocklist of known-vulnerable drivers and refusing to grant handles to their device objects once they are identified.

The blocklist (`blocked_drivers.hpp`) is a compile-time array of `VULNERABLE_DRIVER_ENTRY` structs, each holding a SHA-256 digest, an Authentihash digest, and a filename. It is auto-generated from the loldrivers community dataset via `fill_blocked_drivers_list.py` and currently contains around 2,000 entries. Both hash types are stored because they catch different cases: raw SHA-256 matches unmodified dropped binaries, while Authentihash matches repacked variants that preserve the original certificate but differ byte-for-byte.

Detection can happen at two points. At driver startup, `DetectVulnerableDriversAtRuntime` enumerates all loaded kernel modules, reads each from disk, and checks its hashes against the blocklist. This catches vulnerable drivers already present when the anti-cheat loads. After startup, the image-load callback handles new drivers as they load in real time.

When either path identifies a match, the code calls `GetDriverObjectFromImageName` to resolve the driver's `\Driver\<name>` object, takes a reference to the first device object in the driver's device stack, and passes both to `AddVulnerableDriverDetectionEntry`. The reference on the device object is held for the lifetime of the driver so the object callback can safely dereference it; releasing it without deregistering the callback first would create a use-after-free.

---

## Manual-Map Driver Scanning

Source: `km/core/detection/manual_map_driver_scan/`

Manual-mapped drivers are injected directly into kernel memory without going through the Windows driver loader. Because they bypass `PsSetLoadImageNotifyRoutine` (which only fires for properly loaded images), they do not appear in the module list and require a different detection strategy.

**Legitimate Driver Baseline** (`driver_scan.cpp`)

`GetLegitDriverList` queries `ZwQuerySystemInformation(SystemModuleInformation)` and builds a `LOADED_LEGIT_DRIVERS` structure in `NonPagedPoolNx` containing the base address and size of every legitimately loaded kernel module. This baseline is the reference for all subsequent checks.

**Big Pool Tracker** (`big_pool_track/bpt.cpp`)

`ZwQuerySystemInformation(SystemBigPoolInformation)` returns every allocation tracked by the kernel's big pool allocator — allocations above approximately 4 KB that use the large-pool path. The scanner iterates this list and flags any entry where:
- the pool type is non-paged (`NonPaged == 1`),
- the size exceeds one page (`SizeInBytes > 0x1000`), and
- the base address falls outside every address range in the legitimate module baseline.

A whitelist of known-safe pool tags (`whitelistTags.hpp`) is applied to suppress noise from OS and driver allocations that legitimately do not correspond to a loaded module.

Flagged entries are written as `DETECTION_ENTRY` records with `METHOD_BIG_POOL`. The current implementation detects memory that is unaccounted-for but does not yet inspect whether it contains executable code; PE header inspection and page-permission checks are planned improvements to reduce false positives.

**Kernel Space Walk** (`kernel_space_walk/kwalk.cpp`)

This is a planned second method. The intent is to walk the PML4 page table directly and find executable kernel pages that are not backed by any known module. The file is currently empty.
