#include "manual_mapped_drivers.hpp"
#include "core/reporting/report_manual_mapped_driver/report_manual_mapped_driver.hpp"


static BOOLEAN address_in_range(PVOID address, PVOID base, SIZE_T size) {
    ULONG_PTR a = (ULONG_PTR)address;
    ULONG_PTR b = (ULONG_PTR)base;

    return a >= b && a < b + size;
}


RULE_RESULT rule_manual_mapped(const MODULE_REPORT* r){
    const MANUAL_MAPPED_DRIVER_DETECTION_REPORT* rep = &r->manual_mapped;

    bool pool_present = false;
    bool thread_present = false;

    for(long i = 0; i < rep->count; i++){
        if(rep->entries[i].method == METHOD_BIG_POOL)
            pool_present = true;
        else if(rep->entries[i].method == METHOD_KERNEL_SPACE_WALKING)
            thread_present = true;
    }

    for (LONG i = 0; i < rep->count; i++) {
        const MANUAL_MAPPED_DRIVER_DETECTION_ENTRY* t = &rep->entries[i];
        if (t->method == METHOD_BIG_POOL) continue;

        for (LONG k = 0; k < rep->count; k++) {
            const MANUAL_MAPPED_DRIVER_DETECTION_ENTRY* p = &rep->entries[k];
            if (p->method != METHOD_BIG_POOL) continue;

            if (address_in_range(t->suspicious_address, p->suspicious_address, p->size))
                return found("thread_inside_big_pool", "kernel thread running inside a big pool");
        }
    }

    if (thread_present)
        return found("kernel_thread_outside_valid_module", "kernel thread outside any signed driver");

    if (pool_present)
        return found("big_pool_detected", "non-whitelisted big pool allocation");

    return no_finding();
}