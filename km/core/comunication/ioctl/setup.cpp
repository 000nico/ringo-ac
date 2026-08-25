// device.cpp
#include <ntifs.h>
#include "codes.hpp"
#include "ioctl.hpp"

#define DEVICE_NAME   L"\\Device\\ringo"
#define SYMBOLIC_LINK L"\\DosDevices\\ringo"

static PDEVICE_OBJECT g_DeviceObject = NULL;

static NTSTATUS IrpSuccess(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS IrpNotSupported(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS SetupDevice(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING deviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
    UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMBOLIC_LINK);

    NTSTATUS status = IoCreateDevice(
        DriverObject, 0, &deviceName,
        FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN,
        FALSE, &g_DeviceObject
    );
    if (!NT_SUCCESS(status)) return status;

    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
        return status;
    }

    g_DeviceObject->Flags |= DO_BUFFERED_IO;
    g_DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = IrpNotSupported;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = IrpSuccess;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IrpSuccess;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoctlHandler;

    return STATUS_SUCCESS;
}

VOID CleanupDevice() {
    UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMBOLIC_LINK);
    IoDeleteSymbolicLink(&symLink);
    if (g_DeviceObject) {
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
    }
}