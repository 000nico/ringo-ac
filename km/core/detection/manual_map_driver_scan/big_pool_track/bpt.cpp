#include "../../../../sdk/imports/imports.hpp"
#include "bpt.hpp"
#include "../driver_scan.hpp"
#include "../../detections.hpp"
#include "../../../global.hpp"

PVOID AllocateAndQueryBigPool(ULONG* outReturnLength) {
    ULONG returnLength = 0;

    ZwQuerySystemInformation(SystemBigPoolInformation, NULL, 0, &returnLength);

    if (returnLength == 0)
        return NULL;

    PVOID buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, returnLength, 'tGLD');
    if (!buffer)
        return NULL;

    NTSTATUS status = ZwQuerySystemInformation(SystemBigPoolInformation, buffer, returnLength, &returnLength);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(buffer, 'tGLD');
        return NULL;
    }

    *outReturnLength = returnLength;
    return buffer;
}

void BigPool_Detect() {
    ULONG returnLength = 0;
    LOADED_LEGIT_DRIVERS lld = GetLegitDriverList();

    PVOID buffer = AllocateAndQueryBigPool(&returnLength);
    if (!buffer)
        return;

    SYSTEM_BIGPOOL_INFORMATION* sbi = (SYSTEM_BIGPOOL_INFORMATION*)buffer;

    for (ULONG i = 0; i < sbi->Count; i++) {
        PVOID realVirtualAddress = (PVOID)((ULONG_PTR)sbi->AllocatedInfo[i].VirtualAddress & ~1);

        if (sbi->AllocatedInfo[i].NonPaged == 1 && sbi->AllocatedInfo[i].SizeInBytes > 0x1000 && !isAddressInKnownModule(realVirtualAddress, &lld)) 
            ringo_context.Detections.DriverDetectionReport.Entries[ringo_context.Detections.DriverDetectionReport.Count++] = CreateDriverDetectionEntry(realVirtualAddress, sbi->AllocatedInfo[i].SizeInBytes, METHOD_BIG_POOL, NULL);
    }

    ExFreePoolWithTag(buffer, 'tGLD');
}