#include "../../../../sdk/imports/imports.hpp"
#include "bpt.hpp"
#include "../driver_scan.hpp"
#include "../../detections.hpp"
#include "../../../global.hpp"
#include "whitelistTags.hpp"

BOOLEAN isTagInWhiteList(ULONG tag) {
    for (int i = 0; i < ARRAYSIZE(KnownLegitPoolTags); i++) {
        if (KnownLegitPoolTags[i] == tag)
            return TRUE;
    }
    return FALSE;
}
PVOID AllocateAndQueryBigPool(ULONG* outReturnLength) {
    ULONG returnLength = 0;
    PVOID buffer = NULL;

    for (int attempt = 0; attempt < 5; attempt++) {
        ZwQuerySystemInformation(SystemBigPoolInformation, NULL, 0, &returnLength);
        if (returnLength == 0) return NULL;

        returnLength += sizeof(SYSTEM_BIGPOOL_ENTRY) * 16;

        buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, returnLength, 'tGLD');
        if (!buffer) return NULL;

        NTSTATUS status = ZwQuerySystemInformation(
            SystemBigPoolInformation, buffer, returnLength, &returnLength
        );

        if (NT_SUCCESS(status)) {
            *outReturnLength = returnLength;
            return buffer;
        }

        ExFreePoolWithTag(buffer, 'tGLD');
        buffer = NULL;

        if (status != STATUS_INFO_LENGTH_MISMATCH)
            return NULL;
    }

    return NULL;
}

void BigPool_Detect() {
    ULONG returnLength = 0;
    LOADED_LEGIT_DRIVERS* lld = GetLegitDriverList();
    if (!lld)
        return;

    PVOID buffer = AllocateAndQueryBigPool(&returnLength);
    if (!buffer) {
        FreeLegitDriverList(lld);
        return;
    }

    SYSTEM_BIGPOOL_INFORMATION* sbi = (SYSTEM_BIGPOOL_INFORMATION*)buffer;

    for (ULONG i = 0; i < sbi->Count; i++) {
        PVOID realVirtualAddress = (PVOID)((ULONG_PTR)sbi->AllocatedInfo[i].VirtualAddress & ~1);

        if (sbi->AllocatedInfo[i].NonPaged != 1) continue;
        if (sbi->AllocatedInfo[i].SizeInBytes <= 0x1000) continue;
        if (isAddressInKnownModule(realVirtualAddress, lld)) continue;
        if (!isRegionExecutable(realVirtualAddress)) continue;
        if (isTagInWhiteList(sbi->AllocatedInfo[i].TagUlong)) continue;

        if (ringo_context.Detections.DriverDetectionReport.Count < 256) {
            ringo_context.Detections.DriverDetectionReport.Entries[ringo_context.Detections.DriverDetectionReport.Count++] = CreateDriverDetectionEntry(realVirtualAddress, sbi->AllocatedInfo[i].SizeInBytes, METHOD_BIG_POOL, NULL);
        }
    }

    ExFreePoolWithTag(buffer, 'tGLD');
    FreeLegitDriverList(lld);
}