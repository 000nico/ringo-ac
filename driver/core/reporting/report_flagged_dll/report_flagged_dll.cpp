#include "report_flagged_dll.hpp"
#include "core/global.hpp"

VOID report_flagged_dll(PUNICODE_STRING full_image_name, BOOLEAN is_signed, PVOID image_base, SIZE_T image_size) {
    LONG index = InterlockedIncrement(&g_ringo_context.detections.dll_detection_report.count) - 1;
    if (index >= 256) return;

    LOADED_MODULE* entry = &g_ringo_context.detections.dll_detection_report.entries[index];
    RtlZeroMemory(entry, sizeof(LOADED_MODULE));

    RtlInitEmptyUnicodeString(&entry->full_image_name, entry->name_buffer, sizeof(entry->name_buffer));
    RtlCopyUnicodeString(&entry->full_image_name, full_image_name);

    entry->mem.image.image_base = image_base;
    entry->mem.image.image_size = image_size;
    entry->is_signed = is_signed;
}
