#include "report_byovd.hpp"
#include "core/global.hpp"

VOID report_byovd_detection(PUNICODE_STRING full_image_name, PDRIVER_OBJECT driver_object, PDEVICE_OBJECT device_object, BYOVD_DETECTION_METHOD method) {
	LONG index = InterlockedIncrement(&g_ringo_context.detections.byovd_detection_report.count) - 1;
	if (index >= 256) return;

	BYOVD_DETECTION_ENTRY* entry = &g_ringo_context.detections.byovd_detection_report.entries[index];
	RtlZeroMemory(entry, sizeof(BYOVD_DETECTION_ENTRY));

	RtlInitEmptyUnicodeString(&entry->full_image_name, entry->name_buffer, sizeof(entry->name_buffer));
	RtlCopyUnicodeString(&entry->full_image_name, full_image_name);

	entry->method = method;
	entry->driver_object = driver_object;
	entry->device_object = device_object;
}
