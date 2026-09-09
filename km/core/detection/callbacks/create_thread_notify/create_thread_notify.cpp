// This is util to detect manual map injections, because we can know if a 
// new thread appeared and see if its start address does match with a legitime module known in that process

#include <ntifs.h>
#include "../../../../sdk/imports/imports.hpp"
#include "../../../global.hpp"
#include "create_thread_notify.hpp"

PVOID GetStartAddress(HANDLE ThreadId) {
    PETHREAD thread;
    if (!NT_SUCCESS(PsLookupThreadByThreadId(ThreadId, &thread)))
        return NULL;

    // open handle to thread
    HANDLE hThread;
    if (!NT_SUCCESS(ObOpenObjectByPointer(thread, OBJ_KERNEL_HANDLE, NULL, THREAD_QUERY_INFORMATION, *PsThreadType, KernelMode, &hThread))) {
        ObDereferenceObject(thread);
        return NULL;
    }

    PVOID startAddress = NULL;
    ULONG returnLength = 0;
    NTSTATUS status = ZwQueryInformationThread(hThread, (THREADINFOCLASS)9, &startAddress, sizeof(PVOID), &returnLength);
    if (!NT_SUCCESS(status))
        startAddress = NULL;

    ZwClose(hThread);
    ObDereferenceObject(thread);

    return startAddress;
}

BOOLEAN isCreatedRemotely(HANDLE ProcessId) {
    return PsGetCurrentProcessId() != ProcessId;
}

VOID CreateThreadCallback(_In_ HANDLE ProcessId, _In_ HANDLE ThreadId, _In_ BOOLEAN Create) {
    if (!ringo_context.Game.isGameOpen || ringo_context.Game.ProtectedGamePid == NULL)
        return;

    if (ProcessId == (HANDLE)ringo_context.Game.ProtectedGamePid && Create == TRUE) { 
        // todo: crear una funcion q cree esto x mi
        CREATED_THREAD ct = {};
        ct.StartAddress = GetStartAddress(ThreadId);
        ct.isCreatedRemotely = isCreatedRemotely(ProcessId);
        ct.CreatorId = PsGetCurrentProcessId();
        ct.ThreadId = ThreadId;

        if (ringo_context.Detections.ThreadDetectionReport.Count < 256) {
            LONG index = InterlockedIncrement((PLONG)&ringo_context.Detections.ThreadDetectionReport.Count) - 1;
            if (index < 256) {
                RtlCopyMemory(&ringo_context.Detections.ThreadDetectionReport.Entries[index], &ct, sizeof(CREATED_THREAD));
            }
        }
    }
}

VOID SetCreateThreadCallback() {
    NTSTATUS status = PsSetCreateThreadNotifyRoutineEx(PsCreateThreadNotifyNonSystem, CreateThreadCallback);

    if (NT_SUCCESS(status)) {
        ringo_context.Callbacks.isCreateThreadCallbackRegistered = TRUE;
        ringo_context.Callbacks.CreateThreadCallbackPtr = CreateThreadCallback;
    }
    else {
        ringo_context.Callbacks.isCreateThreadCallbackRegistered = FALSE;
    }
}