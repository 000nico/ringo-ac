#include<ntifs.h>
#include "../../../../global.hpp"
#include "image_notify.hpp"

VOID LoadImageCallback(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo) {
    if (ProcessId != (HANDLE)ringo_context.ProtectedGamePid)
        return;

    if (FullImageName == NULL)
        return;

    if (g_FlaggedDllsCount >= 256)
        return;

    FLAGGED_DLL fd = {};

    RtlInitEmptyUnicodeString(&fd.FullImageName, fd.NameBuffer, sizeof(fd.NameBuffer));
    RtlCopyUnicodeString(&fd.FullImageName, FullImageName);

    fd.Mem.Image.ImageBase = ImageInfo->ImageBase;
    fd.Mem.Image.ImageSize = ImageInfo->ImageSize;
    fd.isSigned = (ImageInfo->ImageSignatureLevel > SE_SIGNING_LEVEL_UNSIGNED);

    LONG index = InterlockedIncrement((PLONG)&g_FlaggedDllsCount) - 1;
    if (index < 256) {
        RtlCopyMemory(&g_FlaggedDlls[index], &fd, sizeof(FLAGGED_DLL));
        g_FlaggedDlls[index].FullImageName.Buffer = g_FlaggedDlls[index].NameBuffer; // fix puntero
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