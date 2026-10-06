#include "driver_scan.hpp"
#include <intrin.h>

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

constexpr ULONG64 PTE_PRESENT = 1ull << 0;
constexpr ULONG64 PTE_LARGE   = 1ull << 7;
constexpr ULONG64 PTE_NX      = 1ull << 63;
constexpr ULONG64 PTE_ADDR    = 0x000FFFFFFFFFF000ull;

bool read_phys_qword(ULONG64 pa, ULONG64& out)
{
    MM_COPY_ADDRESS src{};
    src.PhysicalAddress.QuadPart = (LONGLONG)pa;
    SIZE_T copied = 0;
    return NT_SUCCESS(MmCopyMemory(&out, src, sizeof(out), MM_COPY_MEMORY_PHYSICAL, &copied)) && copied == sizeof(out);
}

bool is_executable(ULONG_PTR va)
{
    ULONG64 table = __readcr3() & PTE_ADDR;
    ULONG64 idx[4] = {
        (va >> 39) & 0x1FF,   // PML4
        (va >> 30) & 0x1FF,   // PDPT
        (va >> 21) & 0x1FF,   // PD
        (va >> 12) & 0x1FF    // PT
    };

    for (int level = 0; level < 4; level++) {
        ULONG64 entry = 0;
        if (!read_phys_qword(table + idx[level] * 8, entry)) return false;
        if (!(entry & PTE_PRESENT)) return false;
        if (entry & PTE_NX)         return false;  

        if ((level == 1 || level == 2) && (entry & PTE_LARGE))
            return true;

        table = entry & PTE_ADDR;
    }
    return true;
}

static bool looks_like_return_address(PUCHAR addr)
{
    // necesitamos poder leer hasta 7 bytes antes de ra
    if (!MmIsAddressValid(addr - 7) || !MmIsAddressValid(addr - 1))
        return false;

    __try {
        // call rel32:            E8 xx xx xx xx          (5 bytes)
        if (addr[-5] == 0xE8) return true;

        // call [rip+rel32]:      FF 15 xx xx xx xx       (6 bytes) 
        if (addr[-6] == 0xFF && addr[-5] == 0x15) return true;

        // call reg:              FF D0..D7               (2 bytes)
        // call r8-r15:           41 FF D0..D7            (3 bytes)
        if (addr[-2] == 0xFF && (addr[-1] & 0xF8) == 0xD0) return true;
        if (addr[-3] == 0x41 && addr[-2] == 0xFF && (addr[-1] & 0xF8) == 0xD0) return true;

        // call [reg+disp8]:      FF 50..57 dd            (3 bytes), con REX: 41 FF 50+ dd (4)
        if (addr[-3] == 0xFF && (addr[-2] & 0xF8) == 0x50) return true;
        if (addr[-4] == 0x41 && addr[-3] == 0xFF && (addr[-2] & 0xF8) == 0x50) return true;

        // call [reg+disp32]:     FF 90..97 dd dd dd dd   (6 bytes)
        if (addr[-6] == 0xFF && (addr[-5] & 0xF8) == 0x90) return true;

        // call [reg] sin disp:   FF 10..17               (2 bytes)
        if (addr[-2] == 0xFF && (addr[-1] & 0xF8) == 0x10 &&
            (addr[-1] & 7) != 4 && (addr[-1] & 7) != 5) return true;

        // call [reg+reg*s+disp]: FF 14 SIB / FF 54 SIB dd (3-4 bytes)
        if (addr[-3] == 0xFF && addr[-2] == 0x14) return true;
        if (addr[-4] == 0xFF && addr[-3] == 0x54) return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {}

    return false;
}