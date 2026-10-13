// this is util so we can know:
	// when the game its started or closed and its pid without manually checking or sending the pid via ioctl
	// detect sussy process such as process hacker, x64dbg, etc while the game's running
    // check the parent proces of anything being created, if the parent its the game, its sus

#include "create_process_notify.hpp"
#include "../../global.hpp"
#include "core/detection/integrity/hash_sections/hash_sections.hpp"
#include "sdk/crypto/hashes.hpp"
#include "sdk/sdk.hpp"
#include "wdm.h"

VOID update_game_state(PUCHAR name, HANDLE process_id, PPS_CREATE_NOTIFY_INFO create_info) {
	if (_stricmp((const char*)name, (const char*)g_ringo_context.game.protected_game_process_name) == 0) {
		// process created
		if (create_info) {
			g_ringo_context.game.protected_game_pid = process_id;
			g_ringo_context.game.is_game_open = TRUE;
			g_ringo_context.session.first_hash_catched = FALSE;
		}
		// process destroyed
		else {
			if (g_ringo_context.game.protected_game_pid == process_id) {
				g_ringo_context.game.protected_game_pid = NULL;
				g_ringo_context.game.is_game_open = FALSE;
				g_ringo_context.session.first_hash_catched = FALSE;
			}
		}
	}
}

VOID clean_session_on_close(PPS_CREATE_NOTIFY_INFO create_info, PEPROCESS Process) {
    if(create_info == NULL) {
        if(g_ringo_context.session.active && g_ringo_context.session.process == Process) {
            ObDereferenceObject(Process);
            g_ringo_context.session.active = FALSE;
            g_ringo_context.session.process = NULL;
            g_ringo_context.session.process_start_key = 0;
        }
    }
}

VOID pcreate_process_notify_routine_ex(_Inout_ PEPROCESS Process, _In_ HANDLE process_id, _Inout_opt_ PPS_CREATE_NOTIFY_INFO create_info) {
    clean_session_on_close(create_info, Process);
    
	PUCHAR name = PsGetProcessImageFileName(Process);
	if (!name) return;
	if (g_ringo_context.game.protected_game_process_name[0] == '\0') return;

	update_game_state(name, process_id, create_info);
}

VOID set_create_process_callback() {
	NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(pcreate_process_notify_routine_ex, FALSE);
	if (NT_SUCCESS(status)) {
		g_ringo_context.callbacks.create_process.is_registered = TRUE;
		g_ringo_context.callbacks.create_process.callback_ptr = pcreate_process_notify_routine_ex;
	}
	else {
		g_ringo_context.callbacks.create_process.is_registered = FALSE;
	}
}