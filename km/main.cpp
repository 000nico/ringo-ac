#include "core/detection/usermode_cheats/callbacks/ob_callbacks/openprocess/openprocess.hpp"
#include "core/detection/usermode_cheats/callbacks/image_notify/image_notify.hpp"
#include "core/detection/usermode_cheats/callbacks/create_thread_notify/create_thread_notify.hpp"
#include "core/global.hpp"

RINGO_CONTEXT ringo_context = { 0 };

void DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);

    if (ringo_context.Callbacks.ObCallbackHandle != NULL)
        ObUnRegisterCallbacks(ringo_context.Callbacks.ObCallbackHandle);

    if (ringo_context.Callbacks.isLoadImageCallbackRegistered == TRUE)
        PsRemoveLoadImageNotifyRoutine(ringo_context.Callbacks.LoadImageCallbackPtr);

    if (ringo_context.Callbacks.isCreateThreadCallbackRegistered == TRUE)
        PsRemoveCreateThreadNotifyRoutine(ringo_context.Callbacks.CreateThreadCallbackPtr);

    if (ringo_context.Callbacks.isCreateProcessCallbackRegistered == TRUE)
        PsSetCreateProcessNotifyRoutineEx(ringo_context.Callbacks.CreateProcessNotifyRoutinePtr, TRUE); // i would really want to know why microsoft has a remove for every callback but not for this one

    KdPrint(("Ringo Driver Unloaded\n"));
    
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    KdPrint(("Ringo Driver Loaded\n"));

    // load callbacks
    ringo_context.Callbacks.ObCallbackHandle = ObRegisterCallbacksOpenProcess();
    if (ringo_context.Callbacks.ObCallbackHandle == NULL)
        KdPrint(("[-] ObCallbackHandle not loaded."));

    SetLoadImageCallback();
    if (ringo_context.Callbacks.isLoadImageCallbackRegistered == FALSE)
        KdPrint(("[-] SetLoadImage Callback not loaded."));

    SetCreateThreadCallback();
    if (ringo_context.Callbacks.isCreateThreadCallbackRegistered == FALSE)
        KdPrint(("[-] CreateThreadCallback not loaded."));

    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}