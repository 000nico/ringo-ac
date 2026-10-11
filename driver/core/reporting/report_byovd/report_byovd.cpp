#include "report_byovd.hpp"
#include "../../global.hpp"

VOID add_byovd_report_entry(PUNICODE_STRING FullImageName, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, BYOVD_DETECTION_METHOD Method) {
	LONG index = InterlockedIncrement(&g_ringo_context.Detections.byovd_detection_report.count) - 1;
	if (index >= 256) return;

	BYOVD_DETECTION_ENTRY* entry = &g_ringo_context.Detections.byovd_detection_report.Entries[index];
	RtlZeroMemory(entry, sizeof(BYOVD_DETECTION_ENTRY));

	RtlInitEmptyUnicodeString(&entry->FullImageName, entry->NameBuffer, sizeof(entry->NameBuffer));
	RtlCopyUnicodeString(&entry->FullImageName, FullImageName);

	entry->Method = Method;
	entry->DriverObject = DriverObject;
	entry->DeviceObject = DeviceObject;
}