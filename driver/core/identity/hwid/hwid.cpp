#include "hwid.hpp"
#include "sdk/sdk.hpp"
#include <ntddstor.h>
#include <intrin.h>

NTSTATUS get_smbios_uuid(UCHAR out_uuid[16]) {
    ULONG size = sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info =
        (PSYSTEM_FIRMWARE_TABLE_INFORMATION)ExAllocatePool2(POOL_FLAG_NON_PAGED, size, 'SMBI');
    if (!info) return STATUS_INSUFFICIENT_RESOURCES;

    info->ProviderSignature = 'RSMB';
    info->Action = SystemFirmwareTable_Get;
    info->TableID = 0;
    info->TableBufferLength = 0;

    ULONG return_length = 0;
    NTSTATUS status = ZwQuerySystemInformation(SystemFirmwareTableInformation, info, size, &return_length);

    ExFreePoolWithTag(info, 'SMBI');
    size = return_length;
    info = (PSYSTEM_FIRMWARE_TABLE_INFORMATION)ExAllocatePool2(POOL_FLAG_NON_PAGED, size, 'SMBI');
    if (!info) return STATUS_INSUFFICIENT_RESOURCES;

    info->ProviderSignature = 'RSMB';
    info->Action = SystemFirmwareTable_Get;
    info->TableID = 0;
    info->TableBufferLength = size;

    status = ZwQuerySystemInformation(SystemFirmwareTableInformation, info, size, &return_length);

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
            RtlCopyMemory(out_uuid, ptr + 8, 16);
            ExFreePoolWithTag(info, 'SMBI');
            return STATUS_SUCCESS;
        }

        PUCHAR str_section = ptr + length;
        while (str_section + 1 < end && !(str_section[0] == 0 && str_section[1] == 0))
            str_section++;
        ptr = str_section + 2;
    }

    ExFreePoolWithTag(info, 'SMBI');
    return STATUS_NOT_FOUND;
}

ULONG get_cpu_hash() {
    int cpu_info[4] = { 0 };

    __cpuid(cpu_info, 0);
    ULONG hash = (ULONG)cpu_info[0] ^ (ULONG)cpu_info[1] ^ (ULONG)cpu_info[2] ^ (ULONG)cpu_info[3];

    __cpuid(cpu_info, 1);
    hash ^= (ULONG)cpu_info[0];
    hash ^= (ULONG)cpu_info[3];

    __cpuidex(cpu_info, 4, 0);
    hash ^= (ULONG)cpu_info[0];

    return hash;
}

NTSTATUS get_disk_serial(ULONG* out_serial) {
    UNICODE_STRING disk_path = RTL_CONSTANT_STRING(L"\\Device\\Harddisk0\\DR0");

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa, &disk_path, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    IO_STATUS_BLOCK iosb;
    HANDLE h_disk;
    NTSTATUS status = ZwOpenFile(&h_disk, GENERIC_READ | SYNCHRONIZE, &oa, &iosb,
        FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(status)) return status;

    ULONG buf_size = sizeof(STORAGE_DEVICE_DESCRIPTOR) + 256;
    PSTORAGE_DEVICE_DESCRIPTOR desc = (PSTORAGE_DEVICE_DESCRIPTOR)
        ExAllocatePool2(POOL_FLAG_NON_PAGED, buf_size, 'DISK');
    if (!desc) { ZwClose(h_disk); return STATUS_INSUFFICIENT_RESOURCES; }

    STORAGE_PROPERTY_QUERY query;
    RtlZeroMemory(&query, sizeof(query));
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    status = ZwDeviceIoControlFile(h_disk, NULL, NULL, NULL, &iosb,
        IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), desc, buf_size);

    if (NT_SUCCESS(status) && desc->SerialNumberOffset != 0) {
        PCHAR serial = (PCHAR)desc + desc->SerialNumberOffset;
        ULONG hash = 0;
        for (ULONG i = 0; serial[i] != 0; i++)
            hash = hash * 31 + (UCHAR)serial[i];
        *out_serial = hash;
    }

    ExFreePoolWithTag(desc, 'DISK');
    ZwClose(h_disk);
    return status;
}

HWID build_hwid() {
    HWID hwid;
    RtlZeroMemory(&hwid, sizeof(hwid));

    hwid.cpu_hash = get_cpu_hash();

    UCHAR uuid[16];
    if (NT_SUCCESS(get_smbios_uuid(uuid)))
        RtlCopyMemory(hwid.smbios_uuid, uuid, 16);

    hwid.disk_serial = NT_SUCCESS(get_disk_serial(&hwid.disk_serial)) ? hwid.disk_serial : 0;

    return hwid;
}