#include "core/detection/manual_mapped_drivers/driver_scan.hpp"
#include "ntdef.h"
#include "sdk/imports/imports.hpp"
#include "sdk/sdk.hpp"
#include "wdm.h"
#include "kernel_space_walking.hpp"
#include "core/reporting/report_manual_mapped_driver/report_manual_mapped_driver.hpp"

#define SystemProcessInformation 5

// this functions detects threads with a start address outside of an actual legit driver
// its a good technique and has almost zero false-positives, but its trivial to bypass
int detect_unknown_threads() {
    SYSTEM_PROCESS_INFORMATION spi = {};
    ULONG buffer_size = 0;

    // first call to get buffer size
    NTSTATUS status = ZwQuerySystemInformation(SystemProcessInformation, NULL, 0, &buffer_size);

    // second call, the actual call
    PVOID buffer = ExAllocatePoolWithTag(PagedPool, buffer_size, 'corP');
    if(!buffer) {
        return 0;
    }

    status = ZwQuerySystemInformation(SystemProcessInformation, buffer, buffer_size, &buffer_size);
    if(!NT_SUCCESS(status)) {
        ExFreePoolWithTag(buffer, 'corP');
        return 0;
    }

    PSYSTEM_PROCESS_INFORMATION current = (PSYSTEM_PROCESS_INFORMATION)buffer;
    PSYSTEM_PROCESS_INFORMATION system = 0;
    unsigned int count = 0;
    while(true) {
        if(current->UniqueProcessId == (HANDLE)4){
            system = current;
            KdPrint(("Found system process"));
            break;
        }
        
        if (current->NextEntryOffset == 0) {
            break;
        }

        current = (PSYSTEM_PROCESS_INFORMATION)((PUCHAR)current + current->NextEntryOffset);
        count++;
    }

    // analyze its threads
    if(system) {
        PSYSTEM_THREAD_INFORMATION threads = (PSYSTEM_THREAD_INFORMATION)(system + 1);
        LOADED_LEGIT_DRIVERS* lld = get_legit_driver_list();

        for(ULONG i = 0; i < system->NumberOfThreads; i++) {
            if(!is_address_in_known_module(threads[i].StartAddress, lld)){
                KdPrint(("thread base outside a known module: %p", threads[i].StartAddress));
                create_manual_mapped_driver_report_entry(threads[i].StartAddress, 0, METHOD_KERNEL_SPACE_WALKING, 0);
            }
        }
    }

    ExFreePoolWithTag(buffer, 'corP');
    return 1;
}
