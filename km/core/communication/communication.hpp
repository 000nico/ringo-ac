#pragma once
#include <ntifs.h>

NTSTATUS ioctl_handler(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS setup_device(PDRIVER_OBJECT DriverObject);
VOID cleanup_device();