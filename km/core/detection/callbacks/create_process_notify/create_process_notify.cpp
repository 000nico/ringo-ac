// this is util so we can know:
	// when the game its started or closed and its pid without manually checking or sending the pid via ioctl
	// detect sussy process such as process hacker, x64dbg, etc while the game's running
    // check the parent proces of anything being created, if the parent its the game, its sus

#include "create_process_notify.hpp"
#include "../../../global.hpp"
#include "../../../../sdk/imports/imports.hpp"

VOID PcreateProcessNotifyRoutineEx(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo) {
	PUCHAR name = PsGetProcessImageFileName(Process);
	if (!name) return;

	if (ringo_context.Game.ProtectedGameProcessName[0] == '\0') return;

	if (_stricmp((const char*)name, (const char*)ringo_context.Game.ProtectedGameProcessName) == 0) {
		// process created
		if (CreateInfo) {
			ringo_context.Game.ProtectedGamePid = ProcessId;
			ringo_context.Game.isGameOpen = TRUE;
		}
		// process destroyed
		else {
			if (ringo_context.Game.ProtectedGamePid == ProcessId) {
				ringo_context.Game.ProtectedGamePid = NULL;
				ringo_context.Game.isGameOpen = FALSE;
			}
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