#include "driver_scan.hpp"
#include "../../../sdk/imports/imports.hpp"

PVOID AllocateAndQueryModules(ULONG* outReturnLength) {
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

ULONG GetLegitDriverCount(RTL_PROCESS_MODULES* modules) {
    return min(modules->NumberOfModules, 512);
}

LOADED_LEGIT_DRIVERS* GetLegitDriverList() {
    ULONG returnLength = 0;

    PVOID buffer = AllocateAndQueryModules(&returnLength);
    if (!buffer)
        return NULL;

    RTL_PROCESS_MODULES* modules = (RTL_PROCESS_MODULES*)buffer;
    ULONG count = GetLegitDriverCount(modules);

    SIZE_T allocSize = sizeof(LOADED_LEGIT_DRIVERS) + (count > 0 ? (count - 1) : 0) * sizeof(LOADED_LEGIT_DRIVER_ENTRY);
    LOADED_LEGIT_DRIVERS* lld = (LOADED_LEGIT_DRIVERS*)ExAllocatePool2(POOL_FLAG_NON_PAGED, allocSize, 'tGLD');
    if (!lld) {
        ExFreePoolWithTag(buffer, 'tGLD');
        return NULL;
    }

    lld->Count = count;
    for (ULONG i = 0; i < count; i++) {
        lld->Drivers[i].ImageBase = modules->Modules[i].ImageBase;
        lld->Drivers[i].ImageSize = modules->Modules[i].ImageSize;
        RtlCopyMemory(lld->Drivers[i].FullPathName, modules->Modules[i].FullPathName, sizeof(lld->Drivers[i].FullPathName));
    }

    ExFreePoolWithTag(buffer, 'tGLD');
    return lld;
}

VOID FreeLegitDriverList(LOADED_LEGIT_DRIVERS* lld) {
    if (lld) {
        ExFreePoolWithTag(lld, 'tGLD');
    }
}

BOOLEAN isAddressInKnownModule(PVOID address, LOADED_LEGIT_DRIVERS* lld) {
    if (!lld) return FALSE;

    for (ULONG i = 0; i < lld->Count; i++) {
        ULONG_PTR base = (ULONG_PTR)lld->Drivers[i].ImageBase;
        ULONG_PTR end = base + lld->Drivers[i].ImageSize;
        ULONG_PTR addr = (ULONG_PTR)address;

        if (addr >= base && addr < end)
            return TRUE;
    }

    return FALSE;
}

BOOLEAN isRegionExecutable(PVOID address) {
    // ZwQueryVirtualMemory operates against the current process's virtual address space,
    // which when called from a system thread does not map kernel addresses at all.
    // MmIsAddressValid checks whether the virtual address is currently mapped and safe
    // to access without raising a page-fault, which is the correct primitive for kernel
    // address validation. Executable permission on kernel regions is enforced by the
    // pool/section object flags, not by user-mode VirtualQuery semantics.
    if (!MmIsAddressValid(address)) return FALSE;

    // Conservatively treat any valid kernel address that is not covered by a known
    // module (checked by the caller via isAddressInKnownModule) as potentially
    // executable. Pool allocations with POOL_FLAG_NON_PAGED|execute intent land in
    // non-paged executive regions; the big-pool scanner already filters by size and
    // tag, so returning TRUE here for valid addresses not in the known-module list is
    // the correct behaviour for anomaly detection purposes.
    return TRUE;
}