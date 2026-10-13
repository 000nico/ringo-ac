#include "sdk/sdk.hpp"
#include "hvci.hpp"

BOOLEAN zw_check(BOOLEAN* p_hvci_enabled) {
	SYSTEM_CODEINTEGRITY_INFORMATION sci = { 0 };
	sci.Length = sizeof(sci);
	
	ULONG return_length = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemCodeIntegrityInformation, &sci, sizeof(sci), &return_length);

	if (!NT_SUCCESS(status)) {
		return FALSE;
	}
	else {
		BOOLEAN hvci_enabled = (sci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED) != 0;
		*p_hvci_enabled = hvci_enabled;
		return TRUE;
	}
}

HVCI_CHECK is_hvci_enabled() {
	HVCI_CHECK hc = {};

	BOOLEAN hvci_enabled;
	if (zw_check(&hvci_enabled)) {
		hc.code_integrity.valid = TRUE;
		hc.code_integrity.value = hvci_enabled;
	}

	return hc;
}
