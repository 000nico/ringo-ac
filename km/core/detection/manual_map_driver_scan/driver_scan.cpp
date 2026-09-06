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

LOADED_LEGIT_DRIVERS GetLegitDriverList() {
    LOADED_LEGIT_DRIVERS lld = {};
    ULONG returnLength = 0;

    PVOID buffer = AllocateAndQueryModules(&returnLength);
    if (!buffer)
        return lld;

    RTL_PROCESS_MODULES* modules = (RTL_PROCESS_MODULES*)buffer;

    lld.Count = GetLegitDriverCount(modules);

    for (ULONG i = 0; i < lld.Count; i++) {
        lld.Drivers[i].ImageBase = modules->Modules[i].ImageBase;
        lld.Drivers[i].ImageSize = modules->Modules[i].ImageSize;
        RtlCopyMemory(lld.Drivers[i].FullPathName, modules->Modules[i].FullPathName, sizeof(lld.Drivers[i].FullPathName));
    }

    ExFreePoolWithTag(buffer, 'tGLD');
    return lld;
}

BOOLEAN isAddressInKnownModule(PVOID address, LOADED_LEGIT_DRIVERS* lld) {
    for (ULONG i = 0; i < lld->Count; i++) {
        ULONG_PTR base = (ULONG_PTR)lld->Drivers[i].ImageBase;
        ULONG_PTR end = base + lld->Drivers[i].ImageSize;
        ULONG_PTR addr = (ULONG_PTR)address;

        if (addr >= base && addr < end)
            return TRUE;
    }

    return FALSE;
}