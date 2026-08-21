#pragma once

typedef struct RINGO_CONTEXT {
	ULONG ProtectedGamePid;
	PVOID ObCallbackHandle;
};

RINGO_CONTEXT ringo_context;
