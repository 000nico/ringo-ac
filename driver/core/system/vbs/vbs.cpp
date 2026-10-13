#include "vbs.hpp"
#include "sdk/sdk.hpp"

BOOLEAN zw_check(BOOLEAN* p_is_vbs_enabled, BOOLEAN* p_is_hvci_enabled) {
	ULONG return_length = 0;

	SYSTEM_ISOLATED_USER_MODE_INFORMATION siumi = { 0 };
	NTSTATUS status = ZwQuerySystemInformation(SystemIsolatedUserModeInformation, &siumi, sizeof(siumi), &return_length);

	if (!NT_SUCCESS(status)) return FALSE;
	else {
		*p_is_hvci_enabled = siumi.HvciEnabled;
		*p_is_vbs_enabled = siumi.SecureKernelRunning;

		return TRUE;
	}
}

VBS_CHECK is_vbs_enabled() {
	VBS_CHECK vc = {};
	BOOLEAN is_vbs_enabled, is_hvci_enabled;

	if (zw_check(&is_vbs_enabled, &is_hvci_enabled)) {
		vc.nt_query_secure_kernel.valid = TRUE;
		vc.nt_query_secure_kernel.value = is_vbs_enabled;
		vc.nt_query_hvci.valid = TRUE;
		vc.nt_query_hvci.value = is_hvci_enabled;
	}

	return vc;
}