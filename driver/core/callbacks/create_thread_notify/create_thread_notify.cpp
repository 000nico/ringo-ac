// This is util to detect manual map injections, because we can know if a 
// new thread appeared and see if its start address does match with a legitime module known in that process

#include <ntifs.h>
#include "sdk/sdk.hpp"
#include "../../global.hpp"
#include "../../reporting/report_thread_detection/report_thread_detection.hpp"
#include "create_thread_notify.hpp"

#define THREAD_QUERY_INFORMATION 0x0040

PVOID get_start_address(HANDLE thread_id) {
    PETHREAD thread;
    if (!NT_SUCCESS(PsLookupThreadByThreadId(thread_id, &thread)))
        return NULL;

    // open handle to thread
    HANDLE thread_handle;
    if (!NT_SUCCESS(ObOpenObjectByPointer(thread, OBJ_KERNEL_HANDLE, NULL, THREAD_QUERY_INFORMATION, *PsThreadType, KernelMode, &thread_handle))) {
        ObDereferenceObject(thread);
        return NULL;
    }

    PVOID start_address = NULL;
    ULONG return_length = 0;
    NTSTATUS status = ZwQueryInformationThread(thread_handle, (THREADINFOCLASS)9, &start_address, sizeof(PVOID), &return_length);
    if (!NT_SUCCESS(status))
        start_address = NULL;

    ZwClose(thread_handle);
    ObDereferenceObject(thread);

    return start_address;
}

BOOLEAN is_created_remotely(HANDLE process_id) {
    return PsGetCurrentProcessId() != process_id;
}

VOID create_thread_callback(_In_ HANDLE process_id, _In_ HANDLE thread_id, _In_ BOOLEAN create) {
    if (!g_ringo_context.game.is_game_open || g_ringo_context.game.protected_game_pid == NULL)
        return;

    if (process_id == (HANDLE)g_ringo_context.game.protected_game_pid && create == TRUE) {
        report_thread_detection(get_start_address(thread_id), is_created_remotely(process_id), thread_id, PsGetCurrentProcessId());
    }
}

VOID set_create_thread_callback() {
    NTSTATUS status = PsSetCreateThreadNotifyRoutineEx(PsCreateThreadNotifyNonSystem, create_thread_callback);

    if (NT_SUCCESS(status)) {
        g_ringo_context.callbacks.create_thread.is_registered = TRUE;
        g_ringo_context.callbacks.create_thread.callback_ptr = create_thread_callback;
    }
    else {
        g_ringo_context.callbacks.create_thread.is_registered = FALSE;
    }
}
