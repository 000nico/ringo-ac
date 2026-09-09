#include "vulnerable_driver_detections.hpp"
#include "../../global.hpp"

VOID AddVulnerableDriverDetectionEntry(PUNICODE_STRING FullImageName, PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT DeviceObject, BYOVD_DETECTION_METHOD Method) {
	LONG index = InterlockedIncrement(&ringo_context.Detections.ByovdDetectionReport.count) - 1;
	if (index >= 256) return;

	BYOVD_DETECTION_ENTRY* entry = &ringo_context.Detections.ByovdDetectionReport.Entries[index];
	RtlZeroMemory(entry, sizeof(BYOVD_DETECTION_ENTRY));

	RtlInitEmptyUnicodeString(&entry->FullImageName, entry->NameBuffer, sizeof(entry->NameBuffer));
	RtlCopyUnicodeString(&entry->FullImageName, FullImageName);

	entry->Method = Method;
	entry->DriverObject = DriverObject;
	entry->DeviceObject = DeviceObject;
}