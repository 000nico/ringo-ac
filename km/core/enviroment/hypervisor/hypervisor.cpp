#include "hypervisor.hpp"
#include "../../../sdk/imports/imports.hpp"
#include <intrin.h>

BOOLEAN zwSystemHypervisorInformationCheck(BOOLEAN* pIsHypervisorPresent, BOOLEAN* pManipulatedInformation) {
	ULONG returnLenght = 0;

	SYSTEM_HYPERVISOR_INFORMATION shi = { 0 };
	NTSTATUS status = ZwQuerySystemInformation(SystemHypervisorInformation, &shi, sizeof(shi), &returnLenght);

	if (!NT_SUCCESS(status)) return false;
	else {
		*pIsHypervisorPresent = shi.HypervisorPresent;

		int cpuInfo[4];
		__cpuid(cpuInfo, 1);
		BOOLEAN rawBit = (cpuInfo[2] >> 31) & 1;

		if (*pIsHypervisorPresent != rawBit) *pManipulatedInformation = TRUE;
		else *pManipulatedInformation = FALSE;
		
		return TRUE;
	}
}

HYPERVISOR_CHECK getHypervisorInfo() {
	HYPERVISOR_CHECK hc;

	if (zwSystemHypervisorInformationCheck(&hc.hypervisorPresent, &hc.manipulatedInformation)) {
		return hc;
	}
	else {
		hc.hypervisorPresent = FALSE, hc.manipulatedInformation = FALSE;
		return hc;
	}
}