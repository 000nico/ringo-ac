#include "core/detection/callbacks/ob_callbacks/obcallback.hpp"
#include "core/detection/callbacks/image_notify/image_notify.hpp"
#include "core/detection/callbacks/create_thread_notify/create_thread_notify.hpp"
#include "core/detection/callbacks/create_process_notify/create_process_notify.hpp"
#include "core/global.hpp"
#include "core/comunication/ioctl/ioctl.hpp"

void DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);

    if (ringo_context.Callbacks.ObCallbackHandle != NULL)
        ObUnRegisterCallbacks(ringo_context.Callbacks.ObCallbackHandle);

    if (ringo_context.Callbacks.isLoadImageCallbackRegistered == TRUE)
        PsRemoveLoadImageNotifyRoutine(ringo_context.Callbacks.LoadImageCallbackPtr);

    if (ringo_context.Callbacks.isCreateThreadCallbackRegistered == TRUE)
        PsRemoveCreateThreadNotifyRoutine(ringo_context.Callbacks.CreateThreadCallbackPtr);

    if (ringo_context.Callbacks.isCreateProcessCallbackRegistered == TRUE)
        PsSetCreateProcessNotifyRoutineEx(ringo_context.Callbacks.CreateProcessNotifyRoutinePtr, TRUE);

    CleanupDevice();

    KdPrint(("Ringo Driver Unloaded\n"));
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);
    KdPrint(("Ringo Driver Loaded\n"));

    // load callbacks
    ringo_context.Callbacks.ObCallbackHandle = ObRegisterCallbacksOpenProcess();
    if (ringo_context.Callbacks.ObCallbackHandle == NULL) {
        KdPrint(("[-] ObCallbackHandle not loaded.\n"));
    }

    SetLoadImageCallback();
    if (ringo_context.Callbacks.isLoadImageCallbackRegistered == FALSE) {
        KdPrint(("[-] SetLoadImage Callback not loaded.\n"));
    }

    SetCreateThreadCallback();
    if (ringo_context.Callbacks.isCreateThreadCallbackRegistered == FALSE) {
        KdPrint(("[-] CreateThreadCallback not loaded.\n"));
    }

    SetCreateProcessCallback();
    if (ringo_context.Callbacks.isCreateProcessCallbackRegistered == FALSE) {
        KdPrint(("[-] CreateProcessCallback not loaded.\n"));
    }

    if (!NT_SUCCESS(SetupDevice(DriverObject))) {
        KdPrint(("[-] IOCTL setup not loaded\n"));
    }

    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}