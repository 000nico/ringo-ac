#include "vbs.hpp"
#include "../../../sdk/imports/imports.hpp"

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
	VBS_CHECK vc = {0};
	BOOLEAN isVBSEnabled, isHVCIEnabled;

	if (zwCheck(&isVBSEnabled, &isHVCIEnabled)) {
		vc.isHVCIEnabled = isHVCIEnabled;
		vc.isVBSEnabled = isVBSEnabled;
	}

	return vc;
}