#include "sdk/sdk.hpp"
#include "big_pool_track.hpp"
#include "../driver_scan.hpp"
#include "../../../reporting/report_manual_mapped_driver/report_manual_mapped_driver.hpp"
#include "../../../global.hpp"
#include "whitelist_tags.hpp"

BOOLEAN is_tag_in_white_list(ULONG tag) {
    for (int i = 0; i < ARRAYSIZE(KnownLegitPoolTags); i++) {
        if (KnownLegitPoolTags[i] == tag)
            return TRUE;
    }
    return FALSE;
}
PVOID allocate_and_query_big_pool(ULONG* outReturnLength) {
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

void big_pool_detect() {
    ULONG returnLength = 0;
    LOADED_LEGIT_DRIVERS* lld = get_legit_driver_list();
    if (!lld)
        return;

    PVOID buffer = allocate_and_query_big_pool(&returnLength);
    if (!buffer) {
        free_legit_driver_list(lld);
        return;
    }

    SYSTEM_BIGPOOL_INFORMATION* sbi = (SYSTEM_BIGPOOL_INFORMATION*)buffer;

    for (ULONG i = 0; i < sbi->Count; i++) {
        PVOID realVirtualAddress = (PVOID)((ULONG_PTR)sbi->allocated_info[i].virtual_address & ~1);

        if (sbi->allocated_info[i].non_paged != 1) continue;
        if (sbi->allocated_info[i].size_in_bytes <= 0x1000) continue;
        if (is_address_in_known_module(realVirtualAddress, lld)) continue;
        if (!is_region_executable(realVirtualAddress)) continue;
        if (is_tag_in_white_list(sbi->allocated_info[i].tag_ulong)) continue;

        if (g_ringo_context.Detections.driver_detection_report.Count < 256) {
            g_ringo_context.Detections.driver_detection_report.Entries[g_ringo_context.Detections.driver_detection_report.Count++] = create_manual_mapped_driver_report_entry(realVirtualAddress, sbi->allocated_info[i].size_in_bytes, METHOD_BIG_POOL, NULL);
        }
    }

    ExFreePoolWithTag(buffer, 'tGLD');
    free_legit_driver_list(lld);
}