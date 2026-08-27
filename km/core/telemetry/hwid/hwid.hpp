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
    UCHAR SmbiosUuid[16];    // SMBIOS type 1, offset 8
    ULONG CpuHash;           // CPUID EAX=1 + EAX=0
    ULONG DiskSerial;
} HWID;

HWID buildHwid();