# System Architecture and IOCTL Interface

This document covers the low-level architectural design of Ringo Anti-Cheat, the kernel-mode driver lifecycle, global runtime structures, and the IOCTL communication protocol between User Mode (UM) and Kernel Mode (KM).

---

## 1. Driver Lifecycle Management

The kernel driver (`km.sys`) is implemented as a standard WDM driver targeting Windows 10/11 x64.

### 1.1 DriverEntry Execution Flow
When the service is loaded into the kernel address space:
1. **Object Callbacks Registration**: Calls `ObRegisterCallbacksOpenProcess()` to hook process and thread handle operations.
2. **System Notify Routines Setup**:
   - `SetLoadImageCallback()` registers `LoadImageCallback` via `PsSetLoadImageNotifyRoutineEx`.
   - `SetCreateThreadCallback()` registers `CreateThreadCallback` via `PsSetCreateThreadNotifyRoutineEx`.
   - `SetCreateProcessCallback()` registers `PcreateProcessNotifyRoutineEx` via `PsSetCreateProcessNotifyRoutineEx`.
3. **Communication Endpoint Creation**: Calls `SetupDevice(DriverObject)`:
   - Creates the device object `\Device\ringo`.
   - Creates the Win32 symbolic link `\DosDevices\ringo` (`\\.\ringo` for UM).
   - Configures `DO_BUFFERED_IO` on `g_DeviceObject`.
   - Maps `IRP_MJ_CREATE`, `IRP_MJ_CLOSE`, and `IRP_MJ_DEVICE_CONTROL`.
4. Sets the `DriverObject->DriverUnload` routine pointer.

### 1.2 DriverUnload Teardown Flow
When the service is stopped:
1. **Unregister Object Callbacks**: Calls `ObUnRegisterCallbacks(ringo_context.Callbacks.ObCallbackHandle)`.
2. **Remove System Notify Routines**:
   - `PsRemoveLoadImageNotifyRoutine`
   - `PsRemoveCreateThreadNotifyRoutine`
   - `PsSetCreateProcessNotifyRoutineEx(..., TRUE)`
3. **Device Cleanup**: Calls `CleanupDevice()` to delete the symbolic link and the device object.

---

## 2. Runtime Context and Memory Model

The driver state is maintained in a centralized global structure: `RINGO_CONTEXT ringo_context`.

```cpp
typedef struct RINGO_CONTEXT {
    HANDLE ProtectedGamePid;             // PID of the active protected game process
    UCHAR ProtectedGameProcessName[64];  // Target executable image name (e.g. "cs2.exe")
    BOOLEAN isGameOpen;                  // Boolean flag indicating if target process is alive

    struct {
        PVOID ObCallbackHandle;
        BOOLEAN isLoadImageCallbackRegistered;
        PLOAD_IMAGE_NOTIFY_ROUTINE LoadImageCallbackPtr;
        BOOLEAN isCreateThreadCallbackRegistered;
        PCREATE_THREAD_NOTIFY_ROUTINE CreateThreadCallbackPtr;
        BOOLEAN isCreateProcessCallbackRegistered;
        PCREATE_PROCESS_NOTIFY_ROUTINE_EX CreateProcessNotifyRoutinePtr;
    } Callbacks;

    struct {
        DRIVER_DETECTION_REPORT DriverDetectionReport;
        FLAGGED_DLL_REPORT DllDetectionReport;
    } Detections;

} RINGO_CONTEXT;
```

### Global Buffers
- `g_LoadedDlls[256]` / `g_LoadedDllsCount`: Lockless circular log storing loaded module structures indexed with `InterlockedIncrement`.
- `g_CreatedThreads[256]` / `g_CreatedThreadsCount`: Circular log of thread creation records indexed with `InterlockedIncrement`.

---

## 3. IOCTL Communication Protocol

All communication uses `METHOD_BUFFERED` via `DeviceIoControl`. Buffer definitions are synchronized in `km/core/comunication/ioctl/codes.hpp`.

### 3.1 Control Codes Specification

| Control Code | Hex Value | Type | Output / Input Payload | Description |
| :--- | :--- | :--- | :--- | :--- |
| `IOCTL_GET_HWID` | `0x8001` | GET | `HWID` (24 bytes) | Returns hardware ID (SMBIOS UUID, CPU Hash, Disk Serial). |
| `IOCTL_GET_GAME_PID` | `0x8002` | GET | `HANDLE` (8 bytes) | Returns current target game process ID. |
| `IOCTL_GET_GAME_STATUS` | `0x8003` | GET | `BOOLEAN` (1 byte) | Returns `TRUE` if protected game is active. |
| `IOCTL_GET_HVCI_INFORMATION` | `0x8004` | GET | `HVCI_CHECK` (1 byte) | Returns Hypervisor-Protected Code Integrity status. |
| `IOCTL_GET_HYPERVISOR_STATUS`| `0x8005` | GET | `HYPERVISOR_CHECK` (2 bytes)| Returns Hypervisor presence and tampering flag. |
| `IOCTL_GET_KERNEL_DEBUGGING_STATUS`| `0x8006`| GET | `KERNEL_DEBUGGING_CHECK` (1 byte)| Returns Kernel Debugger enabled/attached status. |
| `IOCTL_GET_SECURE_BOOT_INFORMATION`| `0x8007`| GET | `SECURE_BOOT_CHECK` (3 bytes)| Returns Secure Boot state and consistency check. |
| `IOCTL_GET_TEST_SIGNING_MODE` | `0x8008` | GET | `TEST_SIGNING_CHECK` (1 byte)| Returns Windows Test-Signing mode state. |
| `IOCTL_GET_VBS_INFORMATION` | `0x8009` | GET | `VBS_CHECK` (2 bytes) | Returns Virtualization-Based Security / Secure Kernel state. |
| `IOCTL_GET_TPM_INFORMATION` | `0x8010` | GET | `TPM` (4 bytes) | Returns detected TPM version enum (`None`, `1.2`, `2.0`). |
| `IOCTL_IS_CREATE_PROCESS_CALLBACK_SET` | `0x8011` | GET | `BOOLEAN` (1 byte) | Verification that process callback is registered. |
| `IOCTL_IS_CREATE_THREAD_CALLBACK_SET` | `0x8012` | GET | `BOOLEAN` (1 byte) | Verification that thread callback is registered. |
| `IOCTL_IS_IMAGE_CALLBACK_SET` | `0x8013` | GET | `BOOLEAN` (1 byte) | Verification that image load callback is registered. |
| `IOCTL_IS_OBCALLBACK_SET` | `0x8014` | GET | `BOOLEAN` (1 byte) | Verification that `ObRegisterCallbacks` is registered. |
| `IOCTL_SET_GAME_NAME` | `0x8015` | SET | `char[]` (string) | Configures target executable name (e.g. `cs2.exe`). |

---

## 4. User-Mode Client Architecture

The user-mode binary (`um.exe`) operates as a consumer:
1. Calls `open_driver(&hDriver)` to acquire a handle via `CreateFile("\\\\.\\ringo", ...)`.
2. Invokes `setGameProcessName(hDriver, "cs2.exe")` to supply the target process name to the driver.
3. Issues `GetEnvironmentInformation(...)` queries to display a complete security posture and hardware telemetry report.
