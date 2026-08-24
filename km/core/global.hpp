#pragma once
#include "detection/usermode_cheats/callbacks/image_notify/image_notify.hpp"

typedef struct RINGO_CONTEXT {
	ULONG ProtectedGamePid;

	struct {
		PVOID ObCallbackHandle;
		BOOLEAN isLoadImageCallbackRegistered;
		PLOAD_IMAGE_NOTIFY_ROUTINE LoadImageCallbackPtr;
	} Callbacks;
};

FLAGGED_DLL g_FlaggedDlls[256];
volatile LONG g_FlaggedDllsCount;

RINGO_CONTEXT ringo_context;
