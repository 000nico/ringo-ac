#include "../../../global.hpp"
#include "image_notify.hpp"

VOID LoadImageCallback(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo) {
    if (!ringo_context.isGameOpen || ringo_context.ProtectedGamePid == NULL)
        return;

    if (ProcessId != (HANDLE)ringo_context.ProtectedGamePid)
        return;

    if (FullImageName == NULL || ImageInfo == NULL)
        return;

    if (g_LoadedDllsCount >= 256 || ringo_context.Detections.DllDetectionReport.Count > 256)
        return;

    LOADED_MODULE fd = {};

    RtlInitEmptyUnicodeString(&fd.FullImageName, fd.NameBuffer, sizeof(fd.NameBuffer));
    RtlCopyUnicodeString(&fd.FullImageName, FullImageName);

    fd.Mem.Image.ImageBase = ImageInfo->ImageBase;
    fd.Mem.Image.ImageSize = ImageInfo->ImageSize;
    fd.isSigned = (ImageInfo->ImageSignatureLevel > SE_SIGNING_LEVEL_UNSIGNED);

    LONG index = InterlockedIncrement((PLONG)&g_LoadedDllsCount) - 1;
    if (index < 256) {
        RtlCopyMemory(&g_LoadedDlls[index], &fd, sizeof(LOADED_MODULE));
        g_LoadedDlls[index].FullImageName.Buffer = g_LoadedDlls[index].NameBuffer;
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