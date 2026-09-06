#include "../../../global.hpp"
#include "image_notify.hpp"

VOID LoadImageCallback(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo) {
    if (!ringo_context.Game.isGameOpen || ringo_context.Game.ProtectedGamePid == NULL)
        return;

    if (ProcessId != (HANDLE)ringo_context.Game.ProtectedGamePid)
        return;

    if (FullImageName == NULL || ImageInfo == NULL)
        return;

    BOOLEAN isSigned = (ImageInfo->ImageSignatureLevel > SE_SIGNING_LEVEL_UNSIGNED);
    if (isSigned)
        return;

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
            ringo_context.Detections.DllDetectionReport.Entries[index].FullImageName.Buffer = 
                ringo_context.Detections.DllDetectionReport.Entries[index].NameBuffer;
        }
    }
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