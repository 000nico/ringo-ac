#include "core/detection/manual_mapped_drivers/driver_scan.hpp"
#include "ntifs.h"
#include "sdk/imports/imports.hpp"
#include "sdk/sdk.hpp"
#include "kernel_space_walking.hpp"
#include "core/reporting/report_manual_mapped_driver/report_manual_mapped_driver.hpp"

constexpr ULONG kSystemProcessInformation = 5;
constexpr ULONG kThreadWaiting = 5;
constexpr ULONG kProcTag = 'corP';

static int check_thread_return_address(PSYSTEM_THREAD_INFORMATION thread, LOADED_LEGIT_DRIVERS* lld);
static int check_thread_start_address(PSYSTEM_THREAD_INFORMATION thread, LOADED_LEGIT_DRIVERS* lld);

int thread_walking() {
    ULONG size = 0;
    ZwQuerySystemInformation(kSystemProcessInformation, nullptr, 0, &size);
    if (size == 0) return 0;

    PVOID buffer = nullptr;
    NTSTATUS status = STATUS_INFO_LENGTH_MISMATCH;

    for (int attempt = 0; attempt < 5 && status == STATUS_INFO_LENGTH_MISMATCH; attempt++) {
        size += 0x4000;
        buffer = ExAllocatePool2(POOL_FLAG_PAGED, size, kProcTag);
        if (!buffer) return 0;

        status = ZwQuerySystemInformation(kSystemProcessInformation, buffer, size, &size);
        if (!NT_SUCCESS(status)) {
            ExFreePoolWithTag(buffer, kProcTag);
            buffer = nullptr;
        }
    }
    if (!buffer) return 0;

    PSYSTEM_PROCESS_INFORMATION system = nullptr;
    for (auto cur = (PSYSTEM_PROCESS_INFORMATION)buffer; ; ) {
        if (cur->UniqueProcessId == (HANDLE)4) { system = cur; break; }
        if (cur->NextEntryOffset == 0) break;
        cur = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)cur + cur->NextEntryOffset);
    }

    if (system) {
        LOADED_LEGIT_DRIVERS* lld = get_legit_driver_list();
        if (lld) {
            for (ULONG i = 0; i < system->NumberOfThreads; i++) {
                check_thread_return_address(&system->Threads[i], lld);
                check_thread_start_address(&system->Threads[i], lld);
            }
            free_legit_driver_list(lld);
        }
    }

    ExFreePoolWithTag(buffer, kProcTag);
    return 1;
}

static int check_thread_return_address(PSYSTEM_THREAD_INFORMATION thread, LOADED_LEGIT_DRIVERS* lld) {
    if (thread->ThreadState != kThreadWaiting)
        return 0;

    PETHREAD pThread = nullptr;
    if (!NT_SUCCESS(PsLookupThreadByThreadId(thread->ClientId.UniqueThread, &pThread)) || !pThread)
        return 0;

    auto base = (PUCHAR)pThread;
    ULONG_PTR initial = 0, limit = 0, kstack = 0;
    __try {
        initial = *(ULONG_PTR*)(base + 0x28);
        limit   = *(ULONG_PTR*)(base + 0x30);
        kstack  = *(ULONG_PTR*)(base + 0x58);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ObDereferenceObject(pThread);
        return 0;
    }

    if (limit >= (ULONG_PTR)MmSystemRangeStart &&
        kstack >= limit && kstack < initial && (initial - limit) <= 0x10000)
    {
        for (ULONG_PTR p = kstack; p < initial; p += 8) {
            ULONG_PTR value = 0;
            __try {
                if (!MmIsAddressValid((PVOID)p)) break;
                value = *(ULONG_PTR*)p;
            } __except (EXCEPTION_EXECUTE_HANDLER) { break; }

            if (value < (ULONG_PTR)MmSystemRangeStart)      continue;
            if (value >= limit && value < initial)          continue;
            if (is_address_in_known_module((PVOID)value, lld)) continue;
            if (!is_executable(value))                      continue;
            if (!looks_like_return_address((PUCHAR)value))  continue;

            create_manual_mapped_driver_report_entry((PVOID)value, 0, METHOD_KERNEL_SPACE_WALKING, 0);
        }
    }

    ObDereferenceObject(pThread);
    return 1;
}

static int check_thread_start_address(PSYSTEM_THREAD_INFORMATION thread, LOADED_LEGIT_DRIVERS* lld) {
    if (!thread->StartAddress) return 0;

    if (!is_address_in_known_module(thread->StartAddress, lld)) {
        KdPrint(("thread start outside known module: %p\n", thread->StartAddress));
        create_manual_mapped_driver_report_entry(thread->StartAddress, 0, METHOD_KERNEL_SPACE_WALKING, 0);
        return 1;
    }
    return 0;
}
