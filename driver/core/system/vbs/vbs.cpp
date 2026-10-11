#include "vbs.hpp"
#include "sdk/sdk.hpp"

BOOLEAN zwCheck(BOOLEAN* pIsVBSEnabled, BOOLEAN* pIsHVCIEnabled) {
	ULONG returnLength = 0;

	SYSTEM_ISOLATED_USER_MODE_INFORMATION siumi = { 0 };
	NTSTATUS status = ZwQuerySystemInformation(SystemIsolatedUserModeInformation, &siumi, sizeof(siumi), &returnLength);

	if (!NT_SUCCESS(status)) return FALSE;
	else {
		*pIsHVCIEnabled = siumi.HvciEnabled;
		*pIsVBSEnabled = siumi.SecureKernelRunning;

		return TRUE;
	}
}

VBS_CHECK isVBSEnabled() {
	VBS_CHECK vc = {};
	BOOLEAN isVBSEnabled, is_hvci_enabled;

	if (zwCheck(&isVBSEnabled, &is_hvci_enabled)) {
		vc.nt_query_secure_kernel.valid = TRUE;
		vc.nt_query_secure_kernel.value = isVBSEnabled;
		vc.nt_query_hvci.valid = TRUE;
		vc.nt_query_hvci.value = is_hvci_enabled;
	}

	return vc;
}