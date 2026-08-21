#include<ntifs.h>
#include<winnt.h>
#include "../../../../../global.hpp"

// todo: add whitelist process
OB_PREOP_CALLBACK_STATUS OpenProcessCallback(_In_ PVOID RegistrationContext, _Inout_ POB_PRE_OPERATION_INFORMATION Info) {
	if (Info->ObjectType != *PsProcessType && Info->ObjectType != *PsThreadType)
		return OB_PREOP_SUCCESS;

	HANDLE callerPid = PsGetProcessId(PsGetCurrentProcess());
	HANDLE targetPid = PsGetProcessId((PEPROCESS)Info->Object);

	if (targetPid == (HANDLE)ringo_context.ProtectedGamePid && callerPid != (HANDLE)ringo_context.ProtectedGamePid) {
		if (Info->Operation == OB_OPERATION_HANDLE_CREATE) {
			if (Info->Parameters->CreateHandleInformation.OriginalDesiredAccess & (PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION)) { // its a bitmask so == doesnt work well{
				Info->Parameters->CreateHandleInformation.DesiredAccess &= ~(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION);
			}
		}

		if (Info->Operation == OB_OPERATION_HANDLE_DUPLICATE) {
			if (Info->Parameters->DuplicateHandleInformation.OriginalDesiredAccess & (PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION)) {
				Info->Parameters->DuplicateHandleInformation.DesiredAccess &= ~(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION);
			}
		}
	}

	return OB_PREOP_SUCCESS;
}

// returns the object to use with ObUnRegisterCallbacks at driver unload
PVOID ObRegisterCallbacksOpenProcess() {
	// this structs declarates which type of object we are vigilating, which operations, and declarate which function to call in the callback
	OB_OPERATION_REGISTRATION operations[2];
	operations[0].ObjectType = PsProcessType;
	operations[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	operations[0].PreOperation = OpenProcessCallback;
	operations[0].PostOperation = NULL; // idc much about the after

	operations[1].ObjectType = PsThreadType;
	operations[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	operations[1].PreOperation = OpenProcessCallback;
	operations[1].PostOperation = NULL;

	UNICODE_STRING altitude = RTL_CONSTANT_STRING(L"321000");
	OB_CALLBACK_REGISTRATION ocr = {};
	ocr.Version = OB_FLT_REGISTRATION_VERSION;
	ocr.OperationRegistrationCount = 2;
	ocr.Altitude = altitude;
	ocr.RegistrationContext = NULL;
	ocr.OperationRegistration = operations;

	PVOID pRegistrationHandle = NULL;
	NTSTATUS status = ObRegisterCallbacks(&ocr, &pRegistrationHandle); // REMEMBER TO UNLOAD WHEN UNLOADING DRIVER!!
	if (!NT_SUCCESS(status)) {
		return NULL;
	}
	else {
		return pRegistrationHandle;
	}
}