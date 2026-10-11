#include "../../global.hpp"
#include "sdk/sdk.hpp"
#include "../../detection/byovd/byovd_scan.hpp"
#include "image_notify.hpp"
#include "../../detection/byovd/byovd_scan.hpp"

NTSTATUS get_driver_object_from_image_name(PUNICODE_STRING FullImageName, PDRIVER_OBJECT* OutDriverObject) {
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

VOID vulnerable_driver_detection(PUNICODE_STRING FullImageName) {
    if (is_driver_vulnerable(FullImageName)) {
        KdPrint(("Vulnerable driver detected"));

        PDRIVER_OBJECT driverObj = NULL;
        NTSTATUS driverObjStatus = get_driver_object_from_image_name(FullImageName, &driverObj);

        if (NT_SUCCESS(driverObjStatus) && driverObj != NULL) {
            PDEVICE_OBJECT deviceObj = driverObj->DeviceObject;

            if (deviceObj != NULL) {
                ObReferenceObject(deviceObj);
                add_byovd_report_entry(FullImageName, driverObj, deviceObj, METHOD_IMAGE_CALLBACK); // this entries gets monitored by ob_callback, which strips handle rights
            }
            ObfDereferenceObject(driverObj);
        }

        // maybe try to unload the driver(?
    }
}

VOID dll_detection(PIMAGE_INFO ImageInfo, PUNICODE_STRING FullImageName) {
    BOOLEAN isSigned = (ImageInfo->ImageSignatureLevel > SE_SIGNING_LEVEL_UNSIGNED);
    if (isSigned) return;

    if (g_ringo_context.Detections.dll_detection_report.Count < 256) {
        LOADED_MODULE fd = {};
        RtlInitEmptyUnicodeString(&fd.FullImageName, fd.NameBuffer, sizeof(fd.NameBuffer));
        RtlCopyUnicodeString(&fd.FullImageName, FullImageName);

        fd.Mem.Image.image_base = ImageInfo->ImageBase;
        fd.Mem.Image.image_size = ImageInfo->ImageSize;
        fd.isSigned = isSigned;

        LONG index = InterlockedIncrement((PLONG)&g_ringo_context.Detections.dll_detection_report.Count) - 1;
        if (index < 256) {
            RtlCopyMemory(&g_ringo_context.Detections.dll_detection_report.Entries[index], &fd, sizeof(LOADED_MODULE));
            g_ringo_context.Detections.dll_detection_report.Entries[index].FullImageName.Buffer = g_ringo_context.Detections.dll_detection_report.Entries[index].NameBuffer;
        }
    }
}

VOID load_image_callback(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo) {
    if (FullImageName == NULL || ImageInfo == NULL) return;

    if (ProcessId == NULL || ProcessId == (HANDLE)4) {
        vulnerable_driver_detection(FullImageName);
        return;
    }

    if (!g_ringo_context.Game.is_game_open || g_ringo_context.Game.protected_game_pid == NULL) return;
    if (ProcessId != (HANDLE)g_ringo_context.Game.protected_game_pid) return;
    dll_detection(ImageInfo, FullImageName);
}

VOID set_load_image_callback() {
	NTSTATUS status = PsSetLoadImageNotifyRoutineEx(load_image_callback, 0);

	if (NT_SUCCESS(status)) {
		g_ringo_context.Callbacks.LoadImage.callback_ptr = load_image_callback;
		g_ringo_context.Callbacks.LoadImage.is_registered = TRUE;
	}
	else {
		g_ringo_context.Callbacks.LoadImage.is_registered = FALSE;
	}
}