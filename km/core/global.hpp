#pragma once
#include "detection/usermode_cheats/callbacks/image_notify/image_notify.hpp"
#include "detection/usermode_cheats/callbacks/create_thread_notify/create_thread_notify.hpp"

typedef struct RINGO_CONTEXT {
	HANDLE ProtectedGamePid;
	PUCHAR ProtectedGameProcessName;
	BOOLEAN isGameOpen = FALSE;

	struct {
		PVOID ObCallbackHandle;

		BOOLEAN isLoadImageCallbackRegistered;
		PLOAD_IMAGE_NOTIFY_ROUTINE LoadImageCallbackPtr;

		BOOLEAN isCreateThreadCallbackRegistered;
		PCREATE_THREAD_NOTIFY_ROUTINE CreateThreadCallbackPtr;

		BOOLEAN isCreateProcessCallbackRegistered;
		PCREATE_PROCESS_NOTIFY_ROUTINE_EX CreateProcessNotifyRoutinePtr;
	} Callbacks;
};

// nico of the future: dont forget to initialize these arrays
LOADED_MODULE g_LoadedDlls[256];
volatile LONG g_LoadedDllsCount;

LOADED_MODULE g_FlaggedDlls[256];
volatile LONG g_FlaggedDllsCount;

CREATED_THREAD g_CreatedThreads[256];
volatile LONG g_CreatedThreadsCount;

RINGO_CONTEXT ringo_context;
