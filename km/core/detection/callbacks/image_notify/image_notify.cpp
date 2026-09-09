#include "../../../global.hpp"
#include "../../../../sdk/crypto/hashes.hpp"
#include "../../vulnerable_drivers/byovd_scan.hpp"
#include "image_notify.hpp"
#include "../../vulnerable_drivers/byovd_scan.hpp"

NTSTATUS GetDriverObjectFromImageName(PUNICODE_STRING FullImageName, PDRIVER_OBJECT* OutDriverObject) {
    UNICODE_STRING fileName = *FullImageName;

    for (int i = fileName.Length / sizeof(WCHAR) - 1; i >= 0; i--) {
        if (fileName.Buffer[i] == L'\\') {
            fileName.Buffer = &fileName.Buffer[i + 1];
            fileName.Length -= (USHORT)((i + 1) * sizeof(WCHAR));
            break;
        }
    }

    UNICODE_STRING driverName = fileName;
    for (int i = driverName.Length / sizeof(WCHAR) - 1; i >= 0; i--) {
        if (driverName.Buffer[i] == L'.') {
            driverName.Length = (USHORT)(i * sizeof(WCHAR));
            break;
        }
    }

    WCHAR pathBuffer[256];
    UNICODE_STRING driverPath;
    RtlInitEmptyUnicodeString(&driverPath, pathBuffer, sizeof(pathBuffer));
    RtlAppendUnicodeToString(&driverPath, L"\\Driver\\");
    RtlAppendUnicodeStringToString(&driverPath, &driverName);

    return ObReferenceObjectByName(
        &driverPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        0,
        *IoDriverObjectType,   
        KernelMode,
        NULL,
        (PVOID*)OutDriverObject
    );
}

VOID VulnerableDriverDetection(PUNICODE_STRING FullImageName) {
    if (isDriverVulnerable(FullImageName)) {
        KdPrint(("Vulnerable driver detected"));

        PDRIVER_OBJECT driverObj = NULL;
        NTSTATUS driverObjStatus = GetDriverObjectFromImageName(FullImageName, &driverObj);

        if (NT_SUCCESS(driverObjStatus) && driverObj != NULL) {
            PDEVICE_OBJECT deviceObj = driverObj->DeviceObject;

            if (deviceObj != NULL) {
                ObReferenceObject(deviceObj);
                AddVulnerableDriverDetectionEntry(FullImageName, driverObj, deviceObj, METHOD_IMAGE_CALLBACK); // this entries gets monitored by obcallback, which strips handle rights
            }
            ObfDereferenceObject(driverObj);
        }

        // maybe try to unload the driver(?
    }
}

VOID DllDetection(PIMAGE_INFO ImageInfo, PUNICODE_STRING FullImageName) {
    BOOLEAN isSigned = (ImageInfo->ImageSignatureLevel > SE_SIGNING_LEVEL_UNSIGNED);
    if (isSigned) return;

    if (ringo_context.Detections.DllDetectionReport.Count < 256) {
        LOADED_MODULE fd = {};
        RtlInitEmptyUnicodeString(&fd.FullImageName, fd.NameBuffer, sizeof(fd.NameBuffer));
        RtlCopyUnicodeString(&fd.FullImageName, FullImageName);

        fd.Mem.Image.ImageBase = ImageInfo->ImageBase;
        fd.Mem.Image.ImageSize = ImageInfo->ImageSize;
        fd.isSigned = isSigned;

        LONG index = InterlockedIncrement((PLONG)&ringo_context.Detections.DllDetectionReport.Count) - 1;
        if (index < 256) {
            RtlCopyMemory(&ringo_context.Detections.DllDetectionReport.Entries[index], &fd, sizeof(LOADED_MODULE));
            ringo_context.Detections.DllDetectionReport.Entries[index].FullImageName.Buffer = ringo_context.Detections.DllDetectionReport.Entries[index].NameBuffer;
        }
    }
}

VOID LoadImageCallback(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo) {
    if (FullImageName == NULL || ImageInfo == NULL) return;

    if (ProcessId == NULL || ProcessId == (HANDLE)4) {
        VulnerableDriverDetection(FullImageName);
        return;
    }

    if (!ringo_context.Game.isGameOpen || ringo_context.Game.ProtectedGamePid == NULL) return;
    if (ProcessId != (HANDLE)ringo_context.Game.ProtectedGamePid) return;
    DllDetection(ImageInfo, FullImageName);
}

VOID SetLoadImageCallback() {
	NTSTATUS status = PsSetLoadImageNotifyRoutineEx(LoadImageCallback, 0);

	if (NT_SUCCESS(status)) {
		ringo_context.Callbacks.LoadImageCallbackPtr = LoadImageCallback;
		ringo_context.Callbacks.isLoadImageCallbackRegistered = TRUE;
	}
	else {
		ringo_context.Callbacks.isLoadImageCallbackRegistered = FALSE;
	}
}