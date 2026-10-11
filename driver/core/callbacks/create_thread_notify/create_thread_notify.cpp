// This is util to detect manual map injections, because we can know if a 
// new thread appeared and see if its start address does match with a legitime module known in that process

#include <ntifs.h>
#include "sdk/sdk.hpp"
#include "../../global.hpp"
#include "create_thread_notify.hpp"

PVOID get_start_address(HANDLE ThreadId) {
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

BOOLEAN is_created_remotely(HANDLE ProcessId) {
    return PsGetCurrentProcessId() != ProcessId;
}

VOID create_thread_callback(_In_ HANDLE ProcessId, _In_ HANDLE ThreadId, _In_ BOOLEAN Create) {
    if (!g_ringo_context.Game.is_game_open || g_ringo_context.Game.protected_game_pid == NULL)
        return;

    if (ProcessId == (HANDLE)g_ringo_context.Game.protected_game_pid && Create == TRUE) {
        // todo: crear una funcion q cree esto x mi
        CREATED_THREAD ct = {};
        ct.StartAddress = get_start_address(ThreadId);
        ct.is_created_remotely = is_created_remotely(ProcessId);
        ct.CreatorId = PsGetCurrentProcessId();
        ct.ThreadId = ThreadId;

        if (g_ringo_context.Detections.thread_detection_report.Count < 256) {
            LONG index = InterlockedIncrement((PLONG)&g_ringo_context.Detections.thread_detection_report.Count) - 1;
            if (index < 256) {
                RtlCopyMemory(&g_ringo_context.Detections.thread_detection_report.Entries[index], &ct, sizeof(CREATED_THREAD));
            }
        }
    }
}

VOID set_create_thread_callback() {
    NTSTATUS status = PsSetCreateThreadNotifyRoutineEx(PsCreateThreadNotifyNonSystem, create_thread_callback);

    if (NT_SUCCESS(status)) {
        g_ringo_context.Callbacks.CreateThread.is_registered = TRUE;
        g_ringo_context.Callbacks.CreateThread.callback_ptr = create_thread_callback;
    }
    else {
        g_ringo_context.Callbacks.CreateThread.is_registered = FALSE;
    }
}