#include "../../../sdk/imports/imports.hpp"
#include "kernel_debugging.hpp"

BOOLEAN zwQueryCheck(BOOLEAN* pIsKernelDebuggerPresent) {
	SYSTEM_KERNEL_DEBUGGER_INFORMATION skdi = { 0 };

	ULONG returnLenght = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemKernelDebuggerInformation, &skdi, sizeof(skdi), &returnLenght);

	if (!NT_SUCCESS(status)) return false;
	else {
		*pIsKernelDebuggerPresent = skdi.KernelDebuggerEnabled && !skdi.KernelDebuggerNotPresent; // kernel debugging can be enabled, but not necesarially present atm
		return true;
	}
}

KERNEL_DEBUGGING_CHECK isKernelDebuggerPresent() {
	KERNEL_DEBUGGING_CHECK kdc = { 0 };

	BOOLEAN isKernelDebuggerPresent;
	if (zwQueryCheck(&isKernelDebuggerPresent)) {
		kdc.isEnabled = isKernelDebuggerPresent;
	}

	return kdc;
}