#pragma once
#include <ntifs.h>

NTSTATUS IoctlHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS SetupDevice(PDRIVER_OBJECT DriverObject);
VOID CleanupDevice();