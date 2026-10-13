#include "report_manual_mapped_driver.hpp"
#include "core/global.hpp"

VOID report_manual_mapped_driver(PVOID suspicious_address, SIZE_T driver_size, DETECTION_METHOD method, UCHAR* module_path) {
    LONG index = InterlockedIncrement(&g_ringo_context.detections.driver_detection_report.count) - 1;
    if (index >= 256) return;

    MANUAL_MAPPED_DRIVER_DETECTION_ENTRY* entry = &g_ringo_context.detections.driver_detection_report.entries[index];
    RtlZeroMemory(entry, sizeof(MANUAL_MAPPED_DRIVER_DETECTION_ENTRY));

    entry->suspicious_address = suspicious_address;
    entry->size = driver_size;
    entry->method = method;

    if (module_path)
        RtlCopyMemory(entry->module_path, module_path, sizeof(entry->module_path));
}
