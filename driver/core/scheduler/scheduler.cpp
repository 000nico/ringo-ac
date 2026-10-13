#include "scheduler.hpp"
#include "core/detection/integrity/hash_sections/hash_sections.hpp"
#include "core/detection/manual_mapped_drivers/big_pool_track/big_pool_track.hpp"
#include "core/detection/manual_mapped_drivers/kernel_space_walking/kernel_space_walking.hpp"
#include "core/global.hpp"
#include <ntdef.h>
#include <ntifs.h>
#include <wdm.h>
#include "core/reporting/report_hash_diff/report_hash_diff.hpp"
#include "sdk/crypto/hashes.hpp"
#include "sdk/pe/pe.hpp"
#include "../detection/byovd/byovd_scan.hpp"
#include "tasks.hpp"
#include <intrin.h>

ULONG random_in_range(ULONG min, ULONG max) {
    ULONG r = RtlRandomEx(&g_ringo_context.Scheduler.random_seed);
    return min + (r % (max - min + 1));
}

VOID scheduler(PVOID context) {
    UNREFERENCED_PARAMETER(context);
    g_ringo_context.Scheduler.random_seed = (ULONG)KeQueryInterruptTime() ^ (ULONG)__rdtsc();

    // one-time-tasks
    detect_byovd_drivers_at_runtime();

    while (!g_ringo_context.Scheduler.scheduler_stop) {
        LARGE_INTEGER timeout;
        timeout.QuadPart = -(LONGLONG)random_in_range(8000, 12000) * 10000;
        KeWaitForSingleObject(&g_stop_event, Executive, KernelMode, FALSE, &timeout);
        if (g_ringo_context.Scheduler.scheduler_stop) break;

        switch (random_in_range(0, 2)) {
            case 0: hash_compare(); break;
            case 1: thread_walking();                break;
            case 2: big_pool_detect();               break;
        }
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}