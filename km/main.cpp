#include <ntddk.h>
#include<winnt.h>
#include "core/detection/usermode_cheats/callbacks/ob_callbacks/openprocess/openprocess.hpp"
#include "core/detection/usermode_cheats/callbacks/image_notify/image_notify.hpp"
#include "core/global.hpp"

RINGO_CONTEXT ringo_context = { 0 };

void DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);

    if(ringo_context.Callbacks.ObCallbackHandle != NULL)
        ObUnRegisterCallbacks(ringo_context.Callbacks.ObCallbackHandle);

    if (ringo_context.Callbacks.isLoadImageCallbackRegistered = TRUE)
        PsRemoveLoadImageNotifyRoutine(ringo_context.Callbacks.LoadImageCallbackPtr);

    KdPrint(("Ringo Driver Unloaded\n"));
    
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    KdPrint(("Ringo Driver Loaded\n"));

    // load callbacks
    ringo_context.Callbacks.ObCallbackHandle = ObRegisterCallbacksOpenProcess();
    if (ringo_context.Callbacks.ObCallbackHandle == NULL)
        KdPrint(("[-] ObCallbackHandle = NULL"));

    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}