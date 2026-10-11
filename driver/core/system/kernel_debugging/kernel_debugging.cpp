#include "sdk/sdk.hpp"
#include "kernel_debugging.hpp"

KERNEL_DEBUGGING_CHECK is_kernel_debugger_present() {
	KERNEL_DEBUGGING_CHECK kdc = {};
	SYSTEM_KERNEL_DEBUGGER_INFORMATION skdi = {};
	ULONG returnLength = 0;

	NTSTATUS status = ZwQuerySystemInformation(
		SystemKernelDebuggerInformation,
		&skdi,
		sizeof(skdi),
		&returnLength
	);

	if (NT_SUCCESS(status)) {
		kdc.nt_query_enabled.valid = TRUE;
		kdc.nt_query_enabled.value = skdi.KernelDebuggerEnabled;
		kdc.nt_query_not_present.valid = TRUE;
		kdc.nt_query_not_present.value = skdi.KernelDebuggerNotPresent;
	}

	return kdc;
}