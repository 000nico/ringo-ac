// device.cpp
#include <ntifs.h>
#include <wdmsec.h>
#include "communication.hpp"
#include "core/global.hpp"
#include "ntddk.h"
#include "ntstatus.h"
#include "protection.hpp"
#include "sdk/crypto/hashes.hpp"
#include "wdm.h"

#define DEVICE_NAME   L"\\Device\\ringo"
#define SYMBOLIC_LINK L"\\DosDevices\\ringo"

static PDEVICE_OBJECT g_device_object = NULL;

static void close_session()
{
    if (g_ringo_context.session.active && g_ringo_context.session.process) {
        ObDereferenceObject(g_ringo_context.session.process);
    }
    g_ringo_context.session.active = FALSE;
    g_ringo_context.session.process = NULL;
    g_ringo_context.session.process_start_key = 0;
}

static NTSTATUS irp_create(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Information = 0;

    PEPROCESS requestor = IoGetRequestorProcess(Irp);

    UCHAR file_sha256[HASH_SIZE_SHA256] = {};
    UCHAR process_sha256[HASH_SIZE_SHA256] = {};

    bool got_file_hash = get_caller_file_hash(requestor, file_sha256);
    bool got_process_hash = get_caller_process_hash(requestor, process_sha256);
    if(got_process_hash)
        RtlCopyMemory(g_ringo_context.session.last_hash, process_sha256, HASH_SIZE_SHA256);
    
    NTSTATUS status;

    if (got_file_hash && got_process_hash && compare_sha256(file_sha256, g_expected_file_hash) && !is_peprocess_banned(requestor)) {
        ObReferenceObject(requestor);

        g_ringo_context.session.active = TRUE;
        g_ringo_context.session.process = requestor;
        g_ringo_context.session.process_start_key = PsGetProcessStartKey(requestor);
        status = STATUS_SUCCESS;
    } else {
        close_session();
        status = STATUS_ACCESS_DENIED;
    }
    
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

static NTSTATUS irp_close(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    close_session();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS irp_not_supported(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS setup_device(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING device_name = RTL_CONSTANT_STRING(DEVICE_NAME);
    UNICODE_STRING sym_link = RTL_CONSTANT_STRING(SYMBOLIC_LINK);
    UNICODE_STRING sddl = RTL_CONSTANT_STRING(L"D:P(A;;GA;;;SY)");
    NTSTATUS status = IoCreateDeviceSecure(
        DriverObject, 0, &device_name,
        FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN,
        TRUE, &sddl, nullptr, &g_device_object
    );
    if (!NT_SUCCESS(status)) return status;

    status = IoCreateSymbolicLink(&sym_link, &device_name);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(g_device_object);
        g_device_object = NULL;
        return status;
    }

    g_device_object->Flags |= DO_BUFFERED_IO;
    g_device_object->Flags &= ~DO_DEVICE_INITIALIZING;

    for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = irp_not_supported;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = irp_create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = irp_close;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ioctl_handler;

    return STATUS_SUCCESS;
}

VOID cleanup_device() {
    UNICODE_STRING sym_link = RTL_CONSTANT_STRING(SYMBOLIC_LINK);
    IoDeleteSymbolicLink(&sym_link);
    if (g_device_object) {
        IoDeleteDevice(g_device_object);
        g_device_object = NULL;
    }
}