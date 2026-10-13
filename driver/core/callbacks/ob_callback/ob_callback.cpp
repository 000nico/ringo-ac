#include "../../global.hpp"
#include "ob_callback.hpp"

VOID strip_handle_to_byovd(POB_PRE_OPERATION_INFORMATION Info) {
	LONG count = g_ringo_context.detections.byovd_detection_report.count;
	if (count > 256) count = 256;

	for (int i = 0; i < count; i++) {
		if (Info->Object == g_ringo_context.detections.byovd_detection_report.entries[i].device_object) {
			if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
				Info->Parameters->CreateHandleInformation.DesiredAccess = 0;
			else if (Info->Operation == OB_OPERATION_HANDLE_DUPLICATE)
				Info->Parameters->DuplicateHandleInformation.DesiredAccess = 0;

			break;
		}
	}
}

VOID strip_handle_to_game(HANDLE caller_pid, HANDLE target_pid, POB_PRE_OPERATION_INFORMATION Info) {
	if (target_pid == (HANDLE)g_ringo_context.game.protected_game_pid && caller_pid != (HANDLE)g_ringo_context.game.protected_game_pid) {
		if (Info->ObjectType == *PsProcessType) {
			if (Info->Operation == OB_OPERATION_HANDLE_CREATE) {
				if (Info->Parameters->CreateHandleInformation.OriginalDesiredAccess & (PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION)) {
					Info->Parameters->CreateHandleInformation.DesiredAccess &= ~(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION);
				}
			}

			if (Info->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
				if (Info->Parameters->DuplicateHandleInformation.OriginalDesiredAccess & (PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION)) {
					Info->Parameters->DuplicateHandleInformation.DesiredAccess &= ~(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION);
				}
			}
		}
		else if (Info->ObjectType == *PsThreadType) {
			ACCESS_MASK dangerous_thread_mask = THREAD_TERMINATE | THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SET_INFORMATION | THREAD_DIRECT_IMPERSONATION;

			if (Info->Operation == OB_OPERATION_HANDLE_CREATE) {
				if (Info->Parameters->CreateHandleInformation.OriginalDesiredAccess & dangerous_thread_mask) {
					Info->Parameters->CreateHandleInformation.DesiredAccess &= ~dangerous_thread_mask;
				}
			}

			if (Info->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
				if (Info->Parameters->DuplicateHandleInformation.OriginalDesiredAccess & dangerous_thread_mask) {
					Info->Parameters->DuplicateHandleInformation.DesiredAccess &= ~dangerous_thread_mask;
				}
			}
		}
	}
}

OB_PREOP_CALLBACK_STATUS open_process_callback(_In_ PVOID RegistrationContext, _Inout_ POB_PRE_OPERATION_INFORMATION Info) {
	UNREFERENCED_PARAMETER(RegistrationContext);

	if (Info->KernelHandle) return OB_PREOP_SUCCESS; // Ignore kernel-mode handles to prevent breaking legitimate drivers / system services
	if (!g_ringo_context.game.is_game_open || g_ringo_context.game.protected_game_pid == NULL) return OB_PREOP_SUCCESS;
	if (Info->ObjectType != *PsProcessType && Info->ObjectType != *PsThreadType && Info->ObjectType != *IoDeviceObjectType) return OB_PREOP_SUCCESS;

	HANDLE caller_pid = PsGetProcessId(PsGetCurrentProcess());
	HANDLE target_pid = NULL;

	if (Info->ObjectType == *PsProcessType)     target_pid = PsGetProcessId((PEPROCESS)Info->Object);
	else if (Info->ObjectType == *PsThreadType) target_pid = PsGetThreadProcessId((PETHREAD)Info->Object);

	strip_handle_to_game(caller_pid, target_pid, Info);
	if(Info->ObjectType == *IoDeviceObjectType) strip_handle_to_byovd(Info);
	return OB_PREOP_SUCCESS;
}

// returns the object to use with ObUnRegisterCallbacks at driver unload
PVOID ob_register_callbacks_open_process() {
	OB_OPERATION_REGISTRATION operations[3] = {};
	operations[0].ObjectType = PsProcessType;
	operations[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	operations[0].PreOperation = open_process_callback;
	operations[0].PostOperation = NULL;

	operations[1].ObjectType = PsThreadType;
	operations[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	operations[1].PreOperation = open_process_callback;
	operations[1].PostOperation = NULL;

	operations[2].ObjectType = IoDeviceObjectType;
	operations[2].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	operations[2].PreOperation = open_process_callback;
	operations[2].PostOperation = NULL;

	UNICODE_STRING altitude = RTL_CONSTANT_STRING(L"321000");
	OB_CALLBACK_REGISTRATION ocr = {};
	ocr.Version = OB_FLT_REGISTRATION_VERSION;
	ocr.OperationRegistrationCount = 3;
	ocr.Altitude = altitude;
	ocr.RegistrationContext = NULL;
	ocr.OperationRegistration = operations;

	PVOID p_registration_handle = NULL;
	NTSTATUS status = ObRegisterCallbacks(&ocr, &p_registration_handle); 

	if (!NT_SUCCESS(status)) return NULL;
	else return p_registration_handle;
}