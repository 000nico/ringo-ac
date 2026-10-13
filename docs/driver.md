# ringo driver

The kernel-mode component of ringo (`rngdrv.sys`). A WDM driver for Windows 10/11 x64 that collects data from the system, registers the callbacks, runs the detections and hands everything to the user-mode client through a protected device.

## Contents

1. [Introduction](#1-introduction)
2. [Lifecycle](#2-lifecycle)
   - 2.1. [DriverEntry](#21-driverentry)
   - 2.2. [DriverUnload](#22-driverunload)
3. [Runtime context](#3-runtime-context)
4. [Communication](#4-communication)
   - 4.1. [The device](#41-the-device)
   - 4.2. [Opening the device: sessions](#42-opening-the-device-sessions)
   - 4.3. [Verifying every request](#43-verifying-every-request)
   - 4.4. [Banning a caller](#44-banning-a-caller)
   - 4.5. [IOCTL list](#45-ioctl-list)
5. [Callbacks](#5-callbacks)
   - 5.1. [Process creation](#51-process-creation)
   - 5.2. [Thread creation](#52-thread-creation)
   - 5.3. [Image load](#53-image-load)
   - 5.4. [Object callbacks](#54-object-callbacks)
6. [Environment checks](#6-environment-checks)
   - 6.1. [Multiple sources per check](#61-multiple-sources-per-check)
   - 6.2. [Checks list](#62-checks-list)
7. [Identity](#7-identity)
8. [Detections](#8-detections)
   - 8.1. [BYOVD](#81-byovd)
   - 8.2. [Manual mapped drivers](#82-manual-mapped-drivers)
   - 8.2.1. [Big pool tracking](#821-big-pool-tracking)
   - 8.2.2. [Kernel thread walking](#822-kernel-thread-walking)
   - 8.3. [Integrity](#83-integrity)
   - 8.4. [Unsigned DLLs in the game](#84-unsigned-dlls-in-the-game)
   - 8.5. [Threads created in the game](#85-threads-created-in-the-game)
9. [Scheduler](#9-scheduler)
10. [Reporting](#10-reporting)
11. [Heartbeat](#11-heartbeat)
12. [Known limitations and planned work](#12-known-limitations-and-planned-work)
13. [Building](#13-building)
14. [Acronyms](#14-acronyms)

## 1. Introduction

The driver does not decide anything. It observes, and reports what it saw. Every verdict is made by the server (see [server.md](server.md)), and the user-mode client (see [client.md](client.md)) only carries data between the two.

The driver is split by responsibility:

```
driver/
  main.cpp                    DriverEntry / DriverUnload
  core/
    global.hpp                the single RINGO_CONTEXT
    communication/            device, sessions, IOCTL dispatcher, caller verification
    callbacks/                process, thread, image and object callbacks
    system/                   environment checks (secure boot, vbs, hvci, ...)
    identity/                 HWID
    detection/                byovd, manual mapped drivers, integrity
    reporting/                fixed size report buffers filled by the detections
    scheduler/                single thread that runs the periodic detections
    heartbeat/                liveness counter
  sdk/                        PE parser, SHA-256 / authentihash, helpers
```

Two decisions shape the whole driver:

- **No module is injected into the game.** Everything is done from kernel mode, so the anti-cheat also works for community managed servers where nothing can be installed in the game.
- **No self protection.** Anti-debugging, anti-dump and similar protections are the responsibility of a packer, which keeps this codebase small and readable.

## 2. Lifecycle

### 2.1. DriverEntry

`DriverEntry` runs the following steps in order:

1. **Object callbacks.** `ob_register_callbacks_open_process()` registers the pre-operation handlers (5.4). The returned handle is stored in `g_ringo_context.callbacks.ob.callback_handle`.
2. **Image load callback.** `PsSetLoadImageNotifyRoutineEx` (5.3).
3. **Thread creation callback.** `PsSetCreateThreadNotifyRoutineEx` with `PsCreateThreadNotifyNonSystem` (5.2).
4. **Process creation callback.** `PsSetCreateProcessNotifyRoutineEx` (5.1).
5. **Device.** `setup_device()` creates `\Device\ringo` and its symbolic link (4.1).
6. **Scheduler.** The stop event is initialised and the scheduler system thread is created (9).

If any callback fails to register, the driver logs it and keeps loading. A missing callback reduces coverage but must not prevent the rest from working, and the client can ask whether each callback is set (`IOCTL_IS_*_CALLBACK_SET`) so the server knows the coverage of every machine.

### 2.2. DriverUnload

Unloading is the reverse of loading, with one rule: **the scheduler goes first**. The thread is running driver code, so unloading with it alive would crash the system.

```
scheduler_stop = TRUE
KeSetEvent(g_stop_event)            -> wakes the thread from its sleep
KeWaitForSingleObject(thread)       -> waits until it has terminated
ObUnRegisterCallbacks(...)
PsRemoveLoadImageNotifyRoutine(...)
PsRemoveCreateThreadNotifyRoutine(...)
PsSetCreateProcessNotifyRoutineEx(..., TRUE)
cleanup_device()
```

Each callback is only removed if its `is_registered` flag is set.

## 3. Runtime context

All mutable state lives in one global, `g_ringo_context` (`core/global.hpp`):

```cpp
typedef struct RINGO_CONTEXT {
    struct { ... } game;        // protected process: pid, name, is_game_open, last_image_hash
    struct { ... } callbacks;   // registration state of every callback
    struct { ... } detections;  // the five report buffers (section 10)
    struct { ... } session;     // the one client allowed to talk to the driver
    struct { ... } scheduler;   // stop flag, thread handle, random seed
} RINGO_CONTEXT;
```

A single context keeps the state easy to follow: nothing is hidden in file-level statics, so reading `global.hpp` is enough to know everything the driver remembers.

## 4. Communication

The driver talks to exactly one user-mode process through a device. This is the main attack surface of the project, so most of the checks live here.

### 4.1. The device

```cpp
IoCreateDeviceSecure(DriverObject, 0, &device_name, FILE_DEVICE_UNKNOWN,
                     FILE_DEVICE_SECURE_OPEN, TRUE /* Exclusive */,
                     &sddl, nullptr, &g_device_object);
```

- **Exclusive = TRUE.** Once a process holds a handle to the device, no other process can open one.
- **`FILE_DEVICE_SECURE_OPEN`** applies the security descriptor to every open, including opens of names under the device.
- **SDDL `D:P(A;;GA;;;SY)(A;;GA;;;BA)`** grants access only to `SYSTEM` and the local administrators.
- `DO_BUFFERED_IO` is used: the I/O manager copies the buffers, and the driver never touches user-mode memory directly.

Only `IRP_MJ_CREATE`, `IRP_MJ_CLOSE` and `IRP_MJ_DEVICE_CONTROL` are implemented. Every other major function returns `STATUS_NOT_SUPPORTED`.

The control codes live in `communication/codes.hpp`, which is included by the driver and the client, so both sides always agree.

### 4.2. Opening the device: sessions

Opening the device (`irp_create`) is where the driver decides whether the caller is trustworthy. It computes two hashes of the requestor's `.text` section:

- **file hash:** from the executable on disk, resolved by path (`SeLocateProcessImageName` + `read_pe_from_disk`).
- **process hash:** from the `FILE_OBJECT` of the image that is actually mapped into the process (`PsReferenceProcessFilePointer` + `read_pe_from_handle`). Going through the file object instead of re-resolving a path avoids a race where the file is swapped between the check and the read.

The open succeeds only if all of these hold:

```
file hash computed       AND
process hash computed    AND
file hash matches g_expected_file_hash (or none has been provisioned yet) AND
the process is not banned
```

On success the **session** is created:

```cpp
session.active            = TRUE;
session.process           = requestor;                       // referenced
session.process_start_key = PsGetProcessStartKey(requestor); // identifies this exact process instance
session.last_hash         = process hash;
```

`process_start_key` matters because PIDs and `EPROCESS` addresses can be reused. The start key is unique per process instance.

**Provisioning the expected hash.** While `g_expected_file_hash` is all zeroes the check is skipped, so the client can set it once with `IOCTL_SET_FILE_HASH`. From then on only an executable with that `.text` hash can open the device.

The session is closed (and the process reference released) in three places: `IRP_MJ_CLOSE`, a failed open, and the process-exit notification (5.1).

### 4.3. Verifying every request

Opening the device once is not enough: a handle can be duplicated or the process modified afterwards. `ioctl_handler` therefore repeats the checks on **every** request, before looking at the control code:

```
session is active
 -> caller == session.process
 -> PsGetProcessStartKey(caller) == session.process_start_key
 -> process hash of the caller (recomputed now) == session.last_hash
 -> caller is not banned
 -> only then: dispatch the IOCTL
```

The hash is recomputed from the mapped image each time and the stored value is refreshed afterwards. A caller that patched its own `.text` after opening the device fails the next request.

```
user-mode process                         driver
        |                                   |
        |-- CreateFile(\\.\ringo) --------->|  irp_create: hash file + process,
        |                                   |  compare with expected, open session
        |-- DeviceIoControl(code) --------->|  ioctl_handler:
        |                                   |    session / start key / process hash / ban list
        |                                   |    switch (code)
        |<-- buffer ------------------------|
```

*Figure 1. Verification on open and on every request.*

### 4.4. Banning a caller

If a request arrives with a control code the driver does not know, the caller is treated as hostile (a client built from this repository never sends one). The process is added to `session.banned.banned_peprocess[128]`, the session is closed and the stored hash is cleared. Every later `CreateFile` from that process is refused, which makes fuzzing the dispatcher impractical.

### 4.5. IOCTL list

All codes use `METHOD_BUFFERED`, `FILE_DEVICE_UNKNOWN` and base `0x8000`.

| Control code | Dir | Payload | Description |
|---|---|---|---|
| `IOCTL_GET_HWID` | out | `HWID` (24 bytes) | SMBIOS UUID, CPU hash, disk serial |
| `IOCTL_GET_GAME_PID` | out | `HANDLE` | Tracked game PID |
| `IOCTL_GET_GAME_STATUS` | out | `BOOLEAN` | Game process is alive |
| `IOCTL_GET_HVCI_INFORMATION` | out | `HVCI_CHECK` | HVCI / KMCI |
| `IOCTL_GET_HYPERVISOR_STATUS` | out | `HYPERVISOR_CHECK` | Hypervisor presence, two sources |
| `IOCTL_GET_KERNEL_DEBUGGING_STATUS` | out | `KERNEL_DEBUGGING_CHECK` | Kernel debugger |
| `IOCTL_GET_SECURE_BOOT_INFORMATION` | out | `SECURE_BOOT_CHECK` | Secure Boot, four sources |
| `IOCTL_GET_TEST_SIGNING_MODE` | out | `TEST_SIGNING_CHECK` | Test signing |
| `IOCTL_GET_VBS_INFORMATION` | out | `VBS_CHECK` | VBS / secure kernel |
| `IOCTL_GET_TPM_INFORMATION` | out | `TPM_CHECK` | TPM 1.2 / 2.0 |
| `IOCTL_IS_CREATE_PROCESS_CALLBACK_SET` | out | `BOOLEAN` | Callback registered |
| `IOCTL_IS_CREATE_THREAD_CALLBACK_SET` | out | `BOOLEAN` | Callback registered |
| `IOCTL_IS_IMAGE_CALLBACK_SET` | out | `BOOLEAN` | Callback registered |
| `IOCTL_IS_OBCALLBACK_SET` | out | `BOOLEAN` | Callback registered |
| `IOCTL_GET_HEARTBEAT` | out | `HEARTBEAT` | Increments and returns the counter |
| `IOCTL_SET_GAME_NAME` | in | `char[]` | Image name of the protected process. Accepted once |
| `IOCTL_SET_FILE_HASH` | in | 32 bytes | Expected `.text` SHA-256 of the client (4.2) |
| `IOCTL_GET_BYOVD_REPORT` | out | `BYOVD_DETECTION_REPORT` | Report buffer |
| `IOCTL_GET_MANUAL_MAPPED_REPORT` | out | `MANUAL_MAPPED_DRIVER_DETECTION_REPORT` | Report buffer |
| `IOCTL_GET_HASH_DIFF_REPORT` | out | `HASH_DIFF_REPORT` | Report buffer |
| `IOCTL_GET_FLAGGED_DLL_REPORT` | out | `FLAGGED_DLL_REPORT` | Report buffer |
| `IOCTL_GET_THREAD_REPORT` | out | `THREAD_DETECTION_REPORT` | Report buffer |

Every handler checks that the output buffer is at least as large as the struct before writing (`STATUS_BUFFER_TOO_SMALL` otherwise) and that the system buffer is non-null whenever a length is claimed (`STATUS_INVALID_PARAMETER`). Report IOCTLs go through `copy_report`, which also clamps the entry count to the 256 slots of the array.

## 5. Callbacks

Callbacks make the driver reactive: instead of polling, it reacts to the event when it happens.

### 5.1. Process creation

`PsSetCreateProcessNotifyRoutineEx`. This is how the driver learns that the game opened or closed, without the client sending a PID.

- If the image name equals `protected_game_process_name` (set with `IOCTL_SET_GAME_NAME`), the callback stores the PID and flips `game.is_game_open`.
- On exit it clears them, and resets `first_hash_catched`.
- On any process exit it also closes the session if that process was the client (`clean_session_on_close`).

`PsGetProcessImageFileName` truncates the name to 15 characters, so the configured name must fit that limit.

### 5.2. Thread creation

`PsSetCreateThreadNotifyRoutineEx(PsCreateThreadNotifyNonSystem, ...)`. System threads are filtered at registration time, since they are high frequency and never interesting here.

For every thread created in the protected process the callback records:

| Field | How |
|---|---|
| `start_address` | `ZwQueryInformationThread(ThreadQuerySetWin32StartAddress)` |
| `is_created_remotely` | `PsGetCurrentProcessId() != process_id` (the creator is not the game) |
| `thread_id` | the new thread |
| `creator_id` | `PsGetCurrentProcessId()` |

A thread created in the game by another process is the classic result of `CreateRemoteThread`-style injection.

### 5.3. Image load

`PsSetLoadImageNotifyRoutineEx` (the `Ex` variant guarantees `FullImageName`). The callback splits by who is loading:

```
process_id == NULL or 4   -> a kernel driver is being loaded -> BYOVD check   (8.1)
process_id == game pid    -> a module is loaded in the game  -> unsigned DLL  (8.4)
anything else             -> ignored
```

### 5.4. Object callbacks

`ObRegisterCallbacks` with altitude `321000`, on `PsProcessType`, `PsThreadType` and `IoDeviceObjectType`, for handle creation and duplication. Kernel handles are ignored so legitimate drivers and system services are not affected.

- **Handles to the game.** If the caller is not the game itself, dangerous rights are stripped from the granted access: `PROCESS_VM_READ`, `PROCESS_VM_WRITE`, `PROCESS_VM_OPERATION` on the process, and terminate / suspend / get-set context / set information / impersonate on its threads. The handle is still created, but it cannot read, write or hijack the game.
- **Handles to a vulnerable driver.** Device objects stored in the BYOVD report have their access mask set to `0`, so a cheat client cannot use the vulnerable driver's device.

## 6. Environment checks

### 6.1. Multiple sources per check

Anything a cheat wants to hide is asked in more than one way. Each way is a **source** and returns a `SOURCE_RESULT`:

```cpp
struct SOURCE_RESULT {
    BOOLEAN valid;   // the source could be queried
    BOOLEAN value;   // what the source answered
};
```

`valid` exists because "the source said no" and "the source could not be queried" are different facts. A failing source can be a signal by itself, so the driver never collapses it into `false`. The driver does **not** compare the sources to each other. It reports each answer as it got it, and the server cross-checks them (see [server.md](server.md)), because a spoofer who controls the machine also controls any comparison done on it.

Every check struct has a `static_assert` on its size, since the layout is shared byte for byte with the client.

### 6.2. Checks list

| Check | Struct | Sources |
|---|---|---|
| Secure Boot | `SECURE_BOOT_CHECK` | `nt_query_enabled` and `nt_query_capable` (`SystemSecureBootInformation`), `uefi_var` (`SecureBoot` firmware variable), `registry` (`...\Control\SecureBoot\State\UEFISecureBootEnabled`) |
| HVCI | `HVCI_CHECK` | `code_integrity` (`SystemCodeIntegrityInformation`, KMCI flag `0x400`) |
| VBS | `VBS_CHECK` | `nt_query_secure_kernel`, `nt_query_hvci` (`SystemIsolatedUserModeInformation`) |
| Hypervisor | `HYPERVISOR_CHECK` | `nt_query` (`SystemHypervisorInformation`), `cpuid` (leaf 1, ECX bit 31) |
| Kernel debugging | `KERNEL_DEBUGGING_CHECK` | `nt_query_enabled`, `nt_query_not_present` (`SystemKernelDebuggerInformation`) |
| Test signing | `TEST_SIGNING_CHECK` | `code_integrity` (`CODEINTEGRITY_OPTION_TESTSIGN`) |
| TPM | `TPM_CHECK` | `tpm_2_0`, `tpm_1_2`: the driver opens `\Device\TPM` and sends a `GetRandom` command in each version's wire format, valid when the response code is zero |

The TPM is probed by actually talking to it instead of reading a registry key, because a registry value is trivial to forge.

## 7. Identity

```cpp
typedef struct _HWID {
    UCHAR smbios_uuid[16];   // SMBIOS type 1 structure, offset 8
    ULONG cpu_hash;          // XOR of CPUID leaves 0, 1 and 4
    ULONG disk_serial;       // hash of the serial of \Device\Harddisk0\DR0
} HWID;
```

The three parts are sent separately instead of being merged into a single hash. That way the server can see *which* part changed between sessions, and a machine where two of three identifiers changed is far more interesting than one that is simply new. The SMBIOS table is read with `SystemFirmwareTableInformation`, and the disk serial with `IOCTL_STORAGE_QUERY_PROPERTY`.

## 8. Detections

### 8.1. BYOVD

*Bring your own vulnerable driver*: a cheat loads a legitimately signed driver that has an arbitrary read/write primitive, and uses it to reach the kernel.

The driver carries a table of known vulnerable drivers (`blocked_drivers.hpp`, generated from [loldrivers.io](https://www.loldrivers.io) by `fill_blocked_drivers_list.py`). Each entry holds the **SHA-256** and the **authentihash** of a known sample. The authentihash is the hash Windows uses for signatures, so it still matches when a sample has been re-signed or had its signature stripped.

There are two vectors:

1. **At start** (`detect_byovd_drivers_at_runtime`, run once by the scheduler): every loaded kernel module from `SystemModuleInformation` is hashed and compared. Method `METHOD_RUNTIME_SCAN`.
2. **On every driver load** (image callback, 5.3): the file being loaded is hashed and compared. Method `METHOD_IMAGE_CALLBACK`.

When a match is found the driver resolves the `DRIVER_OBJECT` by name, stores it together with its `DEVICE_OBJECT` in the report, and the object callback (5.4) starts stripping access to that device.

### 8.2. Manual mapped drivers

A manually mapped driver is code loaded into kernel memory without going through the loader, so it is in no module list. ringo does not look for any specific mapper. It looks for the traces every mapped driver has to leave.

#### 8.2.1. Big pool tracking

The mapper has to put the image somewhere, and an image is bigger than a page. `SystemBigPoolInformation` lists every big pool allocation, and one is reported when all of this is true:

```
non-paged
AND size > 0x1000
AND address is NOT inside a known loaded module
AND the address is mapped (is_region_executable)
AND the pool tag is NOT in the whitelist (whitelist_tags.hpp)
```

The whitelist exists because many legitimate components (NDIS, storage, graphics) make large allocations. A big pool alone is a weak signal, and the server weights it that way (see [server.md](server.md)).

#### 8.2.2. Kernel thread walking

Code that has been mapped still has to run, and it runs in threads. The driver enumerates the threads of the `System` process (PID 4) with `SystemProcessInformation` and applies two checks to each.

**Start address.** A system thread whose start address is outside every known module is running code that was not loaded by the OS.

**Return addresses on the stack.** A thread started from a legitimate driver may still call into mapped code, so the start address alone is not enough. For every thread that is waiting, the driver reads its kernel stack (limits from the `ETHREAD`) and inspects every qword. A value is reported when it:

```
is a kernel address
AND is not inside the stack itself
AND is not inside a known module
AND is executable (page table walk of CR3 through MmCopyMemory, honouring the NX bit and large pages)
AND looks like a return address (the bytes before it decode as a call instruction)
```

The last check filters stale pointers: a return address is, by definition, preceded by a `call`. It recognises the common encodings (`E8 rel32`, `FF /2` with register and memory operands, REX-prefixed variants).

Both checks are reported as `METHOD_KERNEL_SPACE_WALKING`.

### 8.3. Integrity

The `.text` section of the game is hashed in two ways and compared:

- **disk:** `.text` of the executable file, parsed from disk with the PE parser in the SDK.
- **process:** `.text` read from the live process (`MmCopyVirtualMemory` from the image base).

```
hash_compare():
  game open?                      if not, return
  hash disk, hash process         if either fails, return (never compare garbage)
  first time with the game open   -> take the process hash as the previous one
  process != previous process     -> report PROCESS_WITH_PROCESS
  process != disk                 -> report FILE_WITH_PROCESS
  previous = process
```

Only `.text` is compared. Other sections legitimately change after loading (`.data`, the import table), but code is read-only and has to match the file. Both buffers are cut to the same size (the smaller of `SizeOfRawData`, `VirtualSize` and what was actually read). The driver itself does not judge the result; it only reports which of the two comparisons failed.

The disk is used as the reference on purpose. Taking the first in-memory hash as the baseline would have a race against the game starting: a patch applied before the first hash would become the "original" state.

### 8.4. Unsigned DLLs in the game

When an image is loaded in the protected process (5.3), `ImageSignatureLevel` is checked. Anything above `SE_SIGNING_LEVEL_UNSIGNED` is ignored, and unsigned images are added to the flagged DLL report with their path, base and size.

### 8.5. Threads created in the game

See 5.2. The thread report is raw: every thread created in the game is recorded, and the server decides what is suspicious (for example, a remote thread whose start address is not in a known module).

## 9. Scheduler

Periodic detections need something to run them. The scheduler is a **single system thread** (`PsCreateSystemThread`); having one thread keeps the attack surface small and avoids races between detections.

```cpp
while (!scheduler_stop) {
    wait on g_stop_event with a timeout of 8 to 12 seconds (random)
    if (scheduler_stop) break;

    switch (random 0..2) {
        case 0: hash_compare();     break;
        case 1: thread_walking();   break;
        case 2: big_pool_detect();  break;
    }
}
PsTerminateSystemThread(STATUS_SUCCESS);
```

- **Unpredictable.** Both the sleep and the detection chosen are random, so there is no pattern to time a cheat against. With three detections each one runs on average every ~30 seconds, without a fixed order.
- **A sleep that can be cancelled.** The sleep is a `KeWaitForSingleObject` on `g_stop_event` with a timeout instead of a plain delay. On unload `KeSetEvent` wakes the thread immediately, so the unload does not wait for the interval to end.
- **One-time work.** `detect_byovd_drivers_at_runtime()` runs once before the loop.
- **Randomness.** `RtlRandomEx` seeded from `KeQueryInterruptTime() ^ __rdtsc()`. Predictable intervals are not a security boundary here, so cryptographic randomness is not needed.

Callbacks are *not* part of the scheduler. They are reactive and fire on their own.

## 10. Reporting

Detections never talk to user mode directly. They write into five fixed size buffers held in the context:

```cpp
struct XXX_REPORT {
    LONG  count;
    XXX_ENTRY entries[256];
};
```

| Report | Filled by |
|---|---|
| `BYOVD_DETECTION_REPORT` | BYOVD scan, image callback |
| `MANUAL_MAPPED_DRIVER_DETECTION_REPORT` | big pool tracking, thread walking |
| `HASH_DIFF_REPORT` | `hash_compare` |
| `FLAGGED_DLL_REPORT` | image callback |
| `THREAD_DETECTION_REPORT` | thread callback |

A report function claims a slot with `InterlockedIncrement(&count) - 1`, so two callbacks running on different processors always write to different entries. If the index reaches 256 the entry is dropped; `count` keeps growing and is clamped when copied out.

Reports are append-only and are **not cleared** after being read. The client resends the accumulated content and the server is responsible for not counting the same finding twice.

## 11. Heartbeat

`IOCTL_GET_HEARTBEAT` increments a 64-bit counter and returns it. The idea is that the server sees a stream that must keep increasing, and noticing that the stream stopped is a signal in itself (the client was killed or the driver stopped answering). A repeated or decreasing value is always a replay.

The client does not forward the heartbeat to the server yet, and the counter is not authenticated, so on its own it only proves that *something* kept counting. Binding it to the driver is planned (12).

## 12. Known limitations and planned work

- **Authenticated channel.** The planned challenge-response (an HMAC of a random nonce, generated with `BCryptGenRandom`, required before any other IOCTL) is not implemented yet. Today the protection is the exclusive device, the SDDL, the hash checks on open and per request, and the ban list.
- **Heartbeat** is a bare counter, with no signature.
- **`IOCTL_UNLOAD_DRIVER`** is defined but has no handler, so it falls into the unknown code path (4.4).
- **`IOCTL_SET_FILE_HASH`** has no one-shot protection: a caller that already holds a session can set it again.
- **Executable check for pools.** `is_region_executable` only verifies the address is mapped (`MmIsAddressValid`). The page table based `is_executable` exists and is used by thread walking.
- **Pointers in reports** (`suspicious_address`, `driver_object`, ...) are raw kernel addresses. They are only meaningful on the machine that produced them.
- **A BYOVD that is not in the list** cannot be detected.
- Planned: window overlays (`TOPMOST` / `CreateWindowInBand`), communication channels between drivers and the client, hooking `IRP_MJ_DEVICE_CONTROL` of newly loaded drivers (suspicious buffer lengths, physical memory access, diffing `PsLoadedModuleList`), NMI callbacks to inspect kernel thread stacks, and more red-pill techniques.

## 13. Building

See [build-windows.md](build-windows.md). The driver is built with MSVC and the WDK through `FindWDK.cmake`:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-all.ps1 -EnableTestSigning
```

The output is `build/bin/rngdrv.sys`. To regenerate the vulnerable driver table:

```
python driver/core/detection/byovd/fill_blocked_drivers_list.py
```

## 14. Acronyms

- **BYOVD** - bring your own vulnerable driver.
- **HVCI / KMCI** - hypervisor-protected code integrity / kernel-mode code integrity.
- **VBS** - virtualization-based security.
- **SMBIOS** - system management BIOS.
- **IRP** - I/O request packet.
- **IOCTL** - I/O control code.
- **SDDL** - security descriptor definition language.
- **PE** - portable executable.
- **NX** - no-execute (page table bit).
- **NDIS** - network driver interface specification.
- **TPM** - trusted platform module.
