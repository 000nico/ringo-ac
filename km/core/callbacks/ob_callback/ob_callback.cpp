#include "../../global.hpp"
#include "ob_callback.hpp"

VOID strip_handle_to_byovd(POB_PRE_OPERATION_INFORMATION Info) {
	for (int i = 0; i < g_ringo_context.Detections.byovd_detection_report.count; i++) {
		if (Info->Object == g_ringo_context.Detections.byovd_detection_report.Entries[i].DeviceObject) {
			if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
				Info->Parameters->CreateHandleInformation.DesiredAccess = 0;
			else if (Info->Operation == OB_OPERATION_HANDLE_DUPLICATE)
				Info->Parameters->DuplicateHandleInformation.DesiredAccess = 0;

			break;
		}
	}
}

VOID strip_handle_to_game(HANDLE callerPid, HANDLE targetPid, POB_PRE_OPERATION_INFORMATION Info) {
	if (targetPid == (HANDLE)g_ringo_context.Game.protected_game_pid && callerPid != (HANDLE)g_ringo_context.Game.protected_game_pid) {
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
			ACCESS_MASK dangerousThreadMask = THREAD_TERMINATE | THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SET_INFORMATION | THREAD_DIRECT_IMPERSONATION;

			if (Info->Operation == OB_OPERATION_HANDLE_CREATE) {
				if (Info->Parameters->CreateHandleInformation.OriginalDesiredAccess & dangerousThreadMask) {
					Info->Parameters->CreateHandleInformation.DesiredAccess &= ~dangerousThreadMask;
				}
			}

			if (Info->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
				if (Info->Parameters->DuplicateHandleInformation.OriginalDesiredAccess & dangerousThreadMask) {
					Info->Parameters->DuplicateHandleInformation.DesiredAccess &= ~dangerousThreadMask;
				}
			}
		}
	}
}

OB_PREOP_CALLBACK_STATUS open_process_callback(_In_ PVOID RegistrationContext, _Inout_ POB_PRE_OPERATION_INFORMATION Info) {
	UNREFERENCED_PARAMETER(RegistrationContext);

	if (Info->KernelHandle) return OB_PREOP_SUCCESS; // Ignore kernel-mode handles to prevent breaking legitimate drivers / system services
	if (!g_ringo_context.Game.is_game_open || g_ringo_context.Game.protected_game_pid == NULL) return OB_PREOP_SUCCESS;
	if (Info->ObjectType != *PsProcessType && Info->ObjectType != *PsThreadType && Info->ObjectType != *IoDeviceObjectType) return OB_PREOP_SUCCESS;

	HANDLE callerPid = PsGetProcessId(PsGetCurrentProcess());
	HANDLE targetPid = NULL;

	if (Info->ObjectType == *PsProcessType)     targetPid = PsGetProcessId((PEPROCESS)Info->Object);
	else if (Info->ObjectType == *PsThreadType) targetPid = PsGetThreadProcessId((PETHREAD)Info->Object);

	strip_handle_to_game(callerPid, targetPid, Info);
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

	PVOID pRegistrationHandle = NULL;
	NTSTATUS status = ObRegisterCallbacks(&ocr, &pRegistrationHandle); 

	if (!NT_SUCCESS(status)) return NULL;
	else return pRegistrationHandle;
}