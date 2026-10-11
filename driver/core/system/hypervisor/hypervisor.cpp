#include "hypervisor.hpp"
#include "sdk/sdk.hpp"
#include <intrin.h>

BOOLEAN zwSystemHypervisorInformationCheck(BOOLEAN* pIsHypervisorPresent) {
	ULONG returnLenght = 0;

	SYSTEM_HYPERVISOR_INFORMATION shi = { 0 };
	NTSTATUS status = ZwQuerySystemInformation(SystemHypervisorInformation, &shi, sizeof(shi), &returnLenght);

	if (!NT_SUCCESS(status)) return false;
	else {
		*pIsHypervisorPresent = shi.HypervisorPresent;
		return TRUE;
	}
}

HYPERVISOR_CHECK get_hypervisor_info() {
	HYPERVISOR_CHECK hc = {};

	BOOLEAN hypervisorPresent;
	if (zwSystemHypervisorInformationCheck(&hypervisorPresent)) {
		hc.nt_query.valid = TRUE;
		hc.nt_query.value = hypervisorPresent;
	}

	int cpuInfo[4];
	__cpuid(cpuInfo, 1);
	hc.cpuid.valid = TRUE;
	hc.cpuid.value = (cpuInfo[2] >> 31) & 1;

	return hc;
}