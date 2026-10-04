#include "hwid.hpp"
#include "sdk/sdk.hpp"
#include <ntddstor.h>
#include <intrin.h>

NTSTATUS get_smbios_uuid(UCHAR OutUuid[16]) {
    ULONG size = sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info =
        (PSYSTEM_FIRMWARE_TABLE_INFORMATION)ExAllocatePool2(POOL_FLAG_NON_PAGED, size, 'SMBI');
    if (!info) return STATUS_INSUFFICIENT_RESOURCES;

    info->ProviderSignature = 'RSMB';
    info->Action = SystemFirmwareTable_Get;
    info->TableID = 0;
    info->TableBufferLength = 0;

    ULONG returnLength = 0;
    NTSTATUS status = ZwQuerySystemInformation(SystemFirmwareTableInformation, info, size, &returnLength);

    ExFreePoolWithTag(info, 'SMBI');
    size = returnLength;
    info = (PSYSTEM_FIRMWARE_TABLE_INFORMATION)ExAllocatePool2(POOL_FLAG_NON_PAGED, size, 'SMBI');
    if (!info) return STATUS_INSUFFICIENT_RESOURCES;

    info->ProviderSignature = 'RSMB';
    info->Action = SystemFirmwareTable_Get;
    info->TableID = 0;
    info->TableBufferLength = size;

    status = ZwQuerySystemInformation(SystemFirmwareTableInformation, info, size, &returnLength);

    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(info, 'SMBI');
        return status;
    }

    PUCHAR smbios = info->TableBuffer + 8;
    PUCHAR ptr = smbios;
    PUCHAR end = smbios + (info->TableBufferLength - 8);

    while (ptr + 4 <= end) {
        UCHAR type = ptr[0];
        UCHAR length = ptr[1];

        if (type == 127 || length < 4)
            break;

        if (type == 1 && length >= 0x19 && (ptr + 8 + 16 <= end)) {
            RtlCopyMemory(OutUuid, ptr + 8, 16);
            ExFreePoolWithTag(info, 'SMBI');
            return STATUS_SUCCESS;
        }

        PUCHAR strSection = ptr + length;
        while (strSection + 1 < end && !(strSection[0] == 0 && strSection[1] == 0))
            strSection++;
        ptr = strSection + 2;
    }

    ExFreePoolWithTag(info, 'SMBI');
    return STATUS_NOT_FOUND;
}

ULONG get_cpu_hash() {
    int cpuInfo[4] = { 0 };

    __cpuid(cpuInfo, 0);
    ULONG hash = (ULONG)cpuInfo[0] ^ (ULONG)cpuInfo[1] ^ (ULONG)cpuInfo[2] ^ (ULONG)cpuInfo[3];

    __cpuid(cpuInfo, 1);
    hash ^= (ULONG)cpuInfo[0];
    hash ^= (ULONG)cpuInfo[3];

    __cpuidex(cpuInfo, 4, 0);
    hash ^= (ULONG)cpuInfo[0];

    return hash;
}

NTSTATUS get_disk_serial(ULONG* OutSerial) {
    UNICODE_STRING diskPath = RTL_CONSTANT_STRING(L"\\Device\\Harddisk0\\DR0");

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa, &diskPath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    IO_STATUS_BLOCK iosb;
    HANDLE hDisk;
    NTSTATUS status = ZwOpenFile(&hDisk, GENERIC_READ | SYNCHRONIZE, &oa, &iosb,
        FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(status)) return status;

    ULONG bufSize = sizeof(STORAGE_DEVICE_DESCRIPTOR) + 256;
    PSTORAGE_DEVICE_DESCRIPTOR desc = (PSTORAGE_DEVICE_DESCRIPTOR)
        ExAllocatePool2(POOL_FLAG_NON_PAGED, bufSize, 'DISK');
    if (!desc) { ZwClose(hDisk); return STATUS_INSUFFICIENT_RESOURCES; }

    STORAGE_PROPERTY_QUERY query;
    RtlZeroMemory(&query, sizeof(query));
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    status = ZwDeviceIoControlFile(hDisk, NULL, NULL, NULL, &iosb,
        IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), desc, bufSize);

    if (NT_SUCCESS(status) && desc->SerialNumberOffset != 0) {
        PCHAR serial = (PCHAR)desc + desc->SerialNumberOffset;
        ULONG hash = 0;
        for (ULONG i = 0; serial[i] != 0; i++)
            hash = hash * 31 + (UCHAR)serial[i];
        *OutSerial = hash;
    }

    ExFreePoolWithTag(desc, 'DISK');
    ZwClose(hDisk);
    return status;
}

HWID build_hwid() {
    HWID hwid;
    RtlZeroMemory(&hwid, sizeof(hwid));

    hwid.CpuHash = get_cpu_hash();

    UCHAR UUID[16];
    if (NT_SUCCESS(get_smbios_uuid(UUID)))
        RtlCopyMemory(hwid.SmbiosUuid, UUID, 16);

    hwid.DiskSerial = NT_SUCCESS(get_disk_serial(&hwid.DiskSerial)) ? hwid.DiskSerial : 0;

    return hwid;
}