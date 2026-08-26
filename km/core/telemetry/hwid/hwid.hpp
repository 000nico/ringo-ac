// HWID = SHA256(SMBIOS_UUID + CPUID + DiskPhysicalSerial)
#pragma once
#include<ntifs.h>

#define SystemFirmwareTableInformation 76

typedef struct _HWID {
    UCHAR SmbiosUuid[16];    // SMBIOS type 1, offset 8
    ULONG CpuHash;           // CPUID EAX=1 + EAX=0
    ULONG DiskSerial;
} HWID;

HWID buildHwid();