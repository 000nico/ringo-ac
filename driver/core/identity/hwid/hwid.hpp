// HWID = SHA256(SMBIOS_UUID + CPUID + DiskPhysicalSerial)
#pragma once

#ifdef _KERNEL_MODE
#include<ntifs.h>
#else
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#endif

#define SystemFirmwareTableInformation 76

typedef struct _HWID {
    UCHAR smbios_uuid[16];   // SMBIOS type 1, offset 8
    ULONG cpu_hash;          // CPUID EAX=1 + EAX=0
    ULONG disk_serial;
} HWID;

HWID build_hwid();