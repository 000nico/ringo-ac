#include <ntddk.h>

void DriverUnload(PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
    KdPrint(("Ringo Driver Unloaded\n"));
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    KdPrint(("Ringo Driver Loaded\n"));

    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}