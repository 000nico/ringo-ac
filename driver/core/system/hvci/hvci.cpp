#include "sdk/sdk.hpp"
#include "hvci.hpp"

BOOLEAN zwCheck(BOOLEAN* pHvciEnabled) {
	SYSTEM_CODEINTEGRITY_INFORMATION sci = { 0 };
	sci.Length = sizeof(sci);
	
	ULONG returnLenght = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemCodeIntegrityInformation, &sci, sizeof(sci), &returnLenght);

	if (!NT_SUCCESS(status)) {
		return FALSE;
	}
	else {
		BOOLEAN hvciEnabled = (sci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED) != 0;
		*pHvciEnabled = hvciEnabled;
		return TRUE;
	}
}

HVCI_CHECK is_hvci_enabled() {
	HVCI_CHECK hc = {};

	BOOLEAN hvciEnabled;
	if (zwCheck(&hvciEnabled)) {
		hc.code_integrity.valid = TRUE;
		hc.code_integrity.value = hvciEnabled;
	}

	return hc;
}
