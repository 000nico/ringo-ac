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
    MEMORY_BASIC_INFORMATION mbi = {};
    SIZE_T returnLenght = 0;

    NTSTATUS status = ZwQueryVirtualMemory(ZwCurrentProcess(), address, MemoryBasicInformation, &mbi, sizeof(mbi), &returnLenght);

    if (!NT_SUCCESS(status)) return FALSE;

    else {
        return(mbi.Protect & PAGE_EXECUTE) ||
              (mbi.Protect & PAGE_EXECUTE_READ) ||
              (mbi.Protect & PAGE_EXECUTE_READWRITE) ||
              (mbi.Protect & PAGE_EXECUTE_WRITECOPY);
    }
}