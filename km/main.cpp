#include <ntddk.h>
#include<winnt.h>
#include "core/detection/usermode_cheats/callbacks/ob_callbacks/openprocess/openprocess.hpp"
#include "core/global.hpp"

RINGO_CONTEXT ringo_context;

void DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);

    if(ringo_context.ObCallbackHandle != NULL)
        ObUnRegisterCallbacks(ringo_context.ObCallbackHandle);

    KdPrint(("Ringo Driver Unloaded\n"));
    
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    KdPrint(("Ringo Driver Loaded\n"));

    // load callbacks
    ringo_context.ObCallbackHandle = ObRegisterCallbacksOpenProcess();

    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}