#pragma once
#include<ntifs.h>

#define THREAD_QUERY_INFORMATION 0x0040

typedef struct CREATED_THREAD {
	PVOID StartAddress;
	BOOLEAN isCreatedRemotely;
	HANDLE ThreadId;
	HANDLE CreatorId;
};

VOID SetCreateThreadCallback();