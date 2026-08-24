// this is util so we can know:
	// when the game its started or closed and its pid without manually checking or sending the pid via ioctl
	// detect sussy process such as process hacker, x64dbg, etc while the game's running
    // check the parent proces of anything being created, if the parent its the game, its sus

#include "create_process_notify.hpp"
#include "../../../../global.hpp"
#include "../../../../../sdk/imports/imports.hpp"

VOID PcreateProcessNotifyRoutineEx(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo) {
	PUCHAR name = PsGetProcessImageFileName(Process);
	if (!name) return;

	if (strcmp((const char*)name, (const char*)ringo_context.ProtectedGameProcessName) == 0) {
		// process created
		if (CreateInfo) {
			ringo_context.ProtectedGamePid = ProcessId;
			ringo_context.isGameOpen = TRUE;
		}
		// process destroyed
		else {
			ringo_context.ProtectedGamePid = NULL;
			ringo_context.isGameOpen = FALSE;
		}
	}
}

VOID SetCreateProcessCallback() {
	NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(PcreateProcessNotifyRoutineEx, FALSE);
	if (NT_SUCCESS(status)) {
		ringo_context.Callbacks.isCreateProcessCallbackRegistered = TRUE;
		ringo_context.Callbacks.CreateProcessNotifyRoutinePtr = PcreateProcessNotifyRoutineEx;
	}
	else {
		ringo_context.Callbacks.isCreateProcessCallbackRegistered = FALSE;
	}
}