#include "../../../sdk/imports/imports.hpp"
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

HVCI_CHECK isHVCIEnabled() {
	HVCI_CHECK hc = { 0 };

	BOOLEAN hvciEnabled;
	if (zwCheck(&hvciEnabled)) {
		hc.isEnabled = hvciEnabled;
	}

	return hc;
}

