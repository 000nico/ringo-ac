#include "report_thread_detection.hpp"
#include "core/global.hpp"

VOID report_thread_detection(PVOID start_address, BOOLEAN is_created_remotely, HANDLE thread_id, HANDLE creator_id) {
    LONG index = InterlockedIncrement(&g_ringo_context.detections.thread_detection_report.count) - 1;
    if (index >= 256) return;

    CREATED_THREAD* entry = &g_ringo_context.detections.thread_detection_report.entries[index];
    entry->start_address = start_address;
    entry->is_created_remotely = is_created_remotely;
    entry->thread_id = thread_id;
    entry->creator_id = creator_id;
}
