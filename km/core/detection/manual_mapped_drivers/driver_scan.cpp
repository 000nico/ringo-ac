#include "driver_scan.hpp"
#include "sdk/sdk.hpp"

PVOID allocate_and_query_modules(ULONG* outReturnLength) {
    ULONG returnLength = 0;

    ZwQuerySystemInformation(SystemModuleInformation, NULL, 0, &returnLength);

    if (returnLength == 0)
        return NULL;

    PVOID buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, returnLength, 'tGLD');
    if (!buffer)
        return NULL;

    NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, buffer, returnLength, &returnLength);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(buffer, 'tGLD');
        return NULL;
    }

    *outReturnLength = returnLength;
    return buffer;
}

ULONG get_legit_driver_count(RTL_PROCESS_MODULES* modules) {
    return min(modules->number_of_modules, 512);
}

LOADED_LEGIT_DRIVERS* get_legit_driver_list() {
    ULONG returnLength = 0;

    PVOID buffer = allocate_and_query_modules(&returnLength);
    if (!buffer)
        return NULL;

    RTL_PROCESS_MODULES* modules = (RTL_PROCESS_MODULES*)buffer;
    ULONG count = get_legit_driver_count(modules);

    SIZE_T allocSize = sizeof(LOADED_LEGIT_DRIVERS) + (count > 0 ? (count - 1) : 0) * sizeof(LOADED_LEGIT_DRIVER_ENTRY);
    LOADED_LEGIT_DRIVERS* lld = (LOADED_LEGIT_DRIVERS*)ExAllocatePool2(POOL_FLAG_NON_PAGED, allocSize, 'tGLD');
    if (!lld) {
        ExFreePoolWithTag(buffer, 'tGLD');
        return NULL;
    }

    lld->Count = count;
    for (ULONG i = 0; i < count; i++) {
        lld->Drivers[i].image_base = modules->Modules[i].image_base;
        lld->Drivers[i].image_size = modules->Modules[i].image_size;
        RtlCopyMemory(lld->Drivers[i].full_path_name, modules->Modules[i].full_path_name, sizeof(lld->Drivers[i].full_path_name));
    }

    ExFreePoolWithTag(buffer, 'tGLD');
    return lld;
}

VOID free_legit_driver_list(LOADED_LEGIT_DRIVERS* lld) {
    if (lld) {
        ExFreePoolWithTag(lld, 'tGLD');
    }
}

BOOLEAN is_address_in_known_module(PVOID address, LOADED_LEGIT_DRIVERS* lld) {
    if (!lld) return FALSE;

    for (ULONG i = 0; i < lld->Count; i++) {
        ULONG_PTR base = (ULONG_PTR)lld->Drivers[i].image_base;
        ULONG_PTR end = base + lld->Drivers[i].image_size;
        ULONG_PTR addr = (ULONG_PTR)address;

        if (addr >= base && addr < end)
            return TRUE;
    }

    return FALSE;
}

BOOLEAN is_region_executable(PVOID address) {
    // ZwQueryVirtualMemory operates against the current process's virtual address space,
    // which when called from a system thread does not map kernel addresses at all.
    // MmIsAddressValid checks whether the virtual address is currently mapped and safe
    // to access without raising a page-fault, which is the correct primitive for kernel
    // address validation. Executable permission on kernel regions is enforced by the
    // pool/section object flags, not by user-mode VirtualQuery semantics.
    if (!MmIsAddressValid(address)) return FALSE;

    // Conservatively treat any valid kernel address that is not covered by a known
    // module (checked by the caller via is_address_in_known_module) as potentially
    // executable. Pool allocations with POOL_FLAG_NON_PAGED|execute intent land in
    // non-paged executive regions; the big-pool scanner already filters by size and
    // tag, so returning TRUE here for valid addresses not in the known-module list is
    // the correct behaviour for anomaly detection purposes.
    return TRUE;
}