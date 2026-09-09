# Architecture

This document describes the internal layout of the Ringo kernel driver: how it initializes, how its runtime state is organized, and how the IOCTL protocol between the kernel driver and the user-mode service works.

---

## Driver Lifecycle

The kernel driver (`km.sys`) is a standard WDM driver targeting Windows 10/11 x64. Its entry point is `DriverEntry` in `km/main.cpp`.

On load, `DriverEntry` runs the following sequence in order:

1. **Object callbacks** — `ObRegisterCallbacksOpenProcess()` registers pre-operation handlers on `PsProcessType`, `PsThreadType`, and `IoDeviceObjectType`. The returned opaque handle is stored in `ringo_context.Callbacks.ObCallbackHandle` for later deregistration.
2. **Image-load callback** — `PsSetLoadImageNotifyRoutineEx(LoadImageCallback, 0)` registers the module-load monitor. The `Ex` variant is used specifically because it guarantees `FullImageName` is populated and supports the flag that suppresses shared-section remaps.
3. **Thread-creation callback** — `PsSetCreateThreadNotifyRoutineEx` with `PsCreateThreadNotifyNonSystem` filters out kernel system threads at registration, avoiding high-frequency noise that would never match detection conditions.
4. **Process-creation callback** — `PsSetCreateProcessNotifyRoutineEx` tracks the target game process by image name, making the kernel the authoritative source of the protected PID.
5. **Device setup** — `SetupDevice(DriverObject)` creates `\Device\ringo`, the symbolic link `\DosDevices\ringo` (accessible as `\\.\ringo`), configures `DO_BUFFERED_IO`, and maps `IRP_MJ_CREATE`, `IRP_MJ_CLOSE`, and `IRP_MJ_DEVICE_CONTROL` dispatch routines.

If any step fails, the driver logs the failure but continues loading. This is intentional: a missing callback reduces coverage but should not prevent the rest of the system from functioning.

On unload, `DriverUnload` reverses the process: it deregisters each callback only if the corresponding registration flag is set, then calls `CleanupDevice()` to delete the symbolic link and device object.

---

## Runtime Context

All mutable driver state lives in a single zero-initialized global `RINGO_CONTEXT ringo_context`, declared in `km/core/global.hpp`:

```cpp
typedef struct RINGO_CONTEXT {
    struct {
        HANDLE  ProtectedGamePid;               // PID of the active game process
        UCHAR   ProtectedGameProcessName[64];   // Image name set via IOCTL (e.g. "cs2.exe")
        BOOLEAN isGameOpen;                     // TRUE while the target process is alive
    } Game;

    struct {
        PVOID                          ObCallbackHandle;
        BOOLEAN                        isLoadImageCallbackRegistered;
        PLOAD_IMAGE_NOTIFY_ROUTINE     LoadImageCallbackPtr;
        BOOLEAN                        isCreateThreadCallbackRegistered;
        PCREATE_THREAD_NOTIFY_ROUTINE  CreateThreadCallbackPtr;
        BOOLEAN                        isCreateProcessCallbackRegistered;
        PCREATE_PROCESS_NOTIFY_ROUTINE_EX CreateProcessNotifyRoutinePtr;
    } Callbacks;

    struct {
        DRIVER_DETECTION_REPORT  DriverDetectionReport;
        BYOVD_DETECTION_REPORT   ByovdDetectionReport;
        FLAGGED_DLL_REPORT       DllDetectionReport;
        THREAD_DETECTION_REPORT  ThreadDetectionReport;
    } Detections;

} RINGO_CONTEXT;
```

Detection report arrays are fixed-size and write-once from the kernel side. The user-mode service reads them through IOCTL but cannot modify them. All concurrent index increments use `InterlockedIncrement` so that simultaneous callbacks on different processors always write to distinct slots.

---

## IOCTL Protocol

All communication uses `METHOD_BUFFERED` via `DeviceIoControl`. In this mode the I/O manager copies the user-mode input buffer into `Irp->AssociatedIrp.SystemBuffer` before invoking the driver, and copies that same buffer back to the user-mode output buffer on completion. The driver never touches user-mode memory directly.

Control code definitions live in `km/core/comunication/ioctl/codes.hpp`, which is `#include`'d by both the kernel and user-mode projects.

| Control Code | Direction | Payload | Description |
|---|---|---|---|
| `IOCTL_GET_HWID` | GET | `HWID` (24 bytes) | SMBIOS UUID, CPU hash, disk serial |
| `IOCTL_GET_GAME_PID` | GET | `HANDLE` (8 bytes) | Currently tracked game PID |
| `IOCTL_GET_GAME_STATUS` | GET | `BOOLEAN` (1 byte) | Whether the game process is alive |
| `IOCTL_GET_HVCI_INFORMATION` | GET | `HVCI_CHECK` (1 byte) | HVCI / KMCI status |
| `IOCTL_GET_HYPERVISOR_STATUS` | GET | `HYPERVISOR_CHECK` (2 bytes) | Hypervisor presence and spoof flag |
| `IOCTL_GET_KERNEL_DEBUGGING_STATUS` | GET | `KERNEL_DEBUGGING_CHECK` (1 byte) | Kernel debugger attached |
| `IOCTL_GET_SECURE_BOOT_INFORMATION` | GET | `SECURE_BOOT_CHECK` (3 bytes) | Secure Boot state and consistency |
| `IOCTL_GET_TEST_SIGNING_MODE` | GET | `TEST_SIGNING_CHECK` (1 byte) | Test-signing mode enabled |
| `IOCTL_GET_VBS_INFORMATION` | GET | `VBS_CHECK` (2 bytes) | VBS / Secure Kernel state |
| `IOCTL_GET_TPM_INFORMATION` | GET | `TPM` (4 bytes) | TPM version (`None`, `1.2`, `2.0`) |
| `IOCTL_IS_CREATE_PROCESS_CALLBACK_SET` | GET | `BOOLEAN` (1 byte) | Process callback registered |
| `IOCTL_IS_CREATE_THREAD_CALLBACK_SET` | GET | `BOOLEAN` (1 byte) | Thread callback registered |
| `IOCTL_IS_IMAGE_CALLBACK_SET` | GET | `BOOLEAN` (1 byte) | Image-load callback registered |
| `IOCTL_IS_OBCALLBACK_SET` | GET | `BOOLEAN` (1 byte) | Object callback registered |
| `IOCTL_SET_GAME_NAME` | SET | `char[]` | Configures target executable name |

The dispatcher validates that the system buffer pointer is non-null whenever a nonzero length is claimed, and checks that output buffers are at least as large as the expected response struct before writing. Undersized buffers return `STATUS_BUFFER_TOO_SMALL`; unknown control codes return `STATUS_INVALID_DEVICE_REQUEST`.

---

## User-Mode Client

`um/main.cpp` represents the minimal consumer of the IOCTL interface. It opens `\\.\ringo` with `CreateFile`, calls `IOCTL_SET_GAME_NAME` to push the target process name, and then exercises each getter in sequence. The shared type headers from the kernel project are included directly — both solutions reference the same struct definitions without duplication.

In a complete deployment, the user-mode layer would forward the collected data to a backend service, handle session authentication, and act on the driver's detection reports. None of that backend logic is part of this repository.
