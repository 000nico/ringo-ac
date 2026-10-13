#include "sdk/sdk.hpp"
#include "big_pool_track.hpp"
#include "../driver_scan.hpp"
#include "../../../reporting/report_manual_mapped_driver/report_manual_mapped_driver.hpp"
#include "../../../global.hpp"
#include "whitelist_tags.hpp"

BOOLEAN is_tag_in_white_list(ULONG tag) {
    for (int i = 0; i < ARRAYSIZE(KNOWN_LEGIT_POOL_TAGS); i++) {
        if (KNOWN_LEGIT_POOL_TAGS[i] == tag)
            return TRUE;
    }
    return FALSE;
}
PVOID allocate_and_query_big_pool(ULONG* out_return_length) {
    ULONG return_length = 0;
    PVOID buffer = NULL;

    for (int attempt = 0; attempt < 5; attempt++) {
        ZwQuerySystemInformation(SystemBigPoolInformation, NULL, 0, &return_length);
        if (return_length == 0) return NULL;

        return_length += sizeof(SYSTEM_BIGPOOL_ENTRY) * 16;

        buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, return_length, 'tGLD');
        if (!buffer) return NULL;

        NTSTATUS status = ZwQuerySystemInformation(
            SystemBigPoolInformation, buffer, return_length, &return_length
        );

        if (NT_SUCCESS(status)) {
            *out_return_length = return_length;
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
    ULONG return_length = 0;
    LOADED_LEGIT_DRIVERS* lld = get_legit_driver_list();
    if (!lld)
        return;

    PVOID buffer = allocate_and_query_big_pool(&return_length);
    if (!buffer) {
        free_legit_driver_list(lld);
        return;
    }

    SYSTEM_BIGPOOL_INFORMATION* sbi = (SYSTEM_BIGPOOL_INFORMATION*)buffer;

    for (ULONG i = 0; i < sbi->Count; i++) {
        PVOID real_virtual_address = (PVOID)((ULONG_PTR)sbi->allocated_info[i].virtual_address & ~1);

        if (sbi->allocated_info[i].non_paged != 1) continue;
        if (sbi->allocated_info[i].size_in_bytes <= 0x1000) continue;
        if (is_address_in_known_module(real_virtual_address, lld)) continue;
        if (!is_region_executable(real_virtual_address)) continue;
        if (is_tag_in_white_list(sbi->allocated_info[i].tag_ulong)) continue;

        report_manual_mapped_driver(real_virtual_address, sbi->allocated_info[i].size_in_bytes, METHOD_BIG_POOL, NULL);
    }

    ExFreePoolWithTag(buffer, 'tGLD');
    free_legit_driver_list(lld);
}