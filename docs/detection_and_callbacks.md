# 🛡️ Detection Engine & Callbacks

This document details the callback hooks, object filtering routines, and kernel memory scanners implemented in Ringo Anti-Cheat.

---

## 1. Object Callbacks (`ObRegisterCallbacks`)

📁 **Source:** [`km/core/detection/callbacks/ob_callbacks/obcallback.cpp`](file:///C:/dev/ringo-ac/km/core/detection/callbacks/ob_callbacks/obcallback.cpp)

### 1.1 Purpose
Prevents unauthorized user-mode processes from obtaining handle permissions that allow reading/writing process memory, injecting threads, or modifying thread execution contexts in the protected game.

### 1.2 Registration Details
- **Altitude**: `"321000"` (FSFilter Activity Monitor range).
- **Operation Types**: `OB_OPERATION_HANDLE_CREATE` and `OB_OPERATION_HANDLE_DUPLICATE`.
- **Target Types**: `*PsProcessType` and `*PsThreadType`.

### 1.3 Filtering Logic
```
                           [Handle Creation Request]
                                      |
                           Is KernelHandle == 1?  ---> [Allow Unmodified]
                                      | No
                           Is Game Open & PID != 0?
                                      | Yes
                         Target PID == ProtectedGamePid?
                                      | Yes
                         Caller PID != ProtectedGamePid?
                                      | Yes
               +----------------------+----------------------+
               |                                             |
     [PsProcessType Object]                         [PsThreadType Object]
               |                                             |
   Strip Process Permissions:                     Strip Thread Permissions:
   - PROCESS_VM_READ    (0x0010)                  - THREAD_TERMINATE            (0x0001)
   - PROCESS_VM_WRITE   (0x0020)                  - THREAD_SUSPEND_RESUME       (0x0002)
   - PROCESS_VM_OPERATION (0x0008)                - THREAD_GET_CONTEXT          (0x0008)
                                                  - THREAD_SET_CONTEXT          (0x0010)
                                                  - THREAD_SET_INFORMATION      (0x0020)
                                                  - THREAD_DIRECT_IMPERSONATION (0x0200)
```

---

## 2. Process Lifecycle Hook (`PsSetCreateProcessNotifyRoutineEx`)

📁 **Source:** [`km/core/detection/callbacks/create_process_notify/create_process_notify.cpp`](file:///C:/dev/ringo-ac/km/core/detection/callbacks/create_process_notify/create_process_notify.cpp)

- **Callback Signature**: `PCREATE_PROCESS_NOTIFY_ROUTINE_EX` (`PcreateProcessNotifyRoutineEx`).
- **Functionality**:
  - Extracts the short image filename using `PsGetProcessImageFileName(Process)`.
  - Performs case-insensitive matching (`_stricmp`) against `ringo_context.ProtectedGameProcessName`.
  - **On Process Spawn** (`CreateInfo != NULL`): Sets `ringo_context.ProtectedGamePid = ProcessId` and `ringo_context.isGameOpen = TRUE`.
  - **On Process Termination** (`CreateInfo == NULL`): If `ProcessId == ringo_context.ProtectedGamePid`, resets `ProtectedGamePid = NULL` and `isGameOpen = FALSE`.

---

## 3. Thread Creation Hook (`PsSetCreateThreadNotifyRoutineEx`)

📁 **Source:** [`km/core/detection/callbacks/create_thread_notify/create_thread_notify.cpp`](file:///C:/dev/ringo-ac/km/core/detection/callbacks/create_thread_notify/create_thread_notify.cpp)

- **Callback Signature**: `PCREATE_THREAD_NOTIFY_ROUTINE` with `PsCreateThreadNotifyNonSystem` flag.
- **Functionality**:
  - Filters thread creation events specifically for `ProcessId == ringo_context.ProtectedGamePid`.
  - **Remote Thread Detection**: Flags `isCreatedRemotely = (PsGetCurrentProcessId() != ProcessId)`.
  - **Win32 Start Address Resolution**: Calls `GetStartAddress(ThreadId)` which opens a temporary kernel handle with `ObOpenObjectByPointer` and invokes `ZwQueryInformationThread(..., ThreadQuerySetWin32StartAddress, ...)`.
  - Records the event into `g_CreatedThreads[index]`.

---

## 4. Image Load Monitor (`PsSetLoadImageNotifyRoutineEx`)

📁 **Source:** [`km/core/detection/callbacks/image_notify/image_notify.cpp`](file:///C:/dev/ringo-ac/km/core/detection/callbacks/image_notify/image_notify.cpp)

- **Callback Signature**: `PLOAD_IMAGE_NOTIFY_ROUTINE` (`LoadImageCallback`).
- **Functionality**:
  - Filters DLL/image mappings into the address space of `ringo_context.ProtectedGamePid`.
  - Records the `ImageBase`, `ImageSize`, and `FullImageName`.
  - Determines if the module is signed (`ImageInfo->ImageSignatureLevel > SE_SIGNING_LEVEL_UNSIGNED`).
  - Stores the entry into `g_LoadedDlls[index]`.

---

## 5. Kernel Driver Scanners (Manual Map Detection)

📁 **Source:** [`km/core/detection/manual_map_driver_scan/`](file:///C:/dev/ringo-ac/km/core/detection/manual_map_driver_scan/)

### 5.1 Legitimate Driver Enumeration (`driver_scan.cpp`)
- Queries `ZwQuerySystemInformation(SystemModuleInformation, ...)` (`0xB`).
- Allocates a dynamic baseline (`LOADED_LEGIT_DRIVERS`) in `NonPagedPoolNx` containing all legitimate kernel module virtual ranges (`ImageBase` to `ImageBase + ImageSize`).

### 5.2 Big Pool Allocation Tracker (`big_pool_track/bpt.cpp`)
- Queries `ZwQuerySystemInformation(SystemBigPoolInformation, ...)` (`0x42`).
- Iterates through all allocations where:
  - `NonPaged == 1`
  - `SizeInBytes > 0x1000` (greater than a 4 KB page)
  - The allocation's virtual address does **not** fall within any known legitimate module address range (`!isAddressInKnownModule(...)`).
- Creates a `DETECTION_ENTRY` with `METHOD_BIG_POOL`.

### 5.3 Kernel Space Walk (`kernel_space_walk/kwalk.cpp`)
- *Status:* **Placeholder / Unimplemented**. Planned for page table walking (PML4 traversal) to detect unbacked executable kernel pages.
